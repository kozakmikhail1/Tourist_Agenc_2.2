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
    
    // Загружаем данные при запуске
    try {
        loadData();
    } catch (const FileException& e) {
        QString message = QString("Не удалось загрузить данные: %1").arg(e.what());
        QMessageBox::warning(this, "Предупреждение", message);
    }
}

MainWindow::~MainWindow() {
    // Удаляем все действия
    for (Action* action : actions_) {
        delete action;
    }
    actions_.clear();
    
    delete tableManager_;
    delete filterManager_;
    delete filterComboUpdater_;
    // ui автоматически удаляется через std::unique_ptr
}

void MainWindow::setupUI() {
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    // Создаем виджет для курсов валют в углу TabWidget
    QWidget* currencyCornerWidget = new QWidget(this);
    QHBoxLayout* currencyLayout = new QHBoxLayout(currencyCornerWidget);
    currencyLayout->setContentsMargins(0, 0, 10, 0);
    currencyLayout->setSpacing(15);
    
    // Создаем метки для курсов валют
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
    
    // Устанавливаем виджет в правый верхний угол TabWidget
    ui->tabWidget->setCornerWidget(currencyCornerWidget, Qt::TopRightCorner);
    
    // Подключение кнопок "Добавить" на вкладках
    connect(ui->addCountryButton, &QPushButton::clicked, this, &MainWindow::addCountry);
    connect(ui->addHotelButton, &QPushButton::clicked, this, &MainWindow::addHotel);
    connect(ui->addTransportButton, &QPushButton::clicked, this, &MainWindow::addTransportCompany);
    connect(ui->addTourButton, &QPushButton::clicked, this, &MainWindow::addTour);
    connect(ui->addOrderButton, &QPushButton::clicked, this, &MainWindow::addOrder);
    
    // Устанавливаем иконки для кнопок "Добавить"
    ui->addCountryButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addHotelButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addTransportButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addTourButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->addOrderButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    
    // Подключение поиска и фильтрации для
    connect(ui->searchCountryEdit, &QLineEdit::textChanged, this, &MainWindow::applyCountriesFilters);
    connect(ui->filterCountryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyCountriesFilters);
    connect(ui->filterCountryCurrencyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyCountriesFilters);
    
    // Подключение поиска и фильтрации для отелей
    connect(ui->searchHotelEdit, &QLineEdit::textChanged, this, &MainWindow::applyHotelsFilters);
    connect(ui->filterHotelCountryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyHotelsFilters);
    connect(ui->filterHotelStarsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyHotelsFilters);
    
    // Подключение поиска и фильтрации для транспорта
    connect(ui->searchTransportEdit, &QLineEdit::textChanged, this, &MainWindow::applyTransportFilters);
    connect(ui->filterTransportTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyTransportFilters);
    
    // Подключение поиска и фильтрации для туров
    connect(ui->searchTourEdit, &QLineEdit::textChanged, this, &MainWindow::applyToursFilters);
    connect(ui->filterTourCountryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyToursFilters);
    connect(ui->filterTourMinPriceEdit, &QLineEdit::textChanged, this, &MainWindow::applyToursFilters);
    connect(ui->filterTourMaxPriceEdit, &QLineEdit::textChanged, this, &MainWindow::applyToursFilters);
    
    // Подключение поиска и фильтрации для заказов
    connect(ui->searchOrderEdit, &QLineEdit::textChanged, this, &MainWindow::applyOrdersFilters);
    connect(ui->filterOrderStatusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::applyOrdersFilters);
    connect(ui->filterOrderMinCostEdit, &QLineEdit::textChanged, this, &MainWindow::applyOrdersFilters);
    connect(ui->filterOrderMaxCostEdit, &QLineEdit::textChanged, this, &MainWindow::applyOrdersFilters);
}

void MainWindow::setupCurrencyUpdater() {
    // Подключаем обработчик получения данных
    connect(networkManager_, &QNetworkAccessManager::finished, 
            this, &MainWindow::onCurrencyDataReceived);
    
    // Настраиваем таймер для обновления каждую минуту
    connect(currencyTimer_, &QTimer::timeout, this, &MainWindow::updateCurrencyRates);
    currencyTimer_->start(60000); // 1 минута = 60000 мс
    
    // Первое обновление сразу при запуске
    updateCurrencyRates();
}

void MainWindow::setupMenuBar() {
    // Файл
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveData);
    connect(ui->actionLoad, &QAction::triggered, this, &MainWindow::loadData);
    
    // Страны - теперь используются классы-операторы (инициализируются в initializeActions)
    
    // Отели
    connect(ui->actionAddHotel, &QAction::triggered, this, &MainWindow::addHotel);
    connect(ui->actionEditHotel, &QAction::triggered, this, &MainWindow::editHotel);
    connect(ui->actionDeleteHotel, &QAction::triggered, this, &MainWindow::deleteHotel);
    
    // Транспорт
    connect(ui->actionAddTransport, &QAction::triggered, this, &MainWindow::addTransportCompany);
    connect(ui->actionEditTransport, &QAction::triggered, this, &MainWindow::editTransportCompany);
    connect(ui->actionDeleteTransport, &QAction::triggered, this, &MainWindow::deleteTransportCompany);
    
    // Туры
    connect(ui->actionAddTour, &QAction::triggered, this, &MainWindow::addTour);
    connect(ui->actionEditTour, &QAction::triggered, this, &MainWindow::editTour);
    connect(ui->actionDeleteTour, &QAction::triggered, this, &MainWindow::deleteTour);
    connect(ui->actionSearchTours, &QAction::triggered, this, &MainWindow::searchTours);
    
    // Заказы
    connect(ui->actionAddOrder, &QAction::triggered, this, &MainWindow::addOrder);
    connect(ui->actionDeleteOrder, &QAction::triggered, this, &MainWindow::deleteOrder);
    
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage("Готово");
}

void MainWindow::setupTables() {
    // Настройка таблиц
    // Оптимизация производительности для всех таблиц
    auto optimizeTable = [](QTableWidget* table) {
        table->setAlternatingRowColors(false);  // Отключаем чередование цветов для ускорения
        table->setWordWrap(false);  // Отключаем перенос слов для ускорения
        table->setShowGrid(false);  // Отключаем сетку для ускорения отрисовки
        table->setAutoScroll(false);  // Отключаем автоскролл для ускорения
        table->setDragDropMode(QAbstractItemView::NoDragDrop);  // Отключаем drag&drop для ускорения
        // Отключаем анимацию и эффекты для ускорения
        table->setAttribute(Qt::WA_StaticContents, true);
    };
    
    optimizeTable(ui->countriesTable);
    optimizeTable(ui->hotelsTable);
    optimizeTable(ui->transportTable);
    optimizeTable(ui->toursTable);
    optimizeTable(ui->ordersTable);
    
    ui->countriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->countriesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->countriesTable->setSortingEnabled(false);  // Отключаем встроенную сортировку, используем кастомную трёхрежимную
    // Адаптивная ширина столбцов - растягиваются пропорционально доступному пространству
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);  // Название - растягивается
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Континент - растягивается
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);  // Столица - растягивается
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);  // Валюта - растягивается
    ui->countriesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);  // Действия - фиксированная
    ui->countriesTable->setColumnWidth(4, 150);  // Действия - достаточная ширина для увеличенных иконок
    ui->countriesTable->horizontalHeader()->setStretchLastSection(false);  // Отключаем растягивание последнего столбца
    // Отключаем сортировку для колонки действий
    if (ui->countriesTable->horizontalHeaderItem(4)) {
        ui->countriesTable->horizontalHeaderItem(4)->setFlags(Qt::NoItemFlags);
    }
    // Подключаем обработчик для трехрежимной сортировки (Ascending -> Descending -> None)
    ui->countriesTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->countriesTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onCountriesHeaderClicked);
    
    ui->hotelsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->hotelsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->hotelsTable->setSortingEnabled(false);  // Отключаем встроенную сортировку, используем кастомную трёхрежимную
    // Адаптивная ширина столбцов - растягиваются пропорционально доступному пространству
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);  // Название - растягивается
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Страна - растягивается
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Звезды - фиксированная
    ui->hotelsTable->setColumnWidth(2, 100);   // Звезды - достаточная ширина для заголовка
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);  // Адрес - растягивается
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);  // Номера - фиксированная
    ui->hotelsTable->setColumnWidth(4, 180); // Номера - достаточная ширина для заголовка "Количество\nномеров"
    ui->hotelsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);  // Действия - фиксированная
    ui->hotelsTable->setColumnWidth(5, 150);  // Действия - достаточная ширина для увеличенных иконок
    ui->hotelsTable->horizontalHeader()->setStretchLastSection(false);  // Отключаем растягивание последнего столбца
    // Устанавливаем заголовок столбца "Количество номеров" в две строки
    if (ui->hotelsTable->horizontalHeaderItem(4)) {
        ui->hotelsTable->horizontalHeaderItem(4)->setText("Количество\nномеров");
    }
    // Устанавливаем минимальную высоту заголовка для отображения двух строк
    ui->hotelsTable->horizontalHeader()->setMinimumHeight(50);
    // Отключаем сортировку для колонки действий
    if (ui->hotelsTable->horizontalHeaderItem(5)) {
        ui->hotelsTable->horizontalHeaderItem(5)->setFlags(Qt::NoItemFlags);
    }
    // Подключаем обработчик для трехрежимной сортировки (Ascending -> Descending -> None)
    ui->hotelsTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->hotelsTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onHotelsHeaderClicked);
    
    ui->transportTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->transportTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->transportTable->setSortingEnabled(false);  // Отключаем встроенную сортировку, используем кастомную трёхрежимную
    // Адаптивная ширина столбцов - растягиваются пропорционально доступному пространству
    ui->transportTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);  // Название - растягивается
    ui->transportTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Тип - растягивается
    ui->transportTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Количество рейсов - фиксированная
    ui->transportTable->setColumnWidth(2, 160);   // Количество рейсов - достаточная ширина для заголовка "Количество\nрейсов"
    ui->transportTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);  // Дата отправления - фиксированная
    ui->transportTable->setColumnWidth(3, 160);  // Дата отправления - достаточная ширина для двухстрочного заголовка
    ui->transportTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);  // Дата прибытия - фиксированная
    ui->transportTable->setColumnWidth(4, 160);  // Дата прибытия - достаточная ширина для двухстрочного заголовка
    ui->transportTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);  // Действия - фиксированная
    ui->transportTable->setColumnWidth(5, 150);   // Действия - достаточная ширина для увеличенных иконок
    ui->transportTable->horizontalHeader()->setStretchLastSection(false);  // Отключаем растягивание последнего столбца
    // Отключаем сортировку для колонки действий
    if (ui->transportTable->horizontalHeaderItem(5)) {
        ui->transportTable->horizontalHeaderItem(5)->setFlags(Qt::NoItemFlags);
    }
    
    // Устанавливаем минимальную высоту заголовка для отображения двух строк
    ui->transportTable->horizontalHeader()->setMinimumHeight(50);
    
    // Устанавливаем заголовок столбца "Количество рейсов" в две строки
    if (ui->transportTable->horizontalHeaderItem(2)) {
        ui->transportTable->horizontalHeaderItem(2)->setText("Количество\nрейсов");
    }
    
    // Устанавливаем заголовки столбцов "Дата отправления" и "Дата прибытия" в две строки
    if (ui->transportTable->horizontalHeaderItem(3)) {
        ui->transportTable->horizontalHeaderItem(3)->setText("Дата\nотправления");
    }
    if (ui->transportTable->horizontalHeaderItem(4)) {
        ui->transportTable->horizontalHeaderItem(4)->setText("Дата\nприбытия");
    }
    
    // Подключаем обработчик для трехрежимной сортировки (Ascending -> Descending -> None)
    ui->transportTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->transportTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onTransportHeaderClicked);
    
    ui->toursTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->toursTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->toursTable->setSortingEnabled(false);  // Отключаем встроенную сортировку, используем кастомную трёхрежимную
    // Адаптивная ширина столбцов - растягиваются пропорционально доступному пространству
    ui->toursTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);  // Название - растягивается
    ui->toursTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Страна - растягивается
    ui->toursTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Дата начала - фиксированная
    ui->toursTable->setColumnWidth(2, 200);  // Дата начала - достаточная ширина для заголовка "Дата начала"
    ui->toursTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);  // Дата окончания - фиксированная
    ui->toursTable->setColumnWidth(3, 200);  // Дата окончания - достаточная ширина для заголовка "Дата окончания"
    ui->toursTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);  // Стоимость - фиксированная
    ui->toursTable->setColumnWidth(4, 160);  // Стоимость - достаточная ширина для заголовка и "руб"
    ui->toursTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);  // Действия - фиксированная
    ui->toursTable->setColumnWidth(5, 150);  // Действия - достаточная ширина для увеличенных иконок
    ui->toursTable->horizontalHeader()->setStretchLastSection(false);  // Отключаем растягивание последнего столбца
    // Отключаем сортировку для колонки действий
    if (ui->toursTable->horizontalHeaderItem(5)) {
        ui->toursTable->horizontalHeaderItem(5)->setFlags(Qt::NoItemFlags);
    }
    // Подключаем обработчик для трехрежимной сортировки (Ascending -> Descending -> None)
    // Отключаем стандартную сортировку при клике, чтобы использовать свою логику
    ui->toursTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->toursTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onToursHeaderClicked);
    
    ui->ordersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->ordersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->ordersTable->setSortingEnabled(false);  // Отключаем встроенную сортировку, используем кастомную трёхрежимную
    // Адаптивная ширина столбцов - растягиваются пропорционально доступному пространству
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);  // Тур - растягивается
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);  // Клиент - растягивается
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);  // Телефон - фиксированная
    ui->ordersTable->setColumnWidth(2, 180);  // Телефон - достаточная ширина для полного номера телефона
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);  // Email - растягивается
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);  // Стоимость - фиксированная
    ui->ordersTable->setColumnWidth(4, 160);  // Стоимость - достаточная ширина для заголовка и "руб"
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);  // Статус - фиксированная
    ui->ordersTable->setColumnWidth(5, 160);  // Статус - достаточная ширина для полного статуса
    ui->ordersTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);  // Действия - фиксированная
    ui->ordersTable->setColumnWidth(6, 180);  // Действия - достаточная ширина для увеличенных иконок (4 кнопки)
    ui->ordersTable->horizontalHeader()->setStretchLastSection(false);  // Отключаем растягивание последнего столбца
    // Отключаем сортировку для колонки действий
    if (ui->ordersTable->horizontalHeaderItem(6)) {
        ui->ordersTable->horizontalHeaderItem(6)->setFlags(Qt::NoItemFlags);
    }
    // Подключаем обработчик для трехрежимной сортировки (Ascending -> Descending -> None)
    // Отключаем стандартную сортировку при клике, чтобы использовать свою логику
    ui->ordersTable->horizontalHeader()->setSectionsClickable(true);
    connect(ui->ordersTable->horizontalHeader(), &QHeaderView::sectionClicked, 
            this, &MainWindow::onOrdersHeaderClicked);
    
    // Устанавливаем минимальные ширины для заголовков, чтобы они полностью отображались
    ui->countriesTable->horizontalHeader()->setMinimumSectionSize(80);
    ui->hotelsTable->horizontalHeader()->setMinimumSectionSize(80);
    ui->transportTable->horizontalHeader()->setMinimumSectionSize(100);
    ui->toursTable->horizontalHeader()->setMinimumSectionSize(120);
    ui->ordersTable->horizontalHeader()->setMinimumSectionSize(100);
    
    // Настройка адаптивности элементов управления (кнопки, поиск, фильтры)
    setupControlsAdaptivity();
    
    // Настройка адаптивного размера шрифта для таблиц
    updateTablesFontSize();
}

void MainWindow::setupControlsAdaptivity() {
    // Настройка размеров шрифта для меток
    QFont labelFont;
    labelFont.setPointSize(12);
    
    // Настройка для вкладки "Страны"
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->countriesControlsWidget->layout())) {
        // Кнопка "Добавить" - фиксированная ширина, не растягивается
        layout->setStretchFactor(ui->addCountryButton, 0);
        // Поле поиска - растягивается
        layout->setStretchFactor(ui->searchCountryEdit, 2);
        // Комбобоксы фильтров - растягиваются пропорционально
        layout->setStretchFactor(ui->filterCountryCombo, 1);
        layout->setStretchFactor(ui->filterCountryCurrencyCombo, 1);
    }
    ui->searchCountryLabel->setFont(labelFont);
    ui->filterCountryLabel->setFont(labelFont);
    ui->filterCountryCurrencyLabel->setFont(labelFont);
    
    // Настройка для вкладки "Отели"
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->hotelsControlsWidget->layout())) {
        layout->setStretchFactor(ui->addHotelButton, 0);
        layout->setStretchFactor(ui->searchHotelEdit, 2);
        layout->setStretchFactor(ui->filterHotelCountryCombo, 1);
        layout->setStretchFactor(ui->filterHotelStarsCombo, 1);
    }
    ui->searchHotelLabel->setFont(labelFont);
    ui->filterHotelCountryLabel->setFont(labelFont);
    ui->filterHotelStarsLabel->setFont(labelFont);
    
    // Настройка для вкладки "Транспортные компании"
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->transportControlsWidget->layout())) {
        layout->setStretchFactor(ui->addTransportButton, 0);
        layout->setStretchFactor(ui->searchTransportEdit, 3);
        layout->setStretchFactor(ui->filterTransportTypeCombo, 1);
    }
    ui->searchTransportLabel->setFont(labelFont);
    ui->filterTransportTypeLabel->setFont(labelFont);
    
    // Настройка для вкладки "Туры"
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->toursControlsWidget->layout())) {
        layout->setStretchFactor(ui->addTourButton, 0);
        layout->setStretchFactor(ui->searchTourEdit, 2);
        layout->setStretchFactor(ui->filterTourCountryCombo, 1);
        // Поля ввода цены - фиксированная ширина, не растягиваются
        layout->setStretchFactor(ui->filterTourMinPriceEdit, 0);
        layout->setStretchFactor(ui->filterTourMaxPriceEdit, 0);
    }
    ui->searchTourLabel->setFont(labelFont);
    ui->filterTourCountryLabel->setFont(labelFont);
    ui->filterTourPriceLabel->setFont(labelFont);
    ui->filterTourPriceToLabel->setFont(labelFont);
    
    // Настройка для вкладки "Заказы"
    if (auto* layout = qobject_cast<QHBoxLayout*>(ui->ordersControlsWidget->layout())) {
        layout->setStretchFactor(ui->addOrderButton, 0);
        layout->setStretchFactor(ui->searchOrderEdit, 2);
        layout->setStretchFactor(ui->filterOrderStatusCombo, 1);
        // Поля ввода стоимости - фиксированная ширина, не растягиваются
        layout->setStretchFactor(ui->filterOrderMinCostEdit, 0);
        layout->setStretchFactor(ui->filterOrderMaxCostEdit, 0);
    }
    ui->searchOrderLabel->setFont(labelFont);
    ui->filterOrderStatusLabel->setFont(labelFont);
    ui->filterOrderCostLabel->setFont(labelFont);
    ui->filterOrderCostToLabel->setFont(labelFont);
}

void MainWindow::updateTablesFontSize() {
    // Базовые размеры окна (из UI файла)
    const int baseWidth = 900;
    const int baseHeight = 600;
    const int baseFontSize = 10; // Базовый размер шрифта в пунктах
    
    // Получаем текущий размер окна
    QSize currentSize = size();
    int currentWidth = currentSize.width();
    int currentHeight = currentSize.height();
    
    // Вычисляем коэффициент масштабирования на основе минимального размера
    // Используем среднее арифметическое для более плавного масштабирования
    double widthRatio = static_cast<double>(currentWidth) / baseWidth;
    double heightRatio = static_cast<double>(currentHeight) / baseHeight;
    double scaleFactor = (widthRatio + heightRatio) / 2.0;
    
    // Ограничиваем коэффициент масштабирования (от 0.8 до 1.5)
    scaleFactor = qBound(0.8, scaleFactor, 1.5);
    
    // Вычисляем новый размер шрифта
    int fontSize = qRound(baseFontSize * scaleFactor);
    
    // Устанавливаем размер шрифта для всех таблиц
    QFont tableFont;
    tableFont.setPointSize(fontSize);
    
    // Функция для обновления шрифта всех элементов таблицы
    auto updateTableFont = [&tableFont](QTableWidget* table) {
        table->setFont(tableFont);
        // Обновляем шрифт для всех существующих элементов
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
    
    // Применяем шрифт ко всем таблицам
    updateTableFont(ui->countriesTable);
    updateTableFont(ui->hotelsTable);
    updateTableFont(ui->transportTable);
    updateTableFont(ui->toursTable);
    updateTableFont(ui->ordersTable);
    
    // Также устанавливаем размер шрифта для заголовков таблиц
    QFont headerFont = tableFont;
    headerFont.setBold(true);
    headerFont.setPointSize(qRound(fontSize * 1.1)); // Заголовки немного больше
    
    ui->countriesTable->horizontalHeader()->setFont(headerFont);
    ui->hotelsTable->horizontalHeader()->setFont(headerFont);
    ui->transportTable->horizontalHeader()->setFont(headerFont);
    ui->toursTable->horizontalHeader()->setFont(headerFont);
    ui->ordersTable->horizontalHeader()->setFont(headerFont);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    // Обновляем размер шрифта при изменении размера окна
    updateTablesFontSize();
}

// Вспомогательная функция для создания виджета с кнопками действий
QWidget* MainWindow::createActionButtons(int dataIndex, const QString& type) {
    QWidget* widget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(widget);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);
    
    // Функция для создания красной иконки крестика
    auto createRedCrossIcon = []() -> QIcon {
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(220, 53, 69), 2.5)); // Красный цвет
        painter.drawLine(3, 3, 17, 17);
        painter.drawLine(17, 3, 3, 17);
        return QIcon(pixmap);
    };
    
    QIcon redCrossIcon = createRedCrossIcon();
    
    // Создаем кнопки с улучшенными иконками
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
    
    // Функция для создания иконки редактирования (синий карандаш в квадрате)
    auto createEditIcon = []() -> QIcon {
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Рисуем синий/голубой квадрат с закругленными углами
        painter.setBrush(QBrush(QColor(33, 150, 243))); // Синий цвет Material Design
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(0, 0, 20, 20, 4, 4);
        
        // Рисуем белый карандаш в центре
        painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        
        // Тело карандаша (линия от нижнего левого к верхнему правому)
        painter.drawLine(6, 14, 14, 6);
        
        // Острие карандаша (маленький треугольник)
        QPolygon tip;
        tip << QPoint(5, 15) << QPoint(6, 14) << QPoint(7, 15);
        painter.setBrush(QBrush(Qt::white));
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(tip);
        
        // Рисуем маленькую линию редактирования снизу
        painter.setPen(QPen(Qt::white, 1.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(4, 16, 9, 16);
        
        return QIcon(pixmap);
    };
    
    QIcon editIcon = createEditIcon();
    
    // Функция для создания иконки обработки (оранжевая/янтарная шестеренка)
    auto createProcessIcon = []() -> QIcon {
        QPixmap pixmap(20, 20);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Рисуем оранжевый круг
        painter.setBrush(QBrush(QColor(255, 152, 0))); // Оранжевый цвет Material Design
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(0, 0, 20, 20);
        
        // Рисуем белую галочку
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
    // Оптимизация: отключаем обновление виджета во время заполнения
    ui->countriesTable->setUpdatesEnabled(false);
    
    // Временно отключаем сортировку при обновлении
    ui->countriesTable->setSortingEnabled(false);
    
    // Оптимизация: очищаем старые элементы перед добавлением новых
    ui->countriesTable->clearContents();
    ui->countriesTable->setRowCount(countries_.size());
    
    int row = 0;
    int dataIndex = 0;  // Реальный индекс в контейнере
    for (const auto& country : countries_.getData()) {
        // Показываем все строки при обновлении
        ui->countriesTable->setRowHidden(row, false);
        // Сохраняем реальный индекс в UserRole первого столбца для правильной работы после сортировки
        QTableWidgetItem* nameItem = new QTableWidgetItem(country.getName());
        nameItem->setData(Qt::UserRole, dataIndex);  // Сохраняем реальный индекс
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
        
        // Добавляем кнопки действий
        ui->countriesTable->setCellWidget(row, 4, createActionButtons(dataIndex, "country"));
        
        ++row;
        ++dataIndex;
    }
    
    // Включаем сортировку обратно
    ui->countriesTable->setSortingEnabled(true);
    
    // Включаем обновление виджета
    ui->countriesTable->setUpdatesEnabled(true);
    
    // Обновляем фильтры
    filterComboUpdater_->updateCountriesFilterCombo(ui->filterCountryCombo,
                                                     ui->filterCountryCurrencyCombo,
                                                     countries_);
    // Применяем фильтры после обновления
    filterManager_->applyCountriesFilters(ui->countriesTable, ui->searchCountryEdit,
                                         ui->filterCountryCombo, ui->filterCountryCurrencyCombo);
}

void MainWindow::updateHotelsTable() {
    // Оптимизация: отключаем обновление виджета во время заполнения
    ui->hotelsTable->setUpdatesEnabled(false);
    
    // Временно отключаем сортировку при обновлении
    ui->hotelsTable->setSortingEnabled(false);
    
    // Оптимизация: очищаем старые элементы перед добавлением новых
    ui->hotelsTable->clearContents();
    ui->hotelsTable->setRowCount(hotels_.size());
    
    int row = 0;
    int dataIndex = 0;  // Реальный индекс в контейнере
    for (const auto& hotel : hotels_.getData()) {
        // Показываем все строки при обновлении
        ui->hotelsTable->setRowHidden(row, false);
        // Сохраняем реальный индекс в UserRole первого столбца для правильной работы после сортировки
        QTableWidgetItem* nameItem = new QTableWidgetItem(hotel.getName());
        nameItem->setData(Qt::UserRole, dataIndex);  // Сохраняем реальный индекс
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
        
        // Добавляем кнопки действий
        ui->hotelsTable->setCellWidget(row, 5, createActionButtons(dataIndex, "hotel"));
        
        ++row;
        ++dataIndex;
    }
    
    // Включаем сортировку обратно
    ui->hotelsTable->setSortingEnabled(true);
    
    // Включаем обновление виджета
    ui->hotelsTable->setUpdatesEnabled(true);
    
    // Применяем фильтры после обновления
    applyHotelsFilters();
}

void MainWindow::updateTransportCompaniesTable() {
    // Оптимизация: отключаем обновление виджета во время заполнения
    ui->transportTable->setUpdatesEnabled(false);
    
    // Временно отключаем сортировку при обновлении
    ui->transportTable->setSortingEnabled(false);
    
    // Оптимизация: очищаем старые элементы перед добавлением новых
    ui->transportTable->clearContents();
    ui->transportTable->setRowCount(transportCompanies_.size());
    
    int row = 0;
    int dataIndex = 0;  // Реальный индекс в контейнере
    for (const auto& company : transportCompanies_.getData()) {
        // Показываем все строки при обновлении
        ui->transportTable->setRowHidden(row, false);
        // Сохраняем реальный индекс в UserRole первого столбца для правильной работы после сортировки
        QTableWidgetItem* nameItem = new QTableWidgetItem(company.getName());
        nameItem->setData(Qt::UserRole, dataIndex);  // Сохраняем реальный индекс
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->transportTable->setItem(row, 0, nameItem);
        
        QTableWidgetItem* typeItem = new QTableWidgetItem(
            TransportCompany::transportTypeToString(company.getTransportType()));
        typeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->transportTable->setItem(row, 1, typeItem);
        
        // Используем NumericSortItem для правильной числовой сортировки
        int scheduleCount = company.getScheduleCount();
        NumericSortItem* scheduleCountItem = new NumericSortItem(
            QString::number(scheduleCount), 
            static_cast<double>(scheduleCount)
        );
        // Выравниваем значения по центру
        scheduleCountItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->transportTable->setItem(row, 2, scheduleCountItem);
        
        // Добавляем информацию о датах первого расписания (если есть)
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
        
        // Добавляем кнопки действий
        ui->transportTable->setCellWidget(row, 5, createActionButtons(dataIndex, "transport"));
        
        ++row;
        ++dataIndex;
    }
    
    // Включаем сортировку обратно
    ui->transportTable->setSortingEnabled(true);
    
    // Включаем обновление виджета
    ui->transportTable->setUpdatesEnabled(true);
    
    // Применяем фильтры после обновления
    applyTransportFilters();
}

void MainWindow::updateToursTable() {
    // Оптимизация: отключаем обновление виджета во время заполнения
    ui->toursTable->setUpdatesEnabled(false);
    
    // Временно отключаем сортировку при обновлении
    ui->toursTable->setSortingEnabled(false);
    
    // Оптимизация: очищаем старые элементы перед добавлением новых
    ui->toursTable->clearContents();
    
    // Подсчитываем количество валидных туров (с непустыми именами)
    int validTourCount = 0;
    for (const auto& tour : tours_.getData()) {
        if (!tour.getName().isEmpty() && !tour.getCountry().isEmpty()) {
            validTourCount++;
        }
    }
    
    ui->toursTable->setRowCount(validTourCount);
    
    int row = 0;
    int dataIndex = 0;  // Реальный индекс в контейнере
    for (const auto& tour : tours_.getData()) {
        // Пропускаем туры с пустыми именами или странами
        if (tour.getName().isEmpty() || tour.getCountry().isEmpty()) {
            ++dataIndex;
            continue;
        }
        
        // Показываем все строки при обновлении
        ui->toursTable->setRowHidden(row, false);
        
        // Сохраняем реальный индекс в UserRole первого столбца для правильной работы после сортировки
        QTableWidgetItem* nameItem = new QTableWidgetItem(tour.getName());
        nameItem->setData(Qt::UserRole, dataIndex);  // Сохраняем реальный индекс
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 0, nameItem);
        
        QTableWidgetItem* countryItem = new QTableWidgetItem(tour.getCountry());
        countryItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 1, countryItem);
        
        // Проверяем валидность дат перед отображением
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
        
        // Отображаем стоимость используя NumericSortItem для правильной числовой сортировки
        double cost = tour.calculateCost();
        NumericSortItem* costItem = new NumericSortItem(QString::number(cost, 'f', 2) + " руб", cost);
        costItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->toursTable->setItem(row, 4, costItem);
        
        // Добавляем кнопки действий
        ui->toursTable->setCellWidget(row, 5, createActionButtons(dataIndex, "tour"));
        
        ++row;
        ++dataIndex;
    }
    
    // Включаем сортировку (но не применяем автоматическую сортировку, чтобы сохранить порядок по умолчанию)
    ui->toursTable->setSortingEnabled(true);
    
    // Включаем обновление виджета
    ui->toursTable->setUpdatesEnabled(true);
    
    // Применяем фильтры после обновления
    applyToursFilters();
}

void MainWindow::updateOrdersTable() {
    // Оптимизация: отключаем обновление виджета во время заполнения
    ui->ordersTable->setUpdatesEnabled(false);
    
    // Временно отключаем сортировку при обновлении
    ui->ordersTable->setSortingEnabled(false);
    
    // Оптимизация: очищаем старые элементы перед добавлением новых
    ui->ordersTable->clearContents();
    
    // Подсчитываем количество валидных заказов (с непустыми полями)
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
    int dataIndex = 0;  // Реальный индекс в контейнере
    for (const auto& order : orders_.getData()) {
        // Пропускаем заказы с пустыми полями
        if (order.getTour().getName().isEmpty() || 
            order.getClientName().isEmpty() || 
            order.getClientPhone().isEmpty()) {
            ++dataIndex;
            continue;
        }
        
        // Показываем все строки при обновлении
        ui->ordersTable->setRowHidden(row, false);
        
        // Сохраняем реальный индекс в UserRole первого столбца (Тур) для правильной работы после сортировки
        QTableWidgetItem* tourItem = new QTableWidgetItem(order.getTour().getName());
        tourItem->setData(Qt::UserRole, dataIndex);  // Сохраняем реальный индекс
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
        
        // Отображаем стоимость используя NumericSortItem для правильной числовой сортировки
        // Стоимость хранится как число, а не строка
        double cost = order.getTotalCost();
        NumericSortItem* costItem = new NumericSortItem(QString::number(cost, 'f', 2) + " руб", cost);
        costItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 4, costItem);
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(order.getStatus());
        statusItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->ordersTable->setItem(row, 5, statusItem);
        
        // Добавляем кнопки действий
        ui->ordersTable->setCellWidget(row, 6, createActionButtons(dataIndex, "order"));
        
        ++row;
        ++dataIndex;
    }
    
    // Включаем сортировку (но не применяем автоматическую сортировку, чтобы сохранить порядок по умолчанию)
    ui->ordersTable->setSortingEnabled(true);
    
    // Включаем обновление виджета
    ui->ordersTable->setUpdatesEnabled(true);
    
    // Применяем фильтры после обновления
    applyOrdersFilters();
}

int MainWindow::getSelectedRow(QTableWidget* table) const {
    QList<QTableWidgetItem*> items = table->selectedItems();
    if (items.isEmpty()) return -1;
    return items.first()->row();
}

// Получает реальный индекс тура в контейнере из выбранной строки таблицы
int MainWindow::getSelectedTourIndex() const {
    QList<QTableWidgetItem*> items = ui->toursTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    // Берем первый столбец (название), где хранится реальный индекс в UserRole
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->toursTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    // Получаем реальный индекс из UserRole
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    return -1;
}

// Получает реальный индекс заказа в контейнере из выбранной строки таблицы
int MainWindow::getSelectedOrderIndex() const {
    QList<QTableWidgetItem*> items = ui->ordersTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    // Берем первый столбец (Тур), где хранится реальный индекс в UserRole
    int visualRow = items.first()->row();
    QTableWidgetItem* tourItem = ui->ordersTable->item(visualRow, 0);
    if (!tourItem) return -1;
    
    // Получаем реальный индекс из UserRole
    QVariant data = tourItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    // Если UserRole нет (старые данные), используем визуальный индекс как fallback
    return visualRow;
}

// Получает реальный индекс страны в контейнере из выбранной строки таблицы
int MainWindow::getSelectedCountryIndex() const {
    QList<QTableWidgetItem*> items = ui->countriesTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    // Берем первый столбец (название), где хранится реальный индекс в UserRole
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->countriesTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    // Получаем реальный индекс из UserRole
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    // Если UserRole нет (старые данные), используем визуальный индекс как fallback
    return visualRow;
}

// Получает реальный индекс отеля в контейнере из выбранной строки таблицы
int MainWindow::getSelectedHotelIndex() const {
    QList<QTableWidgetItem*> items = ui->hotelsTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    // Берем первый столбец (название), где хранится реальный индекс в UserRole
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->hotelsTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    // Получаем реальный индекс из UserRole
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    // Если UserRole нет (старые данные), используем визуальный индекс как fallback
    return visualRow;
}

// Получает реальный индекс транспортной компании в контейнере из выбранной строки таблицы
int MainWindow::getSelectedTransportIndex() const {
    QList<QTableWidgetItem*> items = ui->transportTable->selectedItems();
    if (items.isEmpty()) return -1;
    
    // Берем первый столбец (название), где хранится реальный индекс в UserRole
    int visualRow = items.first()->row();
    QTableWidgetItem* nameItem = ui->transportTable->item(visualRow, 0);
    if (!nameItem) return -1;
    
    // Получаем реальный индекс из UserRole
    QVariant data = nameItem->data(Qt::UserRole);
    if (data.isValid() && data.canConvert<int>()) {
        return data.toInt();
    }
    
    // Если UserRole нет (старые данные), используем визуальный индекс как fallback
    return visualRow;
}

// Страны
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
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Проверяем, вызвано ли из кнопки
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

// Отели
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
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Проверяем, вызвано ли из кнопки
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

// Транспортные компании
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
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Проверяем, вызвано ли из кнопки
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

// Туры
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
    
    // Проверяем, вызвано ли из кнопки
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
        updateToursTable();  // Обновляем таблицу на случай рассинхронизации индексов
        return;
    }
    
    TourDialog dialog(this, &countries_, &hotels_, &transportCompanies_, tour);
    if (dialog.exec() == QDialog::Accepted) {
        Tour newTour = dialog.getTour();
        
        // Проверяем валидность нового тура
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
    
    // Проверяем, вызвано ли из кнопки
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
        linkToursWithHotelsAndTransport();  // Обновляем связи после удаления
        applyToursFilters();
        statusBar()->showMessage("Тур удален", 2000);
    }
}

void MainWindow::showTourInfo() {
    int dataIndex = -1;
    
    // Проверяем, вызвано ли из кнопки
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

// Заказы
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
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Список доступных статусов
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
        
        // Автоматически сохраняем данные после изменения статуса
        try {
            // Используем ту же логику поиска папки data, что и в loadData
            QString dataPath = "data";
            QStringList possiblePaths;
            
            QDir appDir(QCoreApplication::applicationDirPath());
            possiblePaths << appDir.absoluteFilePath("data");
            possiblePaths << QDir("data").absolutePath();
            
            QDir parentDir = appDir;
            if (parentDir.cdUp()) {
                possiblePaths << parentDir.absoluteFilePath("data");
            }
            
            // Ищем первый существующий путь
            bool found = false;
            for (const QString& path : possiblePaths) {
                if (QDir(path).exists()) {
                    dataPath = path;
                    found = true;
                    break;
                }
            }
            
            // Если не нашли, используем путь рядом с exe по умолчанию
            if (!found) {
                dataPath = appDir.absoluteFilePath("data");
                QDir().mkpath(dataPath); // Создаем папку, если её нет
            }
            
            // Сохраняем только заказы, чтобы не перезаписывать другие данные
            fileManager_.saveOrders(orders_, dataPath + "/orders.txt");
        } catch (const FileException& e) {
            qWarning() << "Failed to auto-save orders after status change:" << e.what();
            // Не показываем ошибку пользователю, чтобы не мешать работе
        }
        
        statusBar()->showMessage(QString("Статус заказа #%1 изменен на '%2' (сохранено)").arg(order->getId()).arg(newStatus), 2000);
    }
}

void MainWindow::editOrder() {
    int dataIndex = -1;
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Создаем диалог для редактирования заказа
    BookTourDialog dialog(this, &countries_, &hotels_, &transportCompanies_, &tours_);
    
    // Устанавливаем текущие данные заказа в диалог
    dialog.setOrder(*order);
    
    if (dialog.exec() == QDialog::Accepted) {
        Order newOrder = dialog.getOrder();
        
        // Обновляем заказ
        order->setTour(newOrder.getTour());
        order->setClientName(newOrder.getClientName());
        order->setClientPhone(newOrder.getClientPhone());
        
        // Связываем тур заказа с отелями и транспортом для правильного расчета стоимости
        linkOrdersToursWithHotelsAndTransport();
        updateOrdersTable();
        applyOrdersFilters();
        statusBar()->showMessage("Заказ обновлен", 2000);
    }
}

void MainWindow::deleteOrder() {
    int dataIndex = -1;
    
    // Проверяем, вызвано ли из кнопки
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
    
    // Проверяем, вызвано ли из кнопки
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

// Файловые операции
void MainWindow::saveData() {
    try {
        // Получаем путь к данным (используем ту же логику, что и при загрузке)
        QString dataPath = "data";
        
        // Список возможных путей для поиска папки data
        QStringList possiblePaths;
        
        // 1. Текущая директория
        possiblePaths << QDir("data").absolutePath();
        
        // 2. Рядом с исполняемым файлом
        QDir appDir(QCoreApplication::applicationDirPath());
        possiblePaths << appDir.absoluteFilePath("data");
        
        // 3. В родительской директории от exe
        QDir parentDir = appDir;
        if (parentDir.cdUp()) {
            possiblePaths << parentDir.absoluteFilePath("data");
        }
        
        // Ищем первый существующий путь
        bool found = false;
        for (const QString& path : possiblePaths) {
            if (QDir(path).exists()) {
                dataPath = path;
                found = true;
                break;
            }
        }
        
        // Если не нашли, используем путь рядом с exe по умолчанию
        if (!found) {
            dataPath = appDir.absoluteFilePath("data");
            // Создаем папку, если её нет
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
    
    // 1. Текущая директория
    possiblePaths << QDir("data").absolutePath();
    
    // 2. Рядом с исполняемым файлом
    QDir appDir(QCoreApplication::applicationDirPath());
    possiblePaths << appDir.absoluteFilePath("data");
    
    // 3. В родительской директории от exe
    QDir parentDir = appDir;
    if (parentDir.cdUp()) {
        possiblePaths << parentDir.absoluteFilePath("data");
    }
    
    // 4. Пробуем найти относительно корня проекта
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
            foundProjectRoot = true; // Достигли корня, выходим
        }
    }
    
    // Ищем первый существующий путь
    for (const QString& path : possiblePaths) {
        if (QDir(path).exists()) {
            return path;
        }
    }
    
    // Если не нашли, используем путь рядом с exe по умолчанию
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
    // Оптимизация: не обновляем таблицы при переключении вкладок
    // Таблицы обновляются только при изменении данных или при первой загрузке
    // Это значительно ускоряет переключение между вкладками
    // Обновление фильтров также отложено - они обновляются только при необходимости
}

// Обновление комбобоксов фильтров
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
    
    // Заполняем комбобокс валют
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
    // Обновляем комбобокс стран
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
    
    // Обновляем комбобокс звезд
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
    
    // Собираем уникальные типы транспорта из существующих компаний
    QSet<QString> transportTypes;
    for (const auto& company : transportCompanies_.getData()) {
        QString typeStr = TransportCompany::transportTypeToString(company.getTransportType());
        if (!typeStr.isEmpty()) {
            transportTypes.insert(typeStr);
        }
    }
    
    // Сортируем и добавляем в комбобокс
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

// Вспомогательные функции для фильтрации
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
            // Для столбца стоимости убираем " руб" перед поиском
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

// Методы фильтрации и поиска
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
    
    // Собираем города из адресов отелей
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
    
    // Проверяем точное совпадение с целевыми городами
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
    
    // Проверяем совпадение со столицей
    if (!capital.isEmpty()) {
        QString capitalLower = capital.toLower();
        if (arrivalCityLower == capitalLower ||
            arrivalCityLower.contains(capitalLower) || 
            capitalLower.contains(arrivalCityLower)) {
            return true;
        }
    }
    
    // Специальные случаи (например, для ОАЭ)
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
            
            // Проверяем совпадение города
            if (!matchesCity(arrivalCity, targetCities, capital, tour.getCountry())) {
                continue;
            }
            
            // Проверяем совпадение дат
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
        
        // Находим столицу и собираем целевые города
        QString capital = findCountryCapital(tourCountry);
        QSet<QString> targetCities = collectTargetCities(tourCountry, capital);
        
        // Ищем и устанавливаем отель
        Hotel* selectedHotel = findHotelForTour(tourCountry);
        if (selectedHotel) {
            tour.setHotel(*selectedHotel);
        }
        
        // Ищем и устанавливаем транспорт
        findTransportForTour(tour, targetCities, capital, tourStartDate);
    }
}

void MainWindow::linkOrdersToursWithHotelsAndTransport() {
    for (auto& order : orders_.getData()) {
        Tour tourInOrder = order.getTour();
        QString tourName = tourInOrder.getName();
        QString tourCountry = tourInOrder.getCountry();
        
        // Ищем полный тур по названию и стране в списке туров
        for (const auto& fullTour : tours_.getData()) {
            if (fullTour.getName() == tourName && fullTour.getCountry() == tourCountry) {
                // Нашли полный тур - заменяем тур в заказе на полный
                order.setTour(fullTour);
                break;
            }
        }
    }
}

// Обработчик клика по заголовку таблицы стран для трехрежимной сортировки
void MainWindow::onCountriesHeaderClicked(int logicalIndex) {
    // Игнорируем клик по колонке с действиями
    if (logicalIndex == 4) return;
    
    static QMap<int, int> columnStates;  // Хранит состояние каждого столбца: 0 - нет сортировки, 1 - возрастание, 2 - убывание
    
    // Получаем текущее состояние столбца
    int currentState = columnStates.value(logicalIndex, 0);
    
    // Переключаем состояние: 0 -> 1 (возрастание), 1 -> 2 (убывание), 2 -> 0 (отключить)
    if (currentState == 0) {
        // Включаем сортировку по возрастанию
        columnStates[logicalIndex] = 1;
        ui->countriesTable->setSortingEnabled(false);
        ui->countriesTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->countriesTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->countriesTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        // Переключаем на убывание
        columnStates[logicalIndex] = 2;
        ui->countriesTable->setSortingEnabled(false);
        ui->countriesTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->countriesTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->countriesTable->setSortingEnabled(true);
    } else {
        // Отключаем сортировку
        columnStates[logicalIndex] = 0;
        
        // Восстанавливаем исходный порядок
        ui->countriesTable->setSortingEnabled(false);
        updateCountriesTable();
        ui->countriesTable->setSortingEnabled(true);
        
        // Убираем индикатор сортировки
        ui->countriesTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        // Сбрасываем состояния всех столбцов
        columnStates.clear();
    }
}

// Обработчик клика по заголовку таблицы отелей для трехрежимной сортировки
void MainWindow::onHotelsHeaderClicked(int logicalIndex) {
    // Игнорируем клик по колонке с действиями
    if (logicalIndex == 5) return;
    
    static QMap<int, int> columnStates;  // Хранит состояние каждого столбца: 0 - нет сортировки, 1 - возрастание, 2 - убывание
    
    // Получаем текущее состояние столбца
    int currentState = columnStates.value(logicalIndex, 0);
    
    // Переключаем состояние: 0 -> 1 (возрастание), 1 -> 2 (убывание), 2 -> 0 (отключить)
    if (currentState == 0) {
        // Включаем сортировку по возрастанию
        columnStates[logicalIndex] = 1;
        ui->hotelsTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->hotelsTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
    } else if (currentState == 1) {
        // Переключаем на убывание
        columnStates[logicalIndex] = 2;
        ui->hotelsTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->hotelsTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
    } else {
        // Отключаем сортировку
        columnStates[logicalIndex] = 0;
        
        // Восстанавливаем исходный порядок
        updateHotelsTable();
        
        // Убираем индикатор сортировки
        ui->hotelsTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        // Сбрасываем состояния всех столбцов
        columnStates.clear();
    }
}

// Обработчик клика по заголовку таблицы транспортных компаний для трехрежимной сортировки
void MainWindow::onTransportHeaderClicked(int logicalIndex) {
    // Игнорируем клик по колонке с действиями
    if (logicalIndex == 5) return;
    
    static QMap<int, int> columnStates;  // Хранит состояние каждого столбца: 0 - нет сортировки, 1 - возрастание, 2 - убывание
    
    // Получаем текущее состояние столбца
    int currentState = columnStates.value(logicalIndex, 0);
    
    // Переключаем состояние: 0 -> 1 (возрастание), 1 -> 2 (убывание), 2 -> 0 (отключить)
    if (currentState == 0) {
        // Включаем сортировку по возрастанию
        columnStates[logicalIndex] = 1;
        ui->transportTable->setSortingEnabled(false);
        ui->transportTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->transportTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->transportTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        // Переключаем на убывание
        columnStates[logicalIndex] = 2;
        ui->transportTable->setSortingEnabled(false);
        ui->transportTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->transportTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->transportTable->setSortingEnabled(true);
    } else {
        // Отключаем сортировку
        columnStates[logicalIndex] = 0;
        
        // Восстанавливаем исходный порядок
        ui->transportTable->setSortingEnabled(false);
        updateTransportCompaniesTable();
        ui->transportTable->setSortingEnabled(true);
        
        // Убираем индикатор сортировки
        ui->transportTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        // Сбрасываем состояния всех столбцов
        columnStates.clear();
    }
}

// Обработчик клика по заголовку таблицы туров для трехрежимной сортировки
void MainWindow::onToursHeaderClicked(int logicalIndex) {
    // Игнорируем клик по колонке с действиями
    if (logicalIndex == 5) return;
    
    static QMap<int, int> columnStates;  // Хранит состояние каждого столбца: 0 - нет сортировки, 1 - возрастание, 2 - убывание
    
    // Получаем текущее состояние столбца
    int currentState = columnStates.value(logicalIndex, 0);
    
    // Переключаем состояние: 0 -> 1 (возрастание), 1 -> 2 (убывание), 2 -> 0 (отключить)
    if (currentState == 0) {
        // Включаем сортировку по возрастанию
        columnStates[logicalIndex] = 1;
        // Отключаем стандартную сортировку, чтобы предотвратить двойную сортировку
        ui->toursTable->setSortingEnabled(false);
        ui->toursTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->toursTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->toursTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        // Переключаем на убывание
        columnStates[logicalIndex] = 2;
        ui->toursTable->setSortingEnabled(false);
        ui->toursTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->toursTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->toursTable->setSortingEnabled(true);
    } else {
        // Отключаем сортировку
        columnStates[logicalIndex] = 0;
        
        // Восстанавливаем исходный порядок
        ui->toursTable->setSortingEnabled(false);
        updateToursTable();
        ui->toursTable->setSortingEnabled(true);
        
        // Убираем индикатор сортировки
        ui->toursTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        // Сбрасываем состояния всех столбцов, так как мы вернулись к исходному порядку
        columnStates.clear();
    }
}

// Обработчик клика по заголовку таблицы заказов для трехрежимной сортировки
void MainWindow::onOrdersHeaderClicked(int logicalIndex) {
    // Игнорируем клик по колонке с действиями
    if (logicalIndex == 6) return;
    
    static QMap<int, int> columnStates;  // Хранит состояние каждого столбца: 0 - нет сортировки, 1 - возрастание, 2 - убывание
    
    // Получаем текущее состояние столбца
    int currentState = columnStates.value(logicalIndex, 0);
    
    // Переключаем состояние: 0 -> 1 (возрастание), 1 -> 2 (убывание), 2 -> 0 (отключить)
    if (currentState == 0) {
        // Включаем сортировку по возрастанию
        columnStates[logicalIndex] = 1;
        // Отключаем стандартную сортировку, чтобы предотвратить двойную сортировку
        ui->ordersTable->setSortingEnabled(false);
        ui->ordersTable->sortItems(logicalIndex, Qt::AscendingOrder);
        ui->ordersTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::AscendingOrder);
        ui->ordersTable->setSortingEnabled(true);
    } else if (currentState == 1) {
        // Переключаем на убывание
        columnStates[logicalIndex] = 2;
        ui->ordersTable->setSortingEnabled(false);
        ui->ordersTable->sortItems(logicalIndex, Qt::DescendingOrder);
        ui->ordersTable->horizontalHeader()->setSortIndicator(logicalIndex, Qt::DescendingOrder);
        ui->ordersTable->setSortingEnabled(true);
    } else {
        // Отключаем сортировку
        columnStates[logicalIndex] = 0;
        
        // Восстанавливаем исходный порядок
        ui->ordersTable->setSortingEnabled(false);
        updateOrdersTable();
        ui->ordersTable->setSortingEnabled(true);
        
        // Убираем индикатор сортировки
        ui->ordersTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
        
        // Сбрасываем состояния всех столбцов, так как мы вернулись к исходному порядку
        columnStates.clear();
    }
}

// Функция для обновления курсов валют
void MainWindow::updateCurrencyRates() {
    // Используем exchangerate-api.com для получения курсов к BYN
    QUrl url("https://api.exchangerate-api.com/v4/latest/BYN");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    networkManager_->get(request);
}

// Обработчик полученных данных о курсах валют
void MainWindow::onCurrencyDataReceived(QNetworkReply* reply) {
    // Находим метки по их именам объектов
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
            
            // Получаем курсы (API возвращает курсы ОТ BYN, нужно инвертировать)
            if (rates.contains("USD")) {
                double bynToUsd = rates["USD"].toDouble();
                double usdToByn = 1.0 / bynToUsd;  // Инвертируем
                usdLabel->setText(QString("1 USD = %1 BYN").arg(usdToByn, 0, 'f', 2));
            } else {
                usdLabel->setText("USD: не найден");
            }
            
            if (rates.contains("EUR")) {
                double bynToEur = rates["EUR"].toDouble();
                double eurToByn = 1.0 / bynToEur;  // Инвертируем
                eurLabel->setText(QString("1 EUR = %1 BYN").arg(eurToByn, 0, 'f', 2));
            } else {
                eurLabel->setText("EUR: не найден");
            }
            
            // Обновляем время последнего обновления
            QDateTime now = QDateTime::currentDateTime();
            updateLabel->setText(
                QString("(обновлено %1)").arg(now.toString("HH:mm"))
            );
        } else {
            usdLabel->setText("USD: ошибка формата");
            eurLabel->setText("EUR: ошибка формата");
        }
    } else {
        // В случае ошибки показываем сообщение
        usdLabel->setText("USD: нет связи");
        eurLabel->setText("EUR: нет связи");
        updateLabel->setText("(оффлайн)");
    }
    
    reply->deleteLater();
}

void MainWindow::initializeActions() {
    // Создаем классы-операторы для стран
    actions_["addCountry"] = new AddCountryAction(&countries_, ui->countriesTable, this);
    actions_["editCountry"] = new EditCountryAction(&countries_, ui->countriesTable, this);
    actions_["deleteCountry"] = new DeleteCountryAction(&countries_, ui->countriesTable, this);
    actions_["showCountryInfo"] = new ShowCountryInfoAction(&countries_, ui->countriesTable, this);
    actions_["refreshCountries"] = new RefreshCountriesAction();
    
    // Подключаем сигналы действий к обновлению таблиц
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
    
    // Подключаем действия к меню
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
