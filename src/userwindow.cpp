#include "userwindow.h"
#include "ui_userwindow.h"
#include "agency/agencydata.h"
#include "agency/algorithms.h"
#include "agency/validation.h"
#include "utils/passwordtoggle.h"
#include "utils/toursortheaderview.h"
#include "utils/tourtabledisplay.h"

#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
void setTourRow(QTableWidget* table, int row, const agency::Tour* t)
{
    // Заполняем одну строку таблицы тура.
    table->setItem(row, 0, new QTableWidgetItem(QString::number(t->id)));
    table->setItem(row, 1, new QTableWidgetItem(agency::tourDisplayName(t)));
    table->setItem(row, 2, new QTableWidgetItem(t->country));
    table->setItem(row, 3, new QTableWidgetItem(QString::number(t->price)));
    table->setItem(row, 4, new QTableWidgetItem(QString::number(t->days)));
    table->setItem(row, 5, new QTableWidgetItem(t->hotel));
    table->setItem(row, 6, new QTableWidgetItem(t->hotelAddress));
    table->setItem(row, 7, new QTableWidgetItem(QString::number(t->hotelStars)));
    table->setItem(row, 8, new QTableWidgetItem(t->dateStart));
    table->setItem(row, 9, new QTableWidgetItem(t->dateEnd));
    for (int c = 0; c < agency::TourTableColumns::Count; ++c) {
        QTableWidgetItem* it = table->item(row, c);
        if (it) {
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        }
    }
}
} // namespace

UserWindow::UserWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::UserWindow>())
{
    ui->setupUi(this);
    auto* tourHeader = new TourSortHeaderView(Qt::Horizontal, ui->toursTable);
    ui->toursTable->setHorizontalHeader(tourHeader);
    ui->toursTable->setColumnCount(agency::TourTableColumns::Count);
    tourSort_.applyToTableHeader(ui->toursTable);
    tour_widgets::applyTourTableColumnLayout(ui->toursTable, true);

    connect(tourHeader, &TourSortHeaderView::sortSectionClicked, this,
            [this](int logicalIndex, Qt::KeyboardModifiers modifiers) {
                tourSort_.onHeaderClicked(logicalIndex, modifiers);
                applySortToCurrentTours();
            });

    connect(ui->filterButton, &QPushButton::clicked, this, &UserWindow::onFilter);
    connect(ui->bookButton, &QPushButton::clicked, this, &UserWindow::onBook);
    connect(ui->profileButton, &QPushButton::clicked, this, &UserWindow::onProfile);
    connect(ui->bookingsButton, &QPushButton::clicked, this, &UserWindow::onBookings);
    connect(ui->logoutButton, &QPushButton::clicked, this, &UserWindow::onLogout);

    refreshTours();
    statusBar()->showMessage(tourSort_.statusHint());
}

UserWindow::~UserWindow() = default;

void UserWindow::closeEvent(QCloseEvent* event)
{
    agency::AgencyData& data = agency::AgencyData::instance();
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
    }
    QMainWindow::closeEvent(event);
}

void UserWindow::refreshTours()
{
    onFilter();
}

void UserWindow::onFilter()
{
    agency::AgencyData& data = agency::AgencyData::instance();
    const QString country = ui->countryEdit->text();

    int minPrice = 0;
    int maxPrice = 0;
    // Парсим интервалы фильтра; пустые поля трактуются как "без ограничения".
    const agency::ValidationResult pr = agency::parseOptionalIntRange(
        ui->minPriceEdit->text(), ui->maxPriceEdit->text(), minPrice, maxPrice,
        QStringLiteral("Мин. цена"), QStringLiteral("Макс. цена"));
    if (!pr.ok) {
        QMessageBox::warning(this, QStringLiteral("Фильтр"), pr.errorMessage);
        return;
    }
    int minDays = 0;
    int maxDays = 0;
    const agency::ValidationResult dr = agency::parseOptionalIntRange(
        ui->minDaysEdit->text(), ui->maxDaysEdit->text(), minDays, maxDays,
        QStringLiteral("Мин. дней"), QStringLiteral("Макс. дней"));
    if (!dr.ok) {
        QMessageBox::warning(this, QStringLiteral("Фильтр"), dr.errorMessage);
        return;
    }

    currentTourPointers_ =
        agency::filterTours(data.toursHead(), country, minPrice, maxPrice, minDays, maxDays);
    applySortToCurrentTours();
}

void UserWindow::refillTourTable()
{
    ui->toursTable->setRowCount(static_cast<int>(currentTourPointers_.size()));
    for (int i = 0; i < currentTourPointers_.size(); ++i) {
        setTourRow(ui->toursTable, i, currentTourPointers_[i]);
    }
    tourSort_.applyToTableHeader(ui->toursTable);
    tour_widgets::applyTourTableColumnLayout(ui->toursTable, true);
}

void UserWindow::applySortToCurrentTours()
{
    // Сортировка учитывает цепочку ключей из заголовка (мультисортировка).
    agency::sortToursByColumnOrder(currentTourPointers_, tourSort_.sortOrder());
    refillTourTable();
    statusBar()->showMessage(tourSort_.statusHint());
}

void UserWindow::onBook()
{
    const int row = ui->toursTable->currentRow();
    if (row < 0 || row >= currentTourPointers_.size()) {
        QMessageBox::information(this, QStringLiteral("Бронирование"),
                                 QStringLiteral("Выберите тур в таблице."));
        return;
    }
    // Берём тур из уже отфильтрованного/отсортированного представления таблицы.
    agency::Tour* t = currentTourPointers_[row];
    agency::AgencyData& data = agency::AgencyData::instance();
    agency::User* u = data.currentUser();
    if (!u) {
        return;
    }
    if (!data.addBooking(u->login, t->id)) {
        QMessageBox::warning(this, QStringLiteral("Бронирование"),
                             QStringLiteral("Не удалось создать бронирование."));
        return;
    }
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Бронирование"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    QMessageBox::information(this, QStringLiteral("Бронирование"),
                             QStringLiteral("Тур «%1» забронирован.").arg(agency::tourDisplayName(t)));
}

void UserWindow::onProfile()
{
    // Единая точка управления профилем: пароль, логин, выход и удаление аккаунта.
    agency::AgencyData& data = agency::AgencyData::instance();
    agency::User* u = data.currentUser();
    if (!u) {
        return;
    }

    QDialog hub(this);
    hub.setWindowTitle(QStringLiteral("Профиль"));
    hub.setMinimumWidth(320);
    auto* lay = new QVBoxLayout(&hub);
    auto* lbl = new QLabel(QStringLiteral("Логин: <b>%1</b>").arg(u->login.toHtmlEscaped()), &hub);
    lbl->setTextFormat(Qt::RichText);
    lay->addWidget(lbl);

    auto* btnPass = new QPushButton(QStringLiteral("Изменить пароль"), &hub);
    auto* btnLogin = new QPushButton(QStringLiteral("Изменить логин"), &hub);
    auto* btnOut = new QPushButton(QStringLiteral("Выйти из аккаунта"), &hub);
    auto* btnDel = new QPushButton(QStringLiteral("Удалить аккаунт"), &hub);
    lay->addWidget(btnPass);
    lay->addWidget(btnLogin);
    lay->addWidget(btnOut);
    lay->addWidget(btnDel);

    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, &hub);
    lay->addWidget(box);
    QObject::connect(box, &QDialogButtonBox::rejected, &hub, &QDialog::reject);

    QObject::connect(btnPass, &QPushButton::clicked, &hub, [&data, u, &hub]() {
        QDialog pw(&hub);
        pw.setWindowTitle(QStringLiteral("Смена пароля"));
        auto* fl = new QFormLayout(&pw);
        auto* p1 = new QLineEdit(&pw);
        auto* p2 = new QLineEdit(&pw);
        p1->setEchoMode(QLineEdit::Password);
        p2->setEchoMode(QLineEdit::Password);
        attachPasswordVisibilityToggle(p1);
        attachPasswordVisibilityToggle(p2);
        fl->addRow(QStringLiteral("Новый пароль:"), p1);
        fl->addRow(QStringLiteral("Повтор пароля:"), p2);
        auto* pbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &pw);
        fl->addRow(pbox);
        QObject::connect(pbox, &QDialogButtonBox::rejected, &pw, &QDialog::reject);
        QObject::connect(pbox, &QDialogButtonBox::accepted, &pw, [&data, u, p1, p2, &pw]() {
            if (p1->text() != p2->text()) {
                QMessageBox::warning(&pw, QStringLiteral("Пароль"), QStringLiteral("Пароли не совпадают."));
                return;
            }
            const agency::ValidationResult pv = agency::validatePassword(p1->text());
            if (!pv.ok) {
                QMessageBox::warning(&pw, QStringLiteral("Пароль"), pv.errorMessage);
                return;
            }
            if (!data.changeUserPassword(u->login, p1->text())) {
                QMessageBox::warning(&pw, QStringLiteral("Пароль"),
                                     QStringLiteral("Не удалось сменить пароль."));
                return;
            }
            if (!data.saveToFiles()) {
                QMessageBox::warning(&pw, QStringLiteral("Сохранение"),
                                     data.lastLoadMessages().join(QLatin1Char('\n')));
                return;
            }
            QMessageBox::information(&pw, QStringLiteral("Пароль"), QStringLiteral("Пароль изменён."));
            pw.accept();
        });
        pw.exec();
    });

    QObject::connect(btnLogin, &QPushButton::clicked, &hub, [&data, u, lbl, &hub]() {
        QDialog lg(&hub);
        lg.setWindowTitle(QStringLiteral("Смена логина"));
        auto* fl = new QFormLayout(&lg);
        auto* newLoginEdit = new QLineEdit(&lg);
        auto* curPass = new QLineEdit(&lg);
        curPass->setEchoMode(QLineEdit::Password);
        newLoginEdit->setPlaceholderText(QStringLiteral("Новый логин"));
        attachPasswordVisibilityToggle(curPass);
        fl->addRow(QStringLiteral("Новый логин:"), newLoginEdit);
        fl->addRow(QStringLiteral("Текущий пароль:"), curPass);
        auto* lbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &lg);
        fl->addRow(lbox);
        QObject::connect(lbox, &QDialogButtonBox::rejected, &lg, &QDialog::reject);
        QObject::connect(lbox, &QDialogButtonBox::accepted, &lg, [&data, u, newLoginEdit, curPass, lbl, &lg]() {
            const QString oldLogin = u->login;
            const QString newLogin = newLoginEdit->text().trimmed();
            if (!data.changeUserLogin(oldLogin, newLogin, curPass->text())) {
                QMessageBox::warning(
                    &lg, QStringLiteral("Логин"),
                    QStringLiteral("Не удалось сменить логин. Проверьте пароль, правила логина и занятость имени."));
                return;
            }
            if (!data.saveToFiles()) {
                QMessageBox::warning(&lg, QStringLiteral("Сохранение"),
                                     data.lastLoadMessages().join(QLatin1Char('\n')));
                return;
            }
            lbl->setText(QStringLiteral("Логин: <b>%1</b>").arg(u->login.toHtmlEscaped()));
            QMessageBox::information(&lg, QStringLiteral("Логин"), QStringLiteral("Логин изменён."));
            lg.accept();
        });
        lg.exec();
    });

    QObject::connect(btnOut, &QPushButton::clicked, &hub, [this, &hub]() {
        hub.close();
        onLogout();
    });

    QObject::connect(btnDel, &QPushButton::clicked, &hub, [&data, u, &hub, this]() {
        const QString loginCopy = u->login;
        const auto ret = QMessageBox::question(
            &hub, QStringLiteral("Удаление"),
            QStringLiteral("Удалить учётную запись? Бронирования будут удалены."));
        if (ret != QMessageBox::Yes) {
            return;
        }
        data.removeUserByLogin(loginCopy);
        if (!data.saveToFiles()) {
            QMessageBox::warning(&hub, QStringLiteral("Сохранение"),
                                 data.lastLoadMessages().join(QLatin1Char('\n')));
        }
        hub.accept();
        close();
    });

    hub.exec();
}

void UserWindow::onBookings()
{
    agency::AgencyData& data = agency::AgencyData::instance();
    agency::User* u = data.currentUser();
    if (!u) {
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("История бронирований"));
    dlg.resize(640, 360);
    auto* lay = new QVBoxLayout(&dlg);
    auto* table = new QTableWidget(&dlg);
    // Для обычного пользователя показываем только понятные поля без технических ID.
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("Тур"), QStringLiteral("Статус"), QStringLiteral("Действие")});
    lay->addWidget(table);

    QVector<agency::Booking*> list = data.bookingsForUser(u->login);

    table->setRowCount(static_cast<int>(list.size()));
    for (int i = 0; i < list.size(); ++i) {
        agency::Booking* b = list[i];
        agency::Tour* t = data.findTourById(b->tourId);
        table->setItem(i, 0, new QTableWidgetItem(agency::tourDisplayName(t)));
        table->setItem(i, 1,
                       new QTableWidgetItem(b->cancelled ? QStringLiteral("Отменено")
                                                         : QStringLiteral("Активно")));

        auto* cancelBtn = new QPushButton(QStringLiteral("Отменить"), &dlg);
        if (b->cancelled) {
            cancelBtn->setEnabled(false);
        }
        const int bid = b->id;
        QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, [&data, bid, u, &dlg]() {
            if (data.cancelBookingForUser(bid, u->login)) {
                if (!data.saveToFiles()) {
                    QMessageBox::warning(&dlg, QStringLiteral("Сохранение"),
                                         data.lastLoadMessages().join(QLatin1Char('\n')));
                } else {
                    QMessageBox::information(&dlg, QStringLiteral("Бронирование"),
                                             QStringLiteral("Бронирование отменено."));
                }
                dlg.accept();
            }
        });
        table->setCellWidget(i, 2, cancelBtn);
    }

    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    if (QPushButton* closeBtn = box->button(QDialogButtonBox::Close)) {
        closeBtn->setText(QStringLiteral("Закрыть"));
    }
    lay->addWidget(box);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
    refreshTours();
}

void UserWindow::onLogout()
{
    agency::AgencyData& data = agency::AgencyData::instance();
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
    }
    close();
}


