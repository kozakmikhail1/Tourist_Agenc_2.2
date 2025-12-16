#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogs/countrydialog.h"
#include "dialogs/hoteldialog.h"
#include "dialogs/companydialog.h"
#include "dialogs/tourdialog.h"
#include "dialogs/searchdialog.h"
#include "dialogs/booktourdialog.h"
#include "utils/numericsortitem.h"
#include "utils/filemanager.h"
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QInputDialog>
#include <QStringList>
#include <QTableWidgetItem>
#include <QVariant>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QSet>
#include <QMap>
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QStyle>
#include <QSize>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QPolygon>
#include <QRect>
#include <QBrush>
#include <QPen>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDateTime>
#include <algorithm>
#include <limits>
#include <exception>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , networkManager_(new QNetworkAccessManager(this))
    , currencyTimer_(new QTimer(this))
    , tableManager_(new TableManager())
    , filterManager_(new FilterManager())
    , filterComboUpdater_(new FilterComboUpdater())
{
    ui->setupUi(this);
    setupUI();
    setupCurrencyUpdater();
    setupMenuBar();
    setupStatusBar();
    setupTables();
    
    try {
        loadData();
    } catch (const FileException& e) {
        QString message = QString("Не удалось загрузить данные: %1").arg(e.what());
        QMessageBox::warning(this, "Предупреждение", message);
    }
}

MainWindow::~MainWindow() {
    for (Action* action : actions_) {
        delete action;
    }
    actions_.clear();
    
    delete tableManager_;
    delete filterManager_;
    delete filterComboUpdater_;
}

void MainWindow::setupUI() {
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    QWidget* currencyCornerWidget = new QWidget(this);
    QHBoxLayout* currencyLayout = new QHBoxLayout(currencyCornerWidget);
    currencyLayout->setContentsMargins(0, 0, 10, 0);
    currencyLayout->setSpacing(15);
    
    QLabel* usdLabel = new QLabel("USD: загрузка...", currencyCornerWidget);
    usdLabel->setObjectName("usdLabel");
    usdLabel->setStyleSheet("color: #2196F3; font-weight: bold;");
    
    QLabel* eurLabel = new QLabel("EUR: загрузка...", currencyCornerWidget);
    eurLabel->setObjectName("eurLabel");
    eurLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
    
    QLabel* updateLabel = new QLabel("", currencyCornerWidget);
    updateLabel->setObjectName("currencyUpdateLabel");
    updateLabel->setStyleSheet("color: #888; font-size: 8pt;");
    
    currencyLayout->addWidget(usdLabel);
    currencyLayout->addWidget(eurLabel);
    currencyLayout->addWidget(updateLabel);
    
    ui->tabWidget->setCornerWidget(currencyCornerWidget, Qt::TopRightCorner);
    
    connect(ui->addCountryButton, &QPushButton::clicked, this, &MainWindow::addCountry);
    connect(ui->addHotelButton, &QPushButton::clicked, this, &MainWindow::addHotel);
    connect(ui->addTransportButton, &QPushButton::clicked, this, &MainWindow::addTransportCompany);
    connect(ui->addTourButton, &QPushButton::clicked, this, &MainWindow::addTour);
    connect(ui->addOrderButton, &QPushButton::clicked, this, &MainWindow::addOrder);
    
    ui->addCountryButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addHotelButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addTransportButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addTourButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addOrderButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    
    connect(ui->searchCountryEdit, &QLineEdit::textChanged, this, &MainWindow::applyCountriesFilters);
    connect(ui->filterCountryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyCountriesFilters);
    connect(ui->filterCountryCurrencyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyCountriesFilters);
    
    connect(ui->searchHotelEdit, &QLineEdit::textChanged, this, &MainWindow::applyHotelsFilters);
    connect(ui->filterHotelCountryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyHotelsFilters);
    connect(ui->filterHotelStarsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyHotelsFilters);
    
    connect(ui->searchTransportEdit, &QLineEdit::textChanged, this, &MainWindow::applyTransportFilters);
    connect(ui->filterTransportTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyTransportFilters);
    
    connect(ui->searchTourEdit, &QLineEdit::textChanged, this, &MainWindow::applyToursFilters);
    connect(ui->filterTourCountryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyToursFilters);
    connect(ui->filterTourMinPriceEdit, &QLineEdit::textChanged, this, &MainWindow::applyToursFilters);
    connect(ui->filterTourMaxPriceEdit, &QLineEdit::textChanged,             this, &MainWindow::applyToursFilters);
    
    connect(ui->searchOrderEdit, &QLineEdit::textChanged, this, &MainWindow::applyOrdersFilters);
    connect(ui->filterOrderStatusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyOrdersFilters);
    connect(ui->filterOrderMinCostEdit, &QLineEdit::textChanged, this, &MainWindow::applyOrdersFilters);
    connect(ui->filterOrderMaxCostEdit, &QLineEdit::textChanged, this, &MainWindow::applyOrdersFilters);
}

void MainWindow::setupCurrencyUpdater() {
    connect(networkManager_, &QNetworkAccessManager::finished, 
            this, &MainWindow::onCurrencyDataReceived);
    
    connect(currencyTimer_, &QTimer::timeout, this, &MainWindow::updateCurrencyRates);
    currencyTimer_->start(60000);
    
    updateCurrencyRates();
}

void MainWindow::setupMenuBar() {
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveData);
    connect(ui->actionLoad, &QAction::triggered, this, &MainWindow::loadData);
    
    connect(ui->actionAddHotel, &QAction::triggered, this, &MainWindow::addHotel);
    connect(ui->actionEditHotel, &QAction::triggered, this, &MainWindow::editHotel);
    connect(ui->actionDeleteHotel, &QAction::triggered, this, &MainWindow::deleteHotel);
    
    connect(ui->actionAddTransport, &QAction::triggered, this, &MainWindow::addTransportCompany);
    connect(ui->actionEditTransport, &QAction::triggered, this, &MainWindow::editTransportCompany);
    connect(ui->actionDeleteTransport, &QAction::triggered, this, &MainWindow::deleteTransportCompany);
    
    connect(ui->actionAddTour, &QAction::triggered, this, &MainWindow::addTour);
    connect(ui->actionEditTour, &QAction::triggered, this, &MainWindow::editTour);
    connect(ui->actionDeleteTour, &QAction::triggered, this, &MainWindow::deleteTour);
    connect(ui->actionSearchTours, &QAction::triggered, this, &MainWindow::searchTours);
    
    connect(ui->actionAddOrder, &QAction::triggered, this, &MainWindow::addOrder);
    connect(ui->actionDeleteOrder, &QAction::triggered, this, &MainWindow::deleteOrder);
    
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Готово");
}

void MainWindow::setupTables() {
    auto optimizeTable = [](QTableWidget* table) {
        table->setAlternatingRowColors(false);
        table->setWordWrap(false);
        table->setShowGrid(false);
        table->setAutoScroll(false);
        table->setDragDropMode(QAbstractItemView::NoDragDrop);
        table->setAttribute(Qt::WA_StaticContents, true);
    };
    
    optimizeTable(ui->countriesTable);
    optimizeTable(ui->hotelsTable);
    optimizeTable(ui->transportTable);
    optimizeTable(ui->toursTable);
    optimizeTable(ui->ordersTable);
    
    ui->countriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->countriesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->countriesTable->setSortingEnabled(false);
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->countriesTable->setColumnWidth(4, 150);
    ui->countriesTable->horizontalHeader()->setStretchLastSection(false);
    if (ui->countriesTable->horizontalHeaderItem(4)) {
        ui->countriesTable->horizontalHeaderItem(4)->setFlags(Qt::NoItemFlags);
    }
    ui->countriesTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->countriesTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onCountriesHeaderClicked);
    
    ui->hotelsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->hotelsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->hotelsTable->setSortingEnabled(false);
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->hotelsTable->setColumnWidth(2, 100);
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->hotelsTable->setColumnWidth(4, 180);
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->hotelsTable->setColumnWidth(5, 150);
    ui->hotelsTable->horizontalHeader()->setStretchLastSection(false);
    if (ui->hotelsTable->horizontalHeaderItem(4)) {
        ui->hotelsTable->horizontalHeaderItem(4)->setText("Количество\nномеров");
    }
    ui->hotelsTable->horizontalHeader()->setMinimumHeight(50);
    if (ui->hotelsTable->horizontalHeaderItem(5)) {
        ui->hotelsTable->horizontalHeaderItem(5)->setFlags(Qt::NoItemFlags);
    }
    ui->hotelsTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->hotelsTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onHotelsHeaderClicked);
    
    ui->transportTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->transportTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->transportTable->setSortingEnabled(false);
    ui->transportTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->transportTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->transportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->transportTable->setColumnWidth(2, 160);
    ui->transportTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->transportTable->setColumnWidth(3, 160);
    ui->transportTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->transportTable->setColumnWidth(4, 160);
    ui->transportTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->transportTable->setColumnWidth(5, 150);
    ui->transportTable->horizontalHeader()->setStretchLastSection(false);
    if (ui->transportTable->horizontalHeaderItem(5)) {
        ui->transportTable->horizontalHeaderItem(5)->setFlags(Qt::NoItemFlags);
    }
    
    ui->transportTable->horizontalHeader()->setMinimumHeight(50);
    
    if (ui->transportTable->horizontalHeaderItem(2)) {
        ui->transportTable->horizontalHeaderItem(2)->setText("Количество\nрейсов");
    }
    
    if (ui->transportTable->horizontalHeaderItem(3)) {
        ui->transportTable->horizontalHeaderItem(3)->setText("Дата\nотправления");
    }
    if (ui->transportTable->horizontalHeaderItem(4)) {
        ui->transportTable->horizontalHeaderItem(4)->setText("Дата\nприбытия");
    }
    
    ui->transportTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->transportTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onTransportHeaderClicked);
    
    ui->toursTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->toursTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->toursTable->setSortingEnabled(false);
    ui->toursTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->toursTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->toursTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->toursTable->setColumnWidth(2, 200);
    ui->toursTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->toursTable->setColumnWidth(3, 200);
    ui->toursTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->toursTable->setColumnWidth(4, 160);
    ui->toursTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->toursTable->setColumnWidth(5, 150);
    ui->toursTable->horizontalHeader()->setStretchLastSection(false);
    if (ui->toursTable->horizontalHeaderItem(5)) {
        ui->toursTable->horizontalHeaderItem(5)->setFlags(Qt::NoItemFlags);
    }
    ui->toursTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->toursTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onToursHeaderClicked);
    
    ui->ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ordersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ordersTable->setSortingEnabled(false);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->ordersTable->setColumnWidth(2, 180);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->ordersTable->setColumnWidth(4, 160);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->ordersTable->setColumnWidth(5, 160);
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    ui->ordersTable->setColumnWidth(6, 180);
    ui->ordersTable->horizontalHeader()->setStretchLastSection(false);
    if (ui->ordersTable->horizontalHeaderItem(6)) {
        ui->ordersTable->horizontalHeaderItem(6)->setFlags(Qt::NoItemFlags);
    }
    ui->ordersTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->ordersTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onOrdersHeaderClicked);
    
    ui->countriesTable->horizontalHeader()->setMinimumSectionSize(80);
    ui->hotelsTable->horizontalHeader()->setMinimumSectionSize(80);
    ui->transportTable->horizontalHeader()->setMinimumSectionSize(100);
    ui->toursTable->horizontalHeader()->setMinimumSectionSize(120);
    ui->ordersTable->horizontalHeader()->setMinimumSectionSize(100);
    
    setupControlsAdaptivity();
    
    updateTablesFontSize();
}

void MainWindow::setupControlsAdaptivity() {
    QFont labelFont;
    labelFont.setPointSize(12);
    
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->countriesControlsWidget->layout())) {
        layout->setStretchFactor(ui->addCountryButton, 0);
        layout->setStretchFactor(ui->searchCountryEdit, 2);
        layout->setStretchFactor(ui->filterCountryCombo, 1);
        layout->setStretchFactor(ui->filterCountryCurrencyCombo, 1);
    }
    ui->searchCountryLabel->setFont(labelFont);
    ui->filterCountryLabel->setFont(labelFont);
    ui->filterCountryCurrencyLabel->setFont(labelFont);
    
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->hotelsControlsWidget->layout())) {
        layout->setStretchFactor(ui->addHotelButton, 0);
        layout->setStretchFactor(ui->searchHotelEdit, 2);
        layout->setStretchFactor(ui->filterHotelCountryCombo, 1);
        layout->setStretchFactor(ui->filterHotelStarsCombo, 1);
    }
    ui->searchHotelLabel->setFont(labelFont);
    ui->filterHotelCountryLabel->setFont(labelFont);
    ui->filterHotelStarsLabel->setFont(labelFont);
    
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->transportControlsWidget->layout())) {
        layout->setStretchFactor(ui->addTransportButton, 0);
        layout->setStretchFactor(ui->searchTransportEdit, 3);
        layout->setStretchFactor(ui->filterTransportTypeCombo, 1);
    }
    ui->searchTransportLabel->setFont(labelFont);
    ui->filterTransportTypeLabel->setFont(labelFont);
    
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->toursControlsWidget->layout())) {
        layout->setStretchFactor(ui->addTourButton, 0);
        layout->setStretchFactor(ui->searchTourEdit, 2);
        layout->setStretchFactor(ui->filterTourCountryCombo, 1);
        layout->setStretchFactor(ui->filterTourMinPriceEdit, 0);
        layout->setStretchFactor(ui->filterTourMaxPriceEdit, 0);
    }
    ui->searchTourLabel->setFont(labelFont);
    ui->filterTourCountryLabel->setFont(labelFont);
    ui->filterTourPriceLabel->setFont(labelFont);
    ui->filterTourPriceToLabel->setFont(labelFont);
    
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->ordersControlsWidget->layout())) {
        layout->setStretchFactor(ui->addOrderButton, 0);
        layout->setStretchFactor(ui->searchOrderEdit, 2);
        layout->setStretchFactor(ui->filterOrderStatusCombo, 1);
        layout->setStretchFactor(ui->filterOrderMinCostEdit, 0);
        layout->setStretchFactor(ui->filterOrderMaxCostEdit, 0);
    }
    ui->searchOrderLabel->setFont(labelFont);
    ui->filterOrderStatusLabel->setFont(labelFont);
    ui->filterOrderCostLabel->setFont(labelFont);
    ui->filterOrderCostToLabel->setFont(labelFont);
}

void MainWindow::updateTablesFontSize() {
    const int baseWidth = 900;
    const int baseHeight = 600;
    const int baseFontSize = 10;
    
    QSize currentSize = size();
    int currentWidth = currentSize.width();
    int currentHeight = currentSize.height();
    
    double widthRatio = static_cast<double>(currentWidth) / baseWidth;
    double heightRatio = static_cast<double>(currentHeight) / baseHeight;
    double scaleFactor = (widthRatio + heightRatio) / 2.0;
    
    scaleFactor = qBound(0.8, scaleFactor, 1.5);
    
    int fontSize = qRound(baseFontSize * scaleFactor);
    
    QFont tableFont;
    tableFont.setPointSize(fontSize);
    
    auto updateTableFont = [&tableFont](QTableWidget* table) {
        table->setFont(tableFont);
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = table->item(row, col);
                if (item) {
                    QFont itemFont = item->font();
                    itemFont.setPointSize(tableFont.pointSize());
                    item->setFont(itemFont);
                }
            }
        }
    };
    
    updateTableFont(ui->countriesTable);
    updateTableFont(ui->hotelsTable);
    updateTableFont(ui->transportTable);
    updateTableFont(ui->toursTable);
    updateTableFont(ui->ordersTable);
    
    QFont headerFont = tableFont;
    headerFont.setBold(true);
    headerFont.setPointSize(qRound(fontSize * 1.1));
    
    ui->countriesTable->horizontalHeader()->setFont(headerFont);
    ui->hotelsTable->horizontalHeader()->setFont(headerFont);
    ui->transportTable->horizontalHeader()->setFont(headerFont);
    ui->toursTable->horizontalHeader()->setFont(headerFont);
    ui->ordersTable->horizontalHeader()->setFont(headerFont);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updateTablesFontSize();
}

QWidget* MainWindow::createActionButtons(int dataIndex, const QString& type) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);
    
    auto createRedCrossIcon = []() -> QIcon {
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(220, 53, 69), 2.5));
        painter.drawLine(3, 3, 17, 17);
        painter.drawLine(17, 3, 3, 17);
        return QIcon(pixmap);
    };
    
    QIcon redCrossIcon = createRedCrossIcon();
    
    auto createButton = [this, dataIndex](QStyle::StandardPixmap icon, const QString& tooltip) -> QPushButton* {
        QPushButton* btn = new QPushButton();
        btn->setIcon(style()->standardIcon(icon));
        btn->setToolTip(tooltip);
        btn->setProperty("dataIndex", dataIndex);
        btn->setMaximumWidth(32);
        btn->setMaximumHeight(32);
        btn->setIconSize(QSize(20, 20));
        btn->setFlat(true);
        return btn;
    };
    
    auto createEditIcon = []() -> QIcon {
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        painter.setBrush(QBrush(QColor(33, 150, 243)));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(0, 0, 20, 20, 4, 4);
        
        painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        
        painter.drawLine(6, 14, 14, 6);
        
        QPolygon tip;
        tip << QPoint(5, 15) << QPoint(6, 14) << QPoint(7, 15);
        painter.setBrush(QBrush(Qt::white));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(tip);
        
        painter.setPen(QPen(Qt::white, 1.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(4, 16, 9, 16);
        
        return QIcon(pixmap);
    };
    
    QIcon editIcon = createEditIcon();
    
    auto createProcessIcon = []() -> QIcon {
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        painter.setBrush(QBrush(QColor(255, 152, 0)));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 20, 20);
        
        painter.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(5, 10, 9, 14);
        painter.drawLine(9, 14, 15, 6);
        
        return QIcon(pixmap);
    };
    
    QIcon processIcon = createProcessIcon();
    
    auto createEditButton = [this, dataIndex, editIcon](const QString& tooltip) -> QPushButton* {
        QPushButton* btn = new QPushButton();
        btn->setIcon(editIcon);
        btn->setToolTip(tooltip);
        btn->setProperty("dataIndex", dataIndex);
        btn->setMaximumWidth(32);
        btn->setMaximumHeight(32);
        btn->setIconSize(QSize(20, 20));
        btn->setFlat(true);
        return btn;
    };
    
    auto createProcessButton = [this, dataIndex, processIcon](const QString& tooltip) -> QPushButton* {
        QPushButton* btn = new QPushButton();
        btn->setIcon(processIcon);
        btn->setToolTip(tooltip);
        btn->setProperty("dataIndex", dataIndex);
        btn->setMaximumWidth(32);
        btn->setMaximumHeight(32);
        btn->setIconSize(QSize(20, 20));
        btn->setFlat(true);
        return btn;
    };
    
    auto createDeleteButton = [this, dataIndex, redCrossIcon](const QString& tooltip) -> QPushButton* {
        QPushButton* btn = new QPushButton();
        btn->setIcon(redCrossIcon);
        btn->setToolTip(tooltip);
        btn->setProperty("dataIndex", dataIndex);
        btn->setMaximumWidth(32);
        btn->setMaximumHeight(32);
        btn->setIconSize(QSize(20, 20));
        btn->setFlat(true);
        return btn;
    };
    
    if (type == "country") {
        QPushButton* infoBtn = createButton(QStyle::SP_MessageBoxInformation, "Информация");
        connect(infoBtn, &QPushButton::clicked, this, &MainWindow::showCountryInfo);
        layout->addWidget(infoBtn);
        
        QPushButton* editBtn = createEditButton("Редактировать");
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::editCountry);
        layout->addWidget(editBtn);
        
        QPushButton* deleteBtn = createDeleteButton("Удалить");
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteCountry);
        layout->addWidget(deleteBtn);
    } else if (type == "hotel") {
        QPushButton* infoBtn = createButton(QStyle::SP_MessageBoxInformation, "Информация");
        connect(infoBtn, &QPushButton::clicked, this, &MainWindow::showHotelInfo);
        layout->addWidget(infoBtn);
        
        QPushButton* editBtn = createEditButton("Редактировать");
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::editHotel);
        layout->addWidget(editBtn);
        
        QPushButton* deleteBtn = createDeleteButton("Удалить");
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteHotel);
        layout->addWidget(deleteBtn);
    } else if (type == "transport") {
        QPushButton* infoBtn = createButton(QStyle::SP_MessageBoxInformation, "Информация");
        connect(infoBtn, &QPushButton::clicked, this, &MainWindow::showTransportCompanyInfo);
        layout->addWidget(infoBtn);
        
        QPushButton* editBtn = createEditButton("Редактировать");
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::editTransportCompany);
        layout->addWidget(editBtn);
        
        QPushButton* deleteBtn = createDeleteButton("Удалить");
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteTransportCompany);
        layout->addWidget(deleteBtn);
    } else if (type == "tour") {
        QPushButton* infoBtn = createButton(QStyle::SP_MessageBoxInformation, "Информация");
        connect(infoBtn, &QPushButton::clicked, this, &MainWindow::showTourInfo);
        layout->addWidget(infoBtn);
        
        QPushButton* editBtn = createEditButton("Редактировать");
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::editTour);
        layout->addWidget(editBtn);
        
        QPushButton* deleteBtn = createDeleteButton("Удалить");
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteTour);
        layout->addWidget(deleteBtn);
    } else if (type == "order") {
        QPushButton* infoBtn = createButton(QStyle::SP_MessageBoxInformation, "Информация");
        connect(infoBtn, &QPushButton::clicked, this, &MainWindow::showOrderInfo);
        layout->addWidget(infoBtn);
        
        QPushButton* editBtn = createEditButton("Редактировать");
        connect(editBtn, &QPushButton::clicked, this, &MainWindow::editOrder);
        layout->addWidget(editBtn);
        
        QPushButton* processBtn = createProcessButton("Обработать");
        connect(processBtn, &QPushButton::clicked, this, &MainWindow::processOrder);
        layout->addWidget(processBtn);
        
        QPushButton* deleteBtn = createDeleteButton("Удалить");
        connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteOrder);
        layout->addWidget(deleteBtn);
    }
    
    return widget;
}

void MainWindow::updateCountriesTable() {
    ui->countriesTable->setUpdatesEnabled(false);
    
    ui->countriesTable->setSortingEnabled(false);
    
    ui->countriesTable->clearContents();
    ui->countriesTable->setRowCount(countries_.size());
    
    int row = 0;
    int dataIndex = 0;
    for (const auto& country : countries_.getData()) {
        ui->countriesTable->setRowHidden(row, false);
        QTableWidgetItem* nameItem = new QTableWidgetItem(country.getName());
        nameItem->setData(Qt::UserRole, dataIndex);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->countriesTable->setItem(row, 0, nameItem);
        
        QTableWidgetItem* continentItem = new QTableWidgetItem(country.getContinent());
        continentItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->countriesTable->setItem(row, 1, continentItem);
        
        QTableWidgetItem* capitalItem = new QTableWidgetItem(country.getCapital());
        capitalItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->countriesTable->setItem(row, 2, capitalItem);
        
        QTableWidgetItem* currencyItem = new QTableWidgetItem(country.getCurrency());
        currencyItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->countriesTable->setItem(row, 3, currencyItem);
        
        ui->countriesTable->setCellWidget(row, 4, createActionButtons(dataIndex, "country"));
        
        ++row;
        ++dataIndex;
    }
    
    ui->countriesTable->setSortingEnabled(true);
    
    ui->countriesTable->setUpdatesEnabled(true);
    
    filterComboUpdater_->updateCountriesFilterCombo(ui->filterCountryCombo,
                                                     ui->filterCountryCurrencyCombo,
                                                     countries_);
    filterManager_->applyCountriesFilters(ui->countriesTable, ui->searchCountryEdit,
                                         ui->filterCountryCombo, ui->filterCountryCurrencyCombo);
}

void MainWindow::updateHotelsTable() {
    ui->hotelsTable->setUpdatesEnabled(false);
    
    ui->hotelsTable->setSortingEnabled(false);
    
    ui->hotelsTable->clearContents();
    ui->hotelsTable->setRowCount(hotels_.size());
    
    int row = 0;
    int dataIndex = 0;
    for (const auto& hotel : hotels_.getData()) {
        ui->hotelsTable->setRowHidden(row, false);
        QTableWidgetItem* nameItem = new QTableWidgetItem(hotel.getName());
        nameItem->setData(Qt::UserRole, dataIndex);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->hotelsTable->setItem(row, 0, nameItem);
        
        QTableWidgetItem* countryItem = new QTableWidgetItem(hotel.getCountry());
        countryItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->hotelsTable->setItem(row, 1, countryItem);
        
        QTableWidgetItem* starsItem = new QTableWidgetItem(QString::number(hotel.getStars()));
        starsItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->hotelsTable->setItem(row, 2, starsItem);
        
        QTableWidgetItem* addressItem = new QTableWidgetItem(hotel.getAddress());
        addressItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->hotelsTable->setItem(row, 3, addressItem);
        
        QTableWidgetItem* roomsItem = new QTableWidgetItem(QString::number(hotel.getRoomCount()));
        roomsItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->hotelsTable->setItem(row, 4, roomsItem);
        
        ui->hotelsTable->setCellWidget(row, 5, createActionButtons(dataIndex, "hotel"));
        
        ++row;
        ++dataIndex;
    }
    
    ui->hotelsTable->setSortingEnabled(true);
    
    ui->hotelsTable->setUpdatesEnabled(true);
    
    applyHotelsFilters();
}

void MainWindow::updateTransportCompaniesTable() {
    ui->transportTable->setUpdatesEnabled(false);
    
    ui->transportTable->setSortingEnabled(false);
    
    ui->transportTable->clearContents();
    ui->transportTable->setRowCount(transportCompanies_.size());
    
    int row = 0;
    int dataIndex = 0;
    for (const auto& company : transportCompanies_.getData()) {
        ui->transportTable->setRowHidden(row, false);
        QTableWidgetItem* nameItem = new QTableWidgetItem(company.getName());
        nameItem->setData(Qt::UserRole, dataIndex);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->transportTable->setItem(row, 0, nameItem);
        
        QTableWidgetItem* typeItem = new QTableWidgetItem(
            TransportCompany::transportTypeToString(company.getTransportType()));
        typeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->transportTable->setItem(row, 1, typeItem);
        
        int scheduleCount = company.getScheduleCount();
        NumericSortItem* scheduleCountItem = new NumericSortItem(
            QString::number(scheduleCount), 
            static_cast<double>(scheduleCount)
        );
        scheduleCountItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->transportTable->setItem(row, 2, scheduleCountItem);
        
        if (company.getScheduleCount() > 0) {
            QVector<TransportSchedule> schedules = company.getSchedules();
            if (!schedules.isEmpty()) {
                const TransportSchedule& firstSchedule = schedules.first();
                QTableWidgetItem* depItem = new QTableWidgetItem(firstSchedule.departureDate.toString("yyyy-MM-dd"));
                depItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
                ui->transportTable->setItem(row, 3, depItem);
                
                QTableWidgetItem* arrItem = new QTableWidgetItem(firstSchedule.arrivalDate.toString("yyyy-MM-dd"));
                arrItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
                ui->transportTable->setItem(row, 4, arrItem);
            } else {
                QTableWidgetItem* depItem = new QTableWidgetItem("-");
                depItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
                ui->transportTable->setItem(row, 3, depItem);
                
                QTableWidgetItem* arrItem = new QTableWidgetItem("-");
                arrItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
                ui->transportTable->setItem(row, 4, arrItem);
            }
        } else {
            QTableWidgetItem* depItem = new QTableWidgetItem("-");
            depItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
            ui->transportTable->setItem(row, 3, depItem);
            
            QTableWidgetItem* arrItem = new QTableWidgetItem("-");
            arrItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
            ui->transportTable->setItem(row, 4, arrItem);
        }
        
        ui->transportTable->setCellWidget(row, 5, createActionButtons(dataIndex, "transport"));
        
        ++row;
        ++dataIndex;
    }
    
    ui->transportTable->setSortingEnabled(true);
    
    ui->transportTable->setUpdatesEnabled(true);
    
    applyTransportFilters();
}

void MainWindow::updateToursTable() {
    ui->toursTable->setUpdatesEnabled(false);
    
    ui->toursTable->setSortingEnabled(false);
    
    ui->toursTable->clearContents();
    
    int validTourCount = 0;
    for (const auto& tour : tours_.getData()) {
        if (!tour.getName().isEmpty() && !tour.getCountry().isEmpty()) {
            validTourCount++;
        }
    }
    
    ui->toursTable->setRowCount(validTourCount);
    
    int row = 0;
    int dataIndex = 0;
    for (const auto& tour : tours_.getData()) {
        if (tour.getName().isEmpty() || tour.getCountry().isEmpty()) {
            ++dataIndex;
            continue;
        }
        
        ui->toursTable->setRowHidden(row, false);
        
        QTableWidgetItem* nameItem = new QTableWidgetItem(tour.getName());
        nameItem->setData(Qt::UserRole, dataIndex);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 0, nameItem);
        
        QTableWidgetItem* countryItem = new QTableWidgetItem(tour.getCountry());
        countryItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 1, countryItem);
        
        QString startDateStr = tour.getStartDate().isValid() ? 
            tour.getStartDate().toString("yyyy-MM-dd") : "";
        QString endDateStr = tour.getEndDate().isValid() ? 
            tour.getEndDate().toString("yyyy-MM-dd") : "";
        
        QTableWidgetItem* startDateItem = new QTableWidgetItem(startDateStr);
        startDateItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 2, startDateItem);
        
        QTableWidgetItem* endDateItem = new QTableWidgetItem(endDateStr);
        endDateItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 3, endDateItem);
        
        double cost = tour.calculateCost();
        NumericSortItem* costItem = new NumericSortItem(QString::number(cost, 'f', 2) + " руб", cost);
        costItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 4, costItem);
        
        ui->toursTable->setCellWidget(row, 5, createActionButtons(dataIndex, "tour"));
        
        ++row;
        ++dataIndex;
    }
    
    ui->toursTable->setSortingEnabled(true);
    
    ui->toursTable->setUpdatesEnabled(true);
    
    applyToursFilters();
}

void MainWindow::updateOrdersTable() {
    ui->ordersTable->setUpdatesEnabled(false);
    
    ui->ordersTable->setSortingEnabled(false);
    
    ui->ordersTable->clearContents();
    
    int validOrderCount = 0;
    for (const auto& order : orders_.getData()) {
        if (!order.getTour().getName().isEmpty() && 
            !order.getClientName().isEmpty() && 
            !order.getClientPhone().isEmpty()) {
            validOrderCount++;
        }
    }
    
    ui->ordersTable->setRowCount(validOrderCount);
    
    int row = 0;
    int dataIndex = 0;
    for (const auto& order : orders_.getData()) {
        if (order.getTour().getName().isEmpty() || 
            order.getClientName().isEmpty() || 
            order.getClientPhone().isEmpty()) {
            ++dataIndex;
            continue;
        }
        
        ui->ordersTable->setRowHidden(row, false);
        
        QTableWidgetItem* tourItem = new QTableWidgetItem(order.getTour().getName());
        tourItem->setData(Qt::UserRole, dataIndex);
        tourItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 0, tourItem);
        
        QTableWidgetItem* clientItem = new QTableWidgetItem(order.getClientName());
        clientItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 1, clientItem);
        
        QTableWidgetItem* phoneItem = new QTableWidgetItem(order.getClientPhone());
        phoneItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 2, phoneItem);
        
        QTableWidgetItem* emailItem = new QTableWidgetItem(order.getClientEmail());
        emailItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 3, emailItem);
        
        double cost = order.getTotalCost();
        NumericSortItem* costItem = new NumericSortItem(QString::number(cost, 'f', 2) + " руб", cost);
        costItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 4, costItem);
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(order.getStatus());
        statusItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 5, statusItem);
        
        ui->ordersTable->setCellWidget(row, 6, createActionButtons(dataIndex, "order"));
        
        ++row;
        ++dataIndex;
    }
    
    ui->ordersTable->setSortingEnabled(true);
    
    ui->ordersTable->setUpdatesEnabled(true);
    
    applyOrdersFilters();
}

int MainWindow::getSelectedRow(QTableWidget* table) const {
    QList<QTableWidgetItem*> items = table->selectedItems();
    if (items.isEmpty()) return -1;
    return items.first()->row();
}

int MainWindow::getSelectedTourIndex() const {
    QList<QTableWidgetItem*> items = ui->toursTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->toursTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    return -1;
}

int MainWindow::getSelectedOrderIndex() const {
    QList<QTableWidgetItem*> items = ui->ordersTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    int visualRow = items.first()->row();
    QTableWidgetItem* tourItem = ui->ordersTable->item(visualRow, 0);
    if (!tourItem) return -1;
    
    QVariant data = tourItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    return visualRow;
}

int MainWindow::getSelectedCountryIndex() const {
    QList<QTableWidgetItem*> items = ui->countriesTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->countriesTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    return visualRow;
}

int MainWindow::getSelectedHotelIndex() const {
    QList<QTableWidgetItem*> items = ui->hotelsTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->hotelsTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    return visualRow;
}

int MainWindow::getSelectedTransportIndex() const {
    QList<QTableWidgetItem*> items = ui->transportTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->transportTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    return visualRow;
}

void MainWindow::addCountry() {
    CountryDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Country country = dialog.getCountry();
        countries_.add(country);
        updateCountriesTable();
        updateCountriesFilterCombo();
        statusBar()->showMessage("Страна добавлена", 2000);
    }
}

void MainWindow::editCountry() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedCountryIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите страну для редактирования");
        return;
    }
    
    Country* country = countries_.get(dataIndex);
    if (!country) return;
    
    CountryDialog dialog(this, country);
    if (dialog.exec() == QDialog::Accepted) {
        Country newCountry = dialog.getCountry();
        *country = newCountry;
        updateCountriesTable();
        updateCountriesFilterCombo();
        applyCountriesFilters();
        statusBar()->showMessage("Страна обновлена", 2000);
    }
}

void MainWindow::deleteCountry() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedCountryIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите страну для удаления");
        return;
    }
    
    Country* country = countries_.get(dataIndex);
    if (!country) return;
    
    if (QMessageBox::question(this, "Подтверждение", 
        "Вы уверены, что хотите удалить эту страну?") == QMessageBox::Yes) {
        countries_.remove(dataIndex);
        updateCountriesTable();
        updateCountriesFilterCombo();
        applyCountriesFilters();
        statusBar()->showMessage("Страна удалена", 2000);
    }
}

void MainWindow::showCountryInfo() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedCountryIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите страну для просмотра информации");
        return;
    }
    
    Country* country = countries_.get(dataIndex);
    if (!country) return;
    
    QString info = QString("Информация о стране:\n\n"
                          "Название: %1\n"
                          "Континент: %2\n"
                          "Столица: %3\n"
                          "Валюта: %4")
                   .arg(country->getName())
                   .arg(country->getContinent())
                   .arg(country->getCapital())
                   .arg(country->getCurrency());
    
    QMessageBox::information(this, "Информация о стране", info);
}

void MainWindow::refreshCountries() {
    updateCountriesTable();
}

void MainWindow::addHotel() {
    HotelDialog dialog(this, &countries_);
    if (dialog.exec() == QDialog::Accepted) {
        Hotel hotel = dialog.getHotel();
        hotels_.add(hotel);
        updateHotelsTable();
        updateHotelsFilterCombos();
        statusBar()->showMessage("Отель добавлен", 2000);
    }
}

void MainWindow::editHotel() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedHotelIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите отель для редактирования");
        return;
    }
    
    Hotel* hotel = hotels_.get(dataIndex);
    if (!hotel) return;
    
    HotelDialog dialog(this, &countries_, hotel);
    if (dialog.exec() == QDialog::Accepted) {
        Hotel newHotel = dialog.getHotel();
        *hotel = newHotel;
        updateHotelsTable();
        updateHotelsFilterCombos();
        applyHotelsFilters();
        statusBar()->showMessage("Отель обновлен", 2000);
    }
}

void MainWindow::deleteHotel() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedHotelIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите отель для удаления");
        return;
    }
    
    Hotel* hotel = hotels_.get(dataIndex);
    if (!hotel) return;
    
    if (QMessageBox::question(this, "Подтверждение", 
        "Вы уверены, что хотите удалить этот отель?") == QMessageBox::Yes) {
        hotels_.remove(dataIndex);
        updateHotelsTable();
        updateHotelsFilterCombos();
        applyHotelsFilters();
        statusBar()->showMessage("Отель удален", 2000);
    }
}

void MainWindow::showHotelInfo() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedHotelIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите отель для просмотра информации");
        return;
    }
    
    Hotel* hotel = hotels_.get(dataIndex);
    if (!hotel) return;
    
    QString roomsInfo;
    if (hotel->getRoomCount() > 0) {
        roomsInfo = "\nНомера:\n";
        int roomNum = 1;
        for (const auto& room : hotel->getRooms()) {
            roomsInfo += QString("  %1. Тип: %2, Стоимость: %3 руб/ночь\n")
                        .arg(roomNum++)
                        .arg(room.getType())
                        .arg(room.getPricePerNight(), 0, 'f', 2);
        }
    } else {
        roomsInfo = "\nНомера: нет";
    }
    
    QString info = QString("Информация об отеле:\n\n"
                          "Название: %1\n"
                          "Страна: %2\n"
                          "Звезды: %3\n"
                          "Адрес: %4\n"
                          "Количество номеров: %5%6")
                   .arg(hotel->getName())
                   .arg(hotel->getCountry())
                   .arg(hotel->getStars())
                   .arg(hotel->getAddress())
                   .arg(hotel->getRoomCount())
                   .arg(roomsInfo);
    
    QMessageBox::information(this, "Информация об отеле", info);
}

void MainWindow::refreshHotels() {
    updateHotelsTable();
}

void MainWindow::addTransportCompany() {
    CompanyDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        TransportCompany company = dialog.getCompany();
        transportCompanies_.add(company);
        updateTransportCompaniesTable();
        applyTransportFilters();
        statusBar()->showMessage("Транспортная компания добавлена", 2000);
    }
}

void MainWindow::editTransportCompany() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedTransportIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите компанию для редактирования");
        return;
    }
    
    TransportCompany* company = transportCompanies_.get(dataIndex);
    if (!company) return;
    
    CompanyDialog dialog(this, company);
    if (dialog.exec() == QDialog::Accepted) {
        TransportCompany newCompany = dialog.getCompany();
        *company = newCompany;
        updateTransportCompaniesTable();
        applyTransportFilters();
        statusBar()->showMessage("Компания обновлена", 2000);
    }
}

void MainWindow::deleteTransportCompany() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedTransportIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите компанию для удаления");
        return;
    }
    
    TransportCompany* company = transportCompanies_.get(dataIndex);
    if (!company) return;
    
    if (QMessageBox::question(this, "Подтверждение", 
        "Вы уверены, что хотите удалить эту компанию?") == QMessageBox::Yes) {
        transportCompanies_.remove(dataIndex);
        updateTransportCompaniesTable();
        applyTransportFilters();
        statusBar()->showMessage("Компания удалена", 2000);
    }
}

void MainWindow::showTransportCompanyInfo() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedTransportIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите компанию для просмотра информации");
        return;
    }
    
    TransportCompany* company = transportCompanies_.get(dataIndex);
    if (!company) return;
    
    QString schedulesInfo;
    if (company->getScheduleCount() > 0) {
        schedulesInfo = "\nРасписание:\n";
        int scheduleNum = 1;
        for (const auto& schedule : company->getSchedules()) {
            schedulesInfo += QString("  %1. Отправление: %2 (%3) -> Прибытие: %4 (%5)\n")
                        .arg(scheduleNum++)
                        .arg(schedule.departureCity)
                        .arg(schedule.departureDate.toString("yyyy-MM-dd"))
                        .arg(schedule.arrivalCity)
                        .arg(schedule.arrivalDate.toString("yyyy-MM-dd"));
        }
    } else {
        schedulesInfo = "\nРасписание: нет рейсов";
    }
    
    QString info = QString("Информация о транспортной компании:\n\n"
                          "Название: %1\n"
                          "Тип транспорта: %2\n"
                          "Количество рейсов: %3%4")
                   .arg(company->getName())
                   .arg(TransportCompany::transportTypeToString(company->getTransportType()))
                   .arg(company->getScheduleCount())
                   .arg(schedulesInfo);
    
    QMessageBox::information(this, "Информация о компании", info);
}

void MainWindow::refreshTransportCompanies() {
    updateTransportCompaniesTable();
}

void MainWindow::addTour() {
    TourDialog dialog(this, &countries_, &hotels_, &transportCompanies_);
    if (dialog.exec() == QDialog::Accepted) {
        Tour tour = dialog.getTour();
        tours_.add(tour);
        updateToursTable();
        updateToursFilterCombo();
        applyToursFilters();
        statusBar()->showMessage("Тур добавлен", 2000);
    }
}

void MainWindow::editTour() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedTourIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите тур для редактирования");
        return;
    }
    
    Tour* tour = tours_.get(dataIndex);
    if (!tour) {
        QMessageBox::warning(this, "Ошибка", "Тур не найден");
        updateToursTable();
        return;
    }
    
    TourDialog dialog(this, &countries_, &hotels_, &transportCompanies_, tour);
    if (dialog.exec() == QDialog::Accepted) {
        Tour newTour = dialog.getTour();
        
        if (newTour.getName().isEmpty() || newTour.getCountry().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Тур должен иметь название и страну");
            return;
        }
        
        *tour = newTour;
        updateToursTable();
        updateToursFilterCombo();
        linkToursWithHotelsAndTransport();
        applyToursFilters();
        statusBar()->showMessage("Тур обновлен", 2000);
    }
}

void MainWindow::deleteTour() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedTourIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите тур для удаления");
        return;
    }
    
    Tour* tour = tours_.get(dataIndex);
    if (!tour) return;
    
    if (QMessageBox::question(this, "Подтверждение", 
        "Вы уверены, что хотите удалить этот тур?") == QMessageBox::Yes) {
        tours_.remove(dataIndex);
        updateToursTable();
        updateToursFilterCombo();
        linkToursWithHotelsAndTransport();
        applyToursFilters();
        statusBar()->showMessage("Тур удален", 2000);
    }
}

void MainWindow::showTourInfo() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedTourIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите тур для просмотра информации");
        return;
    }
    
    Tour* tour = tours_.get(dataIndex);
    if (!tour) return;
    
    QString info = QString("Информация о туре:\n\n"
                          "Название: %1\n"
                          "Страна: %2\n"
                          "Дата начала: %3\n"
                          "Дата окончания: %4\n"
                          "Отель: %5\n"
                          "Транспорт: %6\n"
                          "Стоимость: %7 руб")
                   .arg(tour->getName())
                   .arg(tour->getCountry())
                   .arg(tour->getStartDate().isValid() ? tour->getStartDate().toString("yyyy-MM-dd") : "не указана")
                   .arg(tour->getEndDate().isValid() ? tour->getEndDate().toString("yyyy-MM-dd") : "не указана")
                   .arg(tour->getHotel().getName().isEmpty() ? "не выбран" : tour->getHotel().getName())
                   .arg(tour->getTransportCompany().getName().isEmpty() ? "не выбран" : tour->getTransportCompany().getName())
                   .arg(tour->calculateCost(), 0, 'f', 2);
    
    QMessageBox::information(this, "Информация о туре", info);
}

void MainWindow::refreshTours() {
    updateToursTable();
}

void MainWindow::searchTours() {
    SearchDialog dialog(this, &tours_);
    dialog.exec();
}

void MainWindow::addOrder() {
    if (countries_.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Нет доступных стран для бронирования");
        return;
    }
    if (hotels_.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Нет доступных отелей для бронирования");
        return;
    }
    if (transportCompanies_.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Нет доступных транспортных компаний");
        return;
    }
    
    BookTourDialog dialog(this, &countries_, &hotels_, &transportCompanies_, &tours_);
    if (dialog.exec() == QDialog::Accepted) {
        Order order = dialog.getOrder();
        orders_.add(order);
        updateOrdersTable();
        applyOrdersFilters();
        statusBar()->showMessage("Заказ создан", 2000);
    }
}

void MainWindow::processOrder() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedOrderIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите заказ для обработки");
        return;
    }
    
    Order* order = orders_.get(dataIndex);
    if (!order) return;
    
    QString currentStatus = order->getStatus();
    
    QStringList statuses = {"В обработке", "Подтвержден", "Оплачен", "Завершен", "Отменен"};
    
    bool ok;
    QString newStatus = QInputDialog::getItem(this, "Изменение статуса заказа",
        QString("Выберите новый статус для заказа #%1\nТекущий статус: %2")
            .arg(order->getId()).arg(currentStatus),
        statuses, statuses.indexOf(currentStatus), false, &ok);
    
    if (ok && !newStatus.isEmpty() && newStatus != currentStatus) {
        order->setStatus(newStatus);
        updateOrdersTable();
        applyOrdersFilters();
        
        try {
            QString dataPath = "data";
            QStringList possiblePaths;
            
            QDir appDir(QCoreApplication::applicationDirPath());
            possiblePaths << appDir.absoluteFilePath("data");
            possiblePaths << QDir("data").absolutePath();
            
            QDir parentDir = appDir;
            if (parentDir.cdUp()) {
                possiblePaths << parentDir.absoluteFilePath("data");
            }
            
            bool found = false;
            for (const QString& path : possiblePaths) {
                if (QDir(path).exists()) {
                    dataPath = path;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                dataPath = appDir.absoluteFilePath("data");
                QDir().mkpath(dataPath);
            }
            
            fileManager_.saveOrders(orders_, dataPath + "/orders.txt");
        } catch (const FileException& e) {
            qWarning() << "Failed to auto-save orders after status change:" << e.what();
        }
        
        statusBar()->showMessage(QString("Статус заказа #%1 изменен на '%2' (сохранено)").arg(order->getId()).arg(newStatus), 2000);
    }
}

void MainWindow::editOrder() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedOrderIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите заказ для редактирования");
        return;
    }
    
    Order* order = orders_.get(dataIndex);
    if (!order) return;
    
    BookTourDialog dialog(this, &countries_, &hotels_, &transportCompanies_, &tours_);
    
    dialog.setOrder(*order);
    
    if (dialog.exec() == QDialog::Accepted) {
        Order newOrder = dialog.getOrder();
        
        order->setTour(newOrder.getTour());
        order->setClientName(newOrder.getClientName());
        order->setClientPhone(newOrder.getClientPhone());
        
        linkOrdersToursWithHotelsAndTransport();
        updateOrdersTable();
        applyOrdersFilters();
        statusBar()->showMessage("Заказ обновлен", 2000);
    }
}

void MainWindow::deleteOrder() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedOrderIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите заказ для удаления");
        return;
    }
    
    Order* order = orders_.get(dataIndex);
    if (!order) return;
    
    if (QMessageBox::question(this, "Подтверждение", 
        "Вы уверены, что хотите удалить этот заказ?") == QMessageBox::Yes) {
        orders_.remove(dataIndex);
        updateOrdersTable();
        applyOrdersFilters();
        statusBar()->showMessage("Заказ удален", 2000);
    }
}

void MainWindow::showOrderInfo() {
    int dataIndex = -1;
    
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        dataIndex = button->property("dataIndex").toInt();
    } else {
        dataIndex = getSelectedOrderIndex();
    }
    
    if (dataIndex < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите заказ для просмотра информации");
        return;
    }
    
    Order* order = orders_.get(dataIndex);
    if (!order) return;
    
    QString info = QString("Информация о заказе:\n\n"
                          "ID: %1\n"
                          "Тур: %2\n"
                          "Страна: %3\n"
                          "Клиент: %4\n"
                          "Телефон: %5\n"
                          "Стоимость: %6 руб\n"
                          "Статус: %7")
                   .arg(order->getId())
                   .arg(order->getTour().getName())
                   .arg(order->getTour().getCountry())
                   .arg(order->getClientName())
                   .arg(order->getClientPhone())
                   .arg(order->getTotalCost(), 0, 'f', 2)
                   .arg(order->getStatus());
    
    QMessageBox::information(this, "Информация о заказе", info);
}

void MainWindow::refreshOrders() {
    updateOrdersTable();
}

void MainWindow::saveData() {
    try {
        QString dataPath = "data";
        
        QStringList possiblePaths;
        
        possiblePaths << QDir("data").absolutePath();
        
        QDir appDir(QCoreApplication::applicationDirPath());
        possiblePaths << appDir.absoluteFilePath("data");
        
        QDir parentDir = appDir;
        if (parentDir.cdUp()) {
            possiblePaths << parentDir.absoluteFilePath("data");
        }
        
        bool found = false;
        for (const QString& path : possiblePaths) {
            if (QDir(path).exists()) {
                dataPath = path;
                found = true;
                break;
                }
            }
            
            if (!found) {
                dataPath = appDir.absoluteFilePath("data");
                QDir().mkpath(dataPath);
            }
        
        QString absoluteDataPath = QDir(dataPath).absolutePath();
        
        QString ordersSavePath = absoluteDataPath + "/orders.txt";
        
        fileManager_.saveCountries(countries_, absoluteDataPath + "/countries.txt");
        fileManager_.saveHotels(hotels_, absoluteDataPath + "/hotels.txt");
        fileManager_.saveTransportCompanies(transportCompanies_, absoluteDataPath + "/transport_companies.txt");
        fileManager_.saveTours(tours_, absoluteDataPath + "/tours.txt");
        fileManager_.saveOrders(orders_, ordersSavePath);
        
        statusBar()->showMessage("Данные сохранены", 2000);
        QMessageBox::information(this, "Успех", 
            QString("Данные успешно сохранены в:\n%1\n\nЗаказов: %2")
                .arg(QDir(dataPath).absolutePath())
                .arg(orders_.size()));
    } catch (const FileException& e) {
        QMessageBox::critical(this, "Ошибка",
                               QString("Не удалось сохранить данные: %1").arg(e.what()));
    }
}

QString MainWindow::findDataDirectory() const {
    QStringList possiblePaths;
    
    possiblePaths << QDir("data").absolutePath();
    
    QDir appDir(QCoreApplication::applicationDirPath());
    possiblePaths << appDir.absoluteFilePath("data");
    
    QDir parentDir = appDir;
    if (parentDir.cdUp()) {
        possiblePaths << parentDir.absoluteFilePath("data");
    }
    
    QDir projectRoot = appDir;
    int maxLevels = 5;
    bool foundProjectRoot = false;
    for (int i = 0; i < maxLevels && !projectRoot.isRoot() && !foundProjectRoot; ++i) {
        QString projectDataPath = projectRoot.absoluteFilePath("data");
        if (!possiblePaths.contains(projectDataPath)) {
            possiblePaths << projectDataPath;
        }
        if (QFile::exists(projectRoot.absoluteFilePath("CMakeLists.txt"))) {
            foundProjectRoot = true;
        } else if (!projectRoot.cdUp()) {
            foundProjectRoot = true;
        }
    }
    
    for (const QString& path : possiblePaths) {
        if (QDir(path).exists()) {
            return path;
        }
    }
    
    QString defaultPath = appDir.absoluteFilePath("data");
    return defaultPath;
}

bool MainWindow::validateDataDirectory(const QString& dataPath) {
    QDir dataDir(dataPath);
    
    if (!dataDir.exists()) {
        QString message = QString("Папка 'data' не найдена.\n\n"
                   "Искали в:\n"
                   "- %1\n"
                   "- %2\n\n"
                   "Убедитесь, что файлы данных находятся в папке 'data' рядом с исполняемым файлом.")
                .arg(QDir("data").absolutePath())
                .arg(QDir(QCoreApplication::applicationDirPath() + "/data").absolutePath());
        QMessageBox::warning(this, "Предупреждение", message);
        return false;
    }
    return true;
}

QStringList MainWindow::checkRequiredFiles(const QString& dataPath) {
    QStringList requiredFiles = {
        dataPath + "/countries.txt",
        dataPath + "/hotels.txt",
        dataPath + "/transport_companies.txt",
        dataPath + "/tours.txt",
        dataPath + "/orders.txt"
    };
    
    for (const QString& file : requiredFiles) {
        bool exists = QFile::exists(file);
        if (exists) {
            QFileInfo info(file);
        }
    }
    
    QStringList missingFiles;
    for (const QString& file : requiredFiles) {
        if (!QFile::exists(file)) {
            missingFiles << QFileInfo(file).fileName();
        }
    }
    
    if (!missingFiles.isEmpty()) {
        QString message = QString("Не найдены следующие файлы в папке '%1':\n%2\n\nУбедитесь, что все файлы данных находятся в папке 'data'.")
                .arg(dataPath)
                .arg(missingFiles.join("\n"));
        QMessageBox::warning(this, "Предупреждение", message);
    }
    
    return missingFiles;
}

void MainWindow::clearAllData() {
    countries_.clear();
    hotels_.clear();
    transportCompanies_.clear();
    tours_.clear();
    orders_.clear();
}

MainWindow::LoadResult MainWindow::loadAllDataFiles(const QString& dataPath) {
    LoadResult result;
    
    try {
        fileManager_.loadCountries(countries_, dataPath + "/countries.txt");
        result.countries = countries_.size();
    } catch (const FileException& e) {
        result.errors << QString("Страны: %1").arg(e.what());
    }
    
    try {
        fileManager_.loadHotels(hotels_, dataPath + "/hotels.txt");
        result.hotels = hotels_.size();
    } catch (const FileException& e) {
        result.errors << QString("Отели: %1").arg(e.what());
    }
    
    try {
        fileManager_.loadTransportCompanies(transportCompanies_, dataPath + "/transport_companies.txt");
        result.companies = transportCompanies_.size();
    } catch (const FileException& e) {
        result.errors << QString("Транспортные компании: %1").arg(e.what());
    }
    
    try {
        fileManager_.loadTours(tours_, dataPath + "/tours.txt");
        result.tours = tours_.size();
        
        linkToursWithHotelsAndTransport();
    } catch (const FileException& e) {
        result.errors << QString("Туры: %1").arg(e.what());
    }
    
    try {
        QString ordersPath = dataPath + "/orders.txt";
        QString absoluteOrdersPath = QDir(ordersPath).absolutePath();
        fileManager_.loadOrders(orders_, absoluteOrdersPath);
        result.orders = orders_.size();
        linkOrdersToursWithHotelsAndTransport();
    } catch (const FileException& e) {
        result.errors << QString("Заказы: %1").arg(e.what());
    }
    
    return result;
}

void MainWindow::showLoadResults(const LoadResult& result, const QString& dataPath) {
    updateCountriesTable();
    updateHotelsTable();
    updateTransportCompaniesTable();
    updateToursTable();
    updateOrdersTable();
    
    updateCountriesFilterCombo();
    updateHotelsFilterCombos();
    updateTransportFilterCombo();
    updateToursFilterCombo();
    updateOrdersFilterCombo();
    
    int totalItems = result.countries + result.hotels + result.companies + result.tours + result.orders;
    
    if (result.errors.isEmpty() && totalItems > 0) {
        statusBar()->showMessage(QString("Данные загружены: %1 записей").arg(totalItems), 3000);
        QMessageBox::information(this, "Успех", 
            QString("Данные успешно загружены из '%1':\n\n"
                   "Страны: %2\n"
                   "Отели: %3\n"
                   "Транспортные компании: %4\n"
                   "Туры: %5\n"
                   "Заказы: %6")
                .arg(dataPath)
                .arg(result.countries)
                .arg(result.hotels)
                .arg(result.companies)
                .arg(result.tours)
                .arg(result.orders));
    } else if (!result.errors.isEmpty()) {
        statusBar()->showMessage("Ошибки при загрузке данных", 3000);
        QString message = QString("При загрузке данных возникли ошибки:\n\n%1\n\n"
                   "Успешно загружено:\n"
                   "Страны: %2, Отели: %3, Компании: %4, Туры: %5, Заказы: %6")
                .arg(result.errors.join("\n"))
                .arg(result.countries)
                .arg(result.hotels)
                .arg(result.companies)
                .arg(result.tours)
                .arg(result.orders);
        QMessageBox::warning(this, "Ошибки загрузки", message);
    } else {
        statusBar()->showMessage("Данные не загружены (файлы пусты)", 3000);
        QString message = QString("Файлы найдены в '%1', но все файлы пусты или не содержат данных.")
                .arg(dataPath);
        QMessageBox::warning(this, "Предупреждение", message);
    }
}

void MainWindow::loadData() {
    try {
        QString dataPath = findDataDirectory();
        
        if (!validateDataDirectory(dataPath)) {
            return;
        }
        
        QStringList missingFiles = checkRequiredFiles(dataPath);
        if (!missingFiles.isEmpty()) {
            return;
        }
        
        clearAllData();
        LoadResult result = loadAllDataFiles(dataPath);
        showLoadResults(result, dataPath);
    } catch (const FileException& e) {
        QMessageBox::critical(this, "Ошибка загрузки", 
            QString("Не удалось загрузить данные:\n%1").arg(e.what()));
        statusBar()->showMessage("Ошибка при загрузке данных", 3000);
    }
}

void MainWindow::onTabChanged([[maybe_unused]] int index) {
}

void MainWindow::updateCountriesFilterCombo() {
    ui->filterCountryCombo->clear();
    ui->filterCountryCombo->addItem("Все");
    
    QSet<QString> continents;
    for (const auto& country : countries_.getData()) {
        if (!country.getContinent().isEmpty()) {
            continents.insert(country.getContinent());
        }
    }
    
    QStringList sortedContinents = continents.values();
    sortedContinents.sort();
    for (const QString& continent : sortedContinents) {
        ui->filterCountryCombo->addItem(continent);
    }
    
    ui->filterCountryCurrencyCombo->clear();
    ui->filterCountryCurrencyCombo->addItem("Все");
    
    QSet<QString> currencies;
    for (const auto& country : countries_.getData()) {
        if (!country.getCurrency().isEmpty()) {
            currencies.insert(country.getCurrency());
        }
    }
    
    QStringList sortedCurrencies = currencies.values();
    sortedCurrencies.sort();
    for (const QString& currency : sortedCurrencies) {
        ui->filterCountryCurrencyCombo->addItem(currency);
    }
}

void MainWindow::updateHotelsFilterCombos() {
    ui->filterHotelCountryCombo->clear();
    ui->filterHotelCountryCombo->addItem("Все");
    
    QSet<QString> countries;
    for (const auto& hotel : hotels_.getData()) {
        if (!hotel.getCountry().isEmpty()) {
            countries.insert(hotel.getCountry());
        }
    }
    
    QStringList sortedCountries = countries.values();
    sortedCountries.sort();
    for (const QString& country : sortedCountries) {
        ui->filterHotelCountryCombo->addItem(country);
    }
    
    ui->filterHotelStarsCombo->clear();
    ui->filterHotelStarsCombo->addItem("Все");
    
    QSet<int> stars;
    for (const auto& hotel : hotels_.getData()) {
        stars.insert(hotel.getStars());
    }
    
    QList<int> sortedStars = stars.values();
    std::sort(sortedStars.begin(), sortedStars.end());
    for (int star : sortedStars) {
        ui->filterHotelStarsCombo->addItem(QString::number(star));
    }
}

void MainWindow::updateTransportFilterCombo() {
    ui->filterTransportTypeCombo->clear();
    ui->filterTransportTypeCombo->addItem("Все");
    
    QSet<QString> transportTypes;
    for (const auto& company : transportCompanies_.getData()) {
        QString typeStr = TransportCompany::transportTypeToString(company.getTransportType());
        if (!typeStr.isEmpty()) {
            transportTypes.insert(typeStr);
        }
    }
    
    QStringList sortedTypes = transportTypes.values();
    sortedTypes.sort();
    for (const QString& type : sortedTypes) {
        ui->filterTransportTypeCombo->addItem(type);
    }
}

void MainWindow::updateToursFilterCombo() {
    ui->filterTourCountryCombo->clear();
    ui->filterTourCountryCombo->addItem("Все");
    
    QSet<QString> countries;
    for (const auto& tour : tours_.getData()) {
        if (!tour.getCountry().isEmpty()) {
            countries.insert(tour.getCountry());
        }
    }
    
    QStringList sortedCountries = countries.values();
    sortedCountries.sort();
    for (const QString& country : sortedCountries) {
        ui->filterTourCountryCombo->addItem(country);
    }
}

void MainWindow::updateOrdersFilterCombo() {
    ui->filterOrderStatusCombo->clear();
    ui->filterOrderStatusCombo->addItem("Все");
    ui->filterOrderStatusCombo->addItem("В обработке");
    ui->filterOrderStatusCombo->addItem("Подтвержден");
    ui->filterOrderStatusCombo->addItem("Оплачен");
    ui->filterOrderStatusCombo->addItem("Завершен");
    ui->filterOrderStatusCombo->addItem("Отменен");
}

bool MainWindow::matchesSearchInTable(QTableWidget* table, int row, const QString& searchText, int excludeColumn) const {
    if (searchText.isEmpty()) {
        return true;
    }
    
    int columnCount = table->columnCount();
    for (int col = 0; col < columnCount; ++col) {
        if (col == excludeColumn) {
            continue;
        }
        QTableWidgetItem* item = table->item(row, col);
        if (item) {
            QString itemText = item->text().toLower();
            if (itemText.contains(" руб")) {
                itemText.remove(" руб");
            }
            if (itemText.contains(searchText)) {
                return true;
            }
        }
    }
    return false;
}

double MainWindow::extractCostFromTableItem(QTableWidget* table, int row, int column) const {
    QTableWidgetItem* costItem = table->item(row, column);
    if (!costItem) {
        return 0.0;
    }
    
    QVariant sortData = costItem->data(Qt::UserRole);
    if (sortData.isValid()) {
        return sortData.toDouble();
    }
    
    QString costText = costItem->text();
    costText.remove(" руб");
    return costText.toDouble();
}

void MainWindow::applyCountriesFilters() {
    QString searchText = ui->searchCountryEdit->text().toLower();
    QString filterContinent = ui->filterCountryCombo->currentText();
    QString filterCurrency = ui->filterCountryCurrencyCombo->currentText();
    
    for (int row = 0; row < ui->countriesTable->rowCount(); ++row) {
        bool visible = matchesSearchInTable(ui->countriesTable, row, searchText);
        
        if (visible && filterContinent != "Все") {
            QTableWidgetItem* continentItem = ui->countriesTable->item(row, 1);
            if (!continentItem || continentItem->text() != filterContinent) {
                visible = false;
            }
        }
        
        if (visible && filterCurrency != "Все") {
            QTableWidgetItem* currencyItem = ui->countriesTable->item(row, 3);
            if (!currencyItem || currencyItem->text() != filterCurrency) {
                visible = false;
            }
        }
        
        ui->countriesTable->setRowHidden(row, !visible);
    }
}

void MainWindow::applyHotelsFilters() {
    QString searchText = ui->searchHotelEdit->text().toLower();
    QString filterCountry = ui->filterHotelCountryCombo->currentText();
    QString filterStars = ui->filterHotelStarsCombo->currentText();
    
    for (int row = 0; row < ui->hotelsTable->rowCount(); ++row) {
        bool visible = matchesSearchInTable(ui->hotelsTable, row, searchText);
        
        if (visible && filterCountry != "Все") {
            QTableWidgetItem* countryItem = ui->hotelsTable->item(row, 1);
            if (!countryItem || countryItem->text() != filterCountry) {
                visible = false;
            }
        }
        
        if (visible && filterStars != "Все") {
            QTableWidgetItem* starsItem = ui->hotelsTable->item(row, 2);
            if (!starsItem || starsItem->text() != filterStars) {
                visible = false;
            }
        }
        
        ui->hotelsTable->setRowHidden(row, !visible);
    }
}

void MainWindow::applyTransportFilters() {
    QString searchText = ui->searchTransportEdit->text().toLower();
    QString filterType = ui->filterTransportTypeCombo->currentText();
    
    for (int row = 0; row < ui->transportTable->rowCount(); ++row) {
        bool visible = matchesSearchInTable(ui->transportTable, row, searchText);
        
        if (visible && filterType != "Все") {
            QTableWidgetItem* typeItem = ui->transportTable->item(row, 1);
            if (!typeItem || typeItem->text() != filterType) {
                visible = false;
            }
        }
        
        ui->transportTable->setRowHidden(row, !visible);
    }
}

void MainWindow::applyToursFilters() {
    QString searchText = ui->searchTourEdit->text().toLower();
    QString filterCountry = ui->filterTourCountryCombo->currentText();
    QString minPriceText = ui->filterTourMinPriceEdit->text();
    QString maxPriceText = ui->filterTourMaxPriceEdit->text();
    
    bool hasMinPrice = !minPriceText.isEmpty();
    bool hasMaxPrice = !maxPriceText.isEmpty();
    double minPrice = hasMinPrice ? minPriceText.toDouble() : 0.0;
    double maxPrice = hasMaxPrice ? maxPriceText.toDouble() : std::numeric_limits<double>::max();
    
    for (int row = 0; row < ui->toursTable->rowCount(); ++row) {
        bool visible = matchesSearchInTable(ui->toursTable, row, searchText);
        
        if (visible && filterCountry != "Все") {
            QTableWidgetItem* countryItem = ui->toursTable->item(row, 1);
            if (!countryItem || countryItem->text() != filterCountry) {
                visible = false;
            }
        }
        
        if (visible && (hasMinPrice || hasMaxPrice)) {
            double cost = extractCostFromTableItem(ui->toursTable, row, 4);
            if (cost < minPrice || cost > maxPrice) {
                visible = false;
            }
        }
        
        ui->toursTable->setRowHidden(row, !visible);
    }
}

void MainWindow::applyOrdersFilters() {
    QString searchText = ui->searchOrderEdit->text().toLower();
    QString filterStatus = ui->filterOrderStatusCombo->currentText();
    QString minCostText = ui->filterOrderMinCostEdit->text();
    QString maxCostText = ui->filterOrderMaxCostEdit->text();
    
    bool hasMinCost = !minCostText.isEmpty();
    bool hasMaxCost = !maxCostText.isEmpty();
    double minCost = hasMinCost ? minCostText.toDouble() : 0.0;
    double maxCost = hasMaxCost ? maxCostText.toDouble() : std::numeric_limits<double>::max();
    
    int actionsColumn = ui->ordersTable->columnCount() - 1;
    for (int row = 0; row < ui->ordersTable->rowCount(); ++row) {
        bool visible = matchesSearchInTable(ui->ordersTable, row, searchText, actionsColumn);
        
        if (visible && filterStatus != "Все") {
            QTableWidgetItem* statusItem = ui->ordersTable->item(row, 5);
            if (!statusItem || statusItem->text() != filterStatus) {
                visible = false;
            }
        }
        
        if (visible && (hasMinCost || hasMaxCost)) {
            double cost = extractCostFromTableItem(ui->ordersTable, row, 4);
            if (cost < minCost || cost > maxCost) {
                visible = false;
            }
        }
        
        ui->ordersTable->setRowHidden(row, !visible);
    }
}

QString MainWindow::findCountryCapital(const QString& countryName) const {
    for (const auto& country : countries_.getData()) {
        if (country.getName() == countryName) {
            return country.getCapital();
        }
    }
    return "";
}

QSet<QString> MainWindow::collectTargetCities(const QString& tourCountry, const QString& capital) const {
    QSet<QString> targetCities;
    
    if (!capital.isEmpty()) {
        targetCities.insert(capital);
    }
    
    for (const auto& hotel : hotels_.getData()) {
        if (hotel.getCountry() != tourCountry || hotel.getRoomCount() == 0) {
            continue;
        }
        
        QString address = hotel.getAddress();
        if (address.isEmpty()) {
            continue;
        }
        
        QString city = address.split(',').first().trimmed();
        if (!city.isEmpty()) {
            targetCities.insert(city);
        }
    }
    
    return targetCities;
}

Hotel* MainWindow::findHotelForTour(const QString& tourCountry) {
    for (auto& hotel : hotels_.getData()) {
        if (hotel.getCountry() == tourCountry && hotel.getRoomCount() > 0) {
            return &hotel;
        }
    }
    return nullptr;
}

bool MainWindow::matchesCity(const QString& arrivalCity, const QSet<QString>& targetCities, 
                            const QString& capital, const QString& tourCountry) const {
    QString arrivalCityLower = arrivalCity.toLower();
    
    if (!targetCities.isEmpty()) {
        for (const QString& targetCity : targetCities) {
            QString targetCityLower = targetCity.toLower();
            if (arrivalCity == targetCity || 
                arrivalCityLower == targetCityLower ||
                arrivalCityLower.contains(targetCityLower) ||
                targetCityLower.contains(arrivalCityLower)) {
                return true;
            }
        }
    }
    
    if (!capital.isEmpty()) {
        QString capitalLower = capital.toLower();
        if (arrivalCityLower == capitalLower ||
            arrivalCityLower.contains(capitalLower) || 
            capitalLower.contains(arrivalCityLower)) {
            return true;
        }
    }
    
    if (tourCountry == "ОАЭ") {
        if (arrivalCityLower.contains("дубай") || arrivalCityLower.contains("абу-даби") || 
            arrivalCityLower.contains("abu dhabi") || arrivalCityLower.contains("dubai")) {
            return true;
        }
    }
    
    return false;
}

bool MainWindow::findTransportForTour(Tour& tour, const QSet<QString>& targetCities, 
                                       const QString& capital, const QDate& tourStartDate) {
    for (auto& company : transportCompanies_.getData()) {
        for (int i = 0; i < company.getScheduleCount(); ++i) {
            TransportSchedule* schedule = company.getSchedule(i);
            if (!schedule) {
                continue;
            }
            
            QString arrivalCity = schedule->arrivalCity;
            QDate scheduleDepartureDate = schedule->departureDate;
            
            if (!matchesCity(arrivalCity, targetCities, capital, tour.getCountry())) {
                continue;
            }
            
            bool dateMatches = !tourStartDate.isValid() || 
                              !scheduleDepartureDate.isValid() ||
                              scheduleDepartureDate <= tourStartDate;
            
            if (dateMatches || !tourStartDate.isValid()) {
                tour.setTransportCompany(company);
                tour.setTransportSchedule(*schedule);
                return true;
            }
        }
    }
    return false;
}

void MainWindow::linkToursWithHotelsAndTransport() {
    for (auto& tour : tours_.getData()) {
        QString tourCountry = tour.getCountry();
        QDate tourStartDate = tour.getStartDate();
        
        QString capital = findCountryCapital(tourCountry);
        QSet<QString> targetCities = collectTargetCities(tourCountry, capital);
        
        Hotel* selectedHotel = findHotelForTour(tourCountry);
        if (selectedHotel) {
            tour.setHotel(*selectedHotel);
        }
        
        findTransportForTour(tour, targetCities, capital, tourStartDate);
    }
}

void MainWindow::linkOrdersToursWithHotelsAndTransport() {
    for (auto& order : orders_.getData()) {
        Tour tourInOrder = order.getTour();
        QString tourName = tourInOrder.getName();
        QString tourCountry = tourInOrder.getCountry();
        
        for (const auto& fullTour : tours_.getData()) {
            if (fullTour.getName() == tourName && fullTour.getCountry() == tourCountry) {
                order.setTour(fullTour);
                break;
            }
        }
    }
}

void MainWindow::onCountriesHeaderClicked(int logicalIndex) {
    if (logicalIndex == 4) return;
    
    static QMap<int, int> columnStates;
    
    int currentState = columnStates.value(logicalIndex, 0);
    
    if (currentState == 0) {
        columnStates[logicalIndex] = 1;
        ui->countriesTable->setSortingEnabled(false);
        ui->countriesTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->countriesTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->countriesTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        columnStates[logicalIndex] = 2;
        ui->countriesTable->setSortingEnabled(false);
        ui->countriesTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->countriesTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->countriesTable->setSortingEnabled(true);
    } else {
        columnStates[logicalIndex] = 0;
        
        ui->countriesTable->setSortingEnabled(false);
        updateCountriesTable();
        ui->countriesTable->setSortingEnabled(true);
        
        ui->countriesTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        columnStates.clear();
    }
}

void MainWindow::onHotelsHeaderClicked(int logicalIndex) {
    if (logicalIndex == 5) return;
    
    static QMap<int, int> columnStates;
    
    int currentState = columnStates.value(logicalIndex, 0);
    
    if (currentState == 0) {
        columnStates[logicalIndex] = 1;
        ui->hotelsTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->hotelsTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
    } else if (currentState == 1) {
        columnStates[logicalIndex] = 2;
        ui->hotelsTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->hotelsTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
    } else {
        columnStates[logicalIndex] = 0;
        
        updateHotelsTable();
        
        ui->hotelsTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        columnStates.clear();
    }
}

void MainWindow::onTransportHeaderClicked(int logicalIndex) {
    if (logicalIndex == 5) return;
    
    static QMap<int, int> columnStates;
    
    int currentState = columnStates.value(logicalIndex, 0);
    
    if (currentState == 0) {
        columnStates[logicalIndex] = 1;
        ui->transportTable->setSortingEnabled(false);
        ui->transportTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->transportTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->transportTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        columnStates[logicalIndex] = 2;
        ui->transportTable->setSortingEnabled(false);
        ui->transportTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->transportTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->transportTable->setSortingEnabled(true);
    } else {
        columnStates[logicalIndex] = 0;
        
        ui->transportTable->setSortingEnabled(false);
        updateTransportCompaniesTable();
        ui->transportTable->setSortingEnabled(true);
        
        ui->transportTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        columnStates.clear();
    }
}

void MainWindow::onToursHeaderClicked(int logicalIndex) {
    if (logicalIndex == 5) return;
    
    static QMap<int, int> columnStates;
    
    int currentState = columnStates.value(logicalIndex, 0);
    
    if (currentState == 0) {
        columnStates[logicalIndex] = 1;
        ui->toursTable->setSortingEnabled(false);
        ui->toursTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->toursTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->toursTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        columnStates[logicalIndex] = 2;
        ui->toursTable->setSortingEnabled(false);
        ui->toursTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->toursTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->toursTable->setSortingEnabled(true);
    } else {
        columnStates[logicalIndex] = 0;
        
        ui->toursTable->setSortingEnabled(false);
        updateToursTable();
        ui->toursTable->setSortingEnabled(true);
        
        ui->toursTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        columnStates.clear();
    }
}

void MainWindow::onOrdersHeaderClicked(int logicalIndex) {
    if (logicalIndex == 6) return;
    
    static QMap<int, int> columnStates;
    
    int currentState = columnStates.value(logicalIndex, 0);
    
    if (currentState == 0) {
        columnStates[logicalIndex] = 1;
        ui->ordersTable->setSortingEnabled(false);
        ui->ordersTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->ordersTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->ordersTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        columnStates[logicalIndex] = 2;
        ui->ordersTable->setSortingEnabled(false);
        ui->ordersTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->ordersTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->ordersTable->setSortingEnabled(true);
    } else {
        columnStates[logicalIndex] = 0;
        
        ui->ordersTable->setSortingEnabled(false);
        updateOrdersTable();
        ui->ordersTable->setSortingEnabled(true);
        
        ui->ordersTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        columnStates.clear();
    }
}

void MainWindow::updateCurrencyRates() {
    QUrl url("https://api.exchangerate-api.com/v4/latest/BYN");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    networkManager_->get(request);
}

void MainWindow::onCurrencyDataReceived(QNetworkReply* reply) {
    QLabel* usdLabel = findChild<QLabel*>("usdLabel");
    QLabel* eurLabel = findChild<QLabel*>("eurLabel");
    QLabel* updateLabel = findChild<QLabel*>("currencyUpdateLabel");
    
    if (!usdLabel || !eurLabel || !updateLabel) {
        reply->deleteLater();
        return;
    }
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            QJsonObject rates = obj["rates"].toObject();
            
            if (rates.contains("USD")) {
                double bynToUsd = rates["USD"].toDouble();
                double usdToByn = 1.0 / bynToUsd;
                usdLabel->setText(QString("1 USD = %1 BYN").arg(usdToByn, 0, 'f', 2));
            } else {
                usdLabel->setText("USD: не найден");
            }
            
            if (rates.contains("EUR")) {
                double bynToEur = rates["EUR"].toDouble();
                double eurToByn = 1.0 / bynToEur;
                eurLabel->setText(QString("1 EUR = %1 BYN").arg(eurToByn, 0, 'f', 2));
            } else {
                eurLabel->setText("EUR: не найден");
            }
            
            QDateTime now = QDateTime::currentDateTime();
            updateLabel->setText(
                QString("(обновлено %1)").arg(now.toString("HH:mm"))
            );
        } else {
            usdLabel->setText("USD: ошибка формата");
            eurLabel->setText("EUR: ошибка формата");
        }
    } else {
        usdLabel->setText("USD: нет связи");
        eurLabel->setText("EUR: нет связи");
        updateLabel->setText("(оффлайн)");
    }
    
    reply->deleteLater();
}

void MainWindow::initializeActions() {
    actions_["addCountry"] = new AddCountryAction(&countries_, ui->countriesTable, this);
    actions_["editCountry"] = new EditCountryAction(&countries_, ui->countriesTable, this);
    actions_["deleteCountry"] = new DeleteCountryAction(&countries_, ui->countriesTable, this);
    actions_["showCountryInfo"] = new ShowCountryInfoAction(&countries_, ui->countriesTable, this);
    actions_["refreshCountries"] = new RefreshCountriesAction();
    
    connect(actions_["addCountry"], &Action::executed, this, [this]() {
        updateCountriesTable();
        filterComboUpdater_->updateCountriesFilterCombo(ui->filterCountryCombo,
                                                         ui->filterCountryCurrencyCombo,
                                                         countries_);
        statusBar()->showMessage("Страна добавлена", 2000);
    });
    connect(actions_["editCountry"], &Action::executed, this, [this]() {
        updateCountriesTable();
        filterComboUpdater_->updateCountriesFilterCombo(ui->filterCountryCombo,
                                                         ui->filterCountryCurrencyCombo,
                                                         countries_);
        filterManager_->applyCountriesFilters(ui->countriesTable, ui->searchCountryEdit,
                                              ui->filterCountryCombo, ui->filterCountryCurrencyCombo);
        statusBar()->showMessage("Страна обновлена", 2000);
    });
    connect(actions_["deleteCountry"], &Action::executed, this, [this]() {
        updateCountriesTable();
        filterComboUpdater_->updateCountriesFilterCombo(ui->filterCountryCombo,
                                                         ui->filterCountryCurrencyCombo,
                                                         countries_);
        filterManager_->applyCountriesFilters(ui->countriesTable, ui->searchCountryEdit,
                                              ui->filterCountryCombo, ui->filterCountryCurrencyCombo);
        statusBar()->showMessage("Страна удалена", 2000);
    });
    connect(actions_["refreshCountries"], &Action::executed, this, [this]() {
        updateCountriesTable();
    });
    
    connect(ui->actionAddCountry, &QAction::triggered, this, [this]() {
        actions_["addCountry"]->execute();
    });
    connect(ui->actionEditCountry, &QAction::triggered, this, [this]() {
        actions_["editCountry"]->execute();
    });
    connect(ui->actionDeleteCountry, &QAction::triggered, this, [this]() {
        actions_["deleteCountry"]->execute();
    });
}
