#include "adminwindow.h"
#include "ui_adminwindow.h"
#include "agency/agencydata.h"
#include "agency/algorithms.h"
#include "agency/validation.h"
#include "utils/passwordtoggle.h"
#include "utils/toursortheaderview.h"
#include "utils/tourtabledisplay.h"

#include <algorithm>

#include <QCheckBox>
#include <QCloseEvent>
#include <QDate>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

int countAdmins(agency::User* head)
{
    // Нужен для бизнес-правила: в системе всегда должен оставаться хотя бы один админ.
    int n = 0;
    for (agency::User* u = head; u; u = u->next) {
        if (u->isAdmin) {
            ++n;
        }
    }
    return n;
}

void setTourRow(QTableWidget* table, int row, const agency::Tour* t)
{
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

AdminWindow::AdminWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::AdminWindow>())
{
    ui->setupUi(this);
    ui->usersTable->setColumnCount(3);
    ui->usersTable->setHorizontalHeaderLabels({QStringLiteral("Логин"), QStringLiteral("Админ"),
                                               QStringLiteral("Забронированные туры (активные)")});
    ui->usersTable->horizontalHeader()->setStretchLastSection(true);

    auto* tourHeader = new TourSortHeaderView(Qt::Horizontal, ui->toursTable);
    ui->toursTable->setHorizontalHeader(tourHeader);
    ui->toursTable->setColumnCount(agency::TourTableColumns::Count);
    tourSort_.applyToTableHeader(ui->toursTable);
    tour_widgets::applyTourTableColumnLayout(ui->toursTable, false);

    connect(tourHeader, &TourSortHeaderView::sortSectionClicked, this,
            [this](int logicalIndex, Qt::KeyboardModifiers modifiers) {
                tourSort_.onHeaderClicked(logicalIndex, modifiers);
                applySortToAdminTours();
            });

    connect(ui->addUserButton, &QPushButton::clicked, this, &AdminWindow::onAddUser);
    connect(ui->editUserButton, &QPushButton::clicked, this, &AdminWindow::onEditUser);
    connect(ui->deleteUserButton, &QPushButton::clicked, this, &AdminWindow::onDeleteUser);
    connect(ui->userBookingsButton, &QPushButton::clicked, this, &AdminWindow::onUserBookings);
    connect(ui->addTourButton, &QPushButton::clicked, this, &AdminWindow::onAddTour);
    connect(ui->editTourButton, &QPushButton::clicked, this, &AdminWindow::onEditTour);
    connect(ui->deleteTourButton, &QPushButton::clicked, this, &AdminWindow::onDeleteTour);
    connect(ui->tourBookingsButton, &QPushButton::clicked, this, &AdminWindow::onTourBookings);
    connect(ui->adminFilterButton, &QPushButton::clicked, this, &AdminWindow::onFilterTours);
    connect(ui->logoutButton, &QPushButton::clicked, this, &AdminWindow::onLogout);

    refreshUsers();
    onFilterTours();
    statusBar()->showMessage(tourSort_.statusHint());
}

AdminWindow::~AdminWindow() = default;

void AdminWindow::closeEvent(QCloseEvent* event)
{
    agency::AgencyData& data = agency::AgencyData::instance();
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
    }
    QMainWindow::closeEvent(event);
}

void AdminWindow::refreshUsers()
{
    // Обновляем список пользователей и краткую сводку активных бронирований.
    agency::AgencyData& data = agency::AgencyData::instance();
    QVector<agency::User*> users;
    for (agency::User* u = data.usersHead(); u; u = u->next) {
        users.push_back(u);
    }
    ui->usersTable->setRowCount(static_cast<int>(users.size()));
    for (int i = 0; i < users.size(); ++i) {
        ui->usersTable->setItem(i, 0, new QTableWidgetItem(users[i]->login));
        ui->usersTable->setItem(i, 1,
                                new QTableWidgetItem(users[i]->isAdmin ? QStringLiteral("да")
                                                                       : QStringLiteral("нет")));
        auto* bookCell = new QTableWidgetItem(data.bookedActiveToursSummaryForUser(users[i]->login));
        bookCell->setToolTip(bookCell->text());
        ui->usersTable->setItem(i, 2, bookCell);
    }
}

void AdminWindow::refillToursTable()
{
    ui->toursTable->setRowCount(static_cast<int>(adminTourView_.size()));
    for (int i = 0; i < adminTourView_.size(); ++i) {
        setTourRow(ui->toursTable, i, adminTourView_[i]);
    }
    tourSort_.applyToTableHeader(ui->toursTable);
    tour_widgets::applyTourTableColumnLayout(ui->toursTable, false);
}

void AdminWindow::applySortToAdminTours()
{
    // Применяем текущую цепочку сортировки, заданную кликами по заголовкам.
    agency::sortToursByColumnOrder(adminTourView_, tourSort_.sortOrder());
    refillToursTable();
    statusBar()->showMessage(tourSort_.statusHint());
}

void AdminWindow::onFilterTours()
{
    // Фильтр администратора по стране, диапазону цены и диапазону длительности.
    agency::AgencyData& data = agency::AgencyData::instance();
    const QString country = ui->adminCountryEdit->text();

    int minPrice = 0;
    int maxPrice = 0;
    const agency::ValidationResult pr = agency::parseOptionalIntRange(
        ui->adminMinPriceEdit->text(), ui->adminMaxPriceEdit->text(), minPrice, maxPrice,
        QStringLiteral("Мин. цена"), QStringLiteral("Макс. цена"));
    if (!pr.ok) {
        QMessageBox::warning(this, QStringLiteral("Фильтр"), pr.errorMessage);
        return;
    }
    int minDays = 0;
    int maxDays = 0;
    const agency::ValidationResult dr = agency::parseOptionalIntRange(
        ui->adminMinDaysEdit->text(), ui->adminMaxDaysEdit->text(), minDays, maxDays,
        QStringLiteral("Мин. дней"), QStringLiteral("Макс. дней"));
    if (!dr.ok) {
        QMessageBox::warning(this, QStringLiteral("Фильтр"), dr.errorMessage);
        return;
    }

    adminTourView_ =
        agency::filterTours(data.toursHead(), country, minPrice, maxPrice, minDays, maxDays);
    applySortToAdminTours();
}

void AdminWindow::onUserBookings()
{
    const int row = ui->usersTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Пользователи"),
                                 QStringLiteral("Выберите пользователя."));
        return;
    }
    const QString login = ui->usersTable->item(row, 0)->text();
    agency::AgencyData& data = agency::AgencyData::instance();
    const QVector<agency::Booking*> list = data.bookingsForUser(login);

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Бронирования: %1").arg(login));
    dlg.resize(720, 400);
    auto* lay = new QVBoxLayout(&dlg);
    auto* table = new QTableWidget(&dlg);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("ID брони"), QStringLiteral("Тур ID"), QStringLiteral("Тур"),
         QStringLiteral("Статус")});
    table->setRowCount(static_cast<int>(list.size()));
    for (int i = 0; i < list.size(); ++i) {
        agency::Booking* b = list[i];
        agency::Tour* t = data.findTourById(b->tourId);
        table->setItem(i, 0, new QTableWidgetItem(QString::number(b->id)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(b->tourId)));
        table->setItem(i, 2, new QTableWidgetItem(agency::tourDisplayName(t)));
        table->setItem(i, 3,
                       new QTableWidgetItem(b->cancelled ? QStringLiteral("Отменено")
                                                         : QStringLiteral("Активно")));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    lay->addWidget(table);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    lay->addWidget(box);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
}

void AdminWindow::onTourBookings()
{
    const int row = ui->toursTable->currentRow();
    if (row < 0 || row >= adminTourView_.size()) {
        QMessageBox::information(this, QStringLiteral("Туры"), QStringLiteral("Выберите тур."));
        return;
    }
    const int tourId = adminTourView_[row]->id;
    agency::AgencyData& data = agency::AgencyData::instance();
    agency::Tour* tour = data.findTourById(tourId);
    const QVector<agency::Booking*> list = data.bookingsForTour(tourId);

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Бронирования тура ID %1%2")
                             .arg(tourId)
                             .arg(tour ? QStringLiteral(" - %1").arg(agency::tourDisplayName(tour))
                                       : QString()));
    dlg.resize(560, 400);
    auto* lay = new QVBoxLayout(&dlg);
    auto* table = new QTableWidget(&dlg);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("ID брони"), QStringLiteral("Пользователь"), QStringLiteral("Статус")});
    table->setRowCount(static_cast<int>(list.size()));
    for (int i = 0; i < list.size(); ++i) {
        agency::Booking* b = list[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::number(b->id)));
        table->setItem(i, 1, new QTableWidgetItem(b->userLogin));
        table->setItem(i, 2,
                       new QTableWidgetItem(b->cancelled ? QStringLiteral("Отменено")
                                                         : QStringLiteral("Активно")));
    }
    table->horizontalHeader()->setStretchLastSection(true);
    lay->addWidget(table);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    lay->addWidget(box);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlg.exec();
}

void AdminWindow::onAddUser()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Новый пользователь"));
    auto* lay = new QFormLayout(&dlg);
    auto* loginEdit = new QLineEdit(&dlg);
    auto* passEdit = new QLineEdit(&dlg);
    passEdit->setEchoMode(QLineEdit::Password);
    attachPasswordVisibilityToggle(passEdit);
    auto* adminBox = new QCheckBox(QStringLiteral("Администратор"), &dlg);
    lay->addRow(QStringLiteral("Логин:"), loginEdit);
    lay->addRow(QStringLiteral("Пароль:"), passEdit);
    lay->addRow(adminBox);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    lay->addRow(box);
    QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    const QString newLogin = loginEdit->text().trimmed();
    const QString newPass = passEdit->text();
    const agency::ValidationResult lv = agency::validateLogin(newLogin);
    if (!lv.ok) {
        QMessageBox::warning(this, QStringLiteral("Пользователь"), lv.errorMessage);
        return;
    }
    const agency::ValidationResult pv = agency::validatePassword(newPass);
    if (!pv.ok) {
        QMessageBox::warning(this, QStringLiteral("Пользователь"), pv.errorMessage);
        return;
    }
    agency::AgencyData& data = agency::AgencyData::instance();
    if (!data.registerUser(newLogin, newPass, adminBox->isChecked())) {
        QMessageBox::warning(this, QStringLiteral("Пользователь"),
                             QStringLiteral("Этот логин уже занят."));
        return;
    }
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    refreshUsers();
}

void AdminWindow::onEditUser()
{
    const int row = ui->usersTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Пользователи"),
                                 QStringLiteral("Выберите пользователя."));
        return;
    }
    const QString login = ui->usersTable->item(row, 0)->text();

    agency::AgencyData& data = agency::AgencyData::instance();
    agency::User* self = data.currentUser();

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Пользователь: %1").arg(login));
    auto* lay = new QFormLayout(&dlg);
    auto* passEdit = new QLineEdit(&dlg);
    passEdit->setEchoMode(QLineEdit::Password);
    attachPasswordVisibilityToggle(passEdit);
    passEdit->setPlaceholderText(QStringLiteral("Новый пароль (необязательно)"));
    auto* adminBox = new QCheckBox(QStringLiteral("Администратор"), &dlg);
    agency::User* target = data.findUserByLogin(login);
    if (target) {
        adminBox->setChecked(target->isAdmin);
    }
    lay->addRow(QStringLiteral("Новый пароль:"), passEdit);
    lay->addRow(adminBox);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    lay->addRow(box);
    QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    if (target && self && target->login == self->login && !adminBox->isChecked()
        && target->isAdmin && countAdmins(data.usersHead()) <= 1) {
        QMessageBox::warning(this, QStringLiteral("Пользователи"),
                             QStringLiteral("Нельзя снять права у единственного администратора."));
        return;
    }

    if (!passEdit->text().isEmpty()) {
        const agency::ValidationResult pv = agency::validatePassword(passEdit->text());
        if (!pv.ok) {
            QMessageBox::warning(this, QStringLiteral("Пользователь"), pv.errorMessage);
            return;
        }
        if (!data.changeUserPassword(login, passEdit->text())) {
            QMessageBox::warning(this, QStringLiteral("Пользователь"),
                                 QStringLiteral("Не удалось сменить пароль."));
            return;
        }
    }
    data.updateUserAdminFlag(login, adminBox->isChecked());
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    refreshUsers();
}

void AdminWindow::onDeleteUser()
{
    const int row = ui->usersTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("Пользователи"),
                                 QStringLiteral("Выберите пользователя."));
        return;
    }
    const QString login = ui->usersTable->item(row, 0)->text();

    agency::AgencyData& data = agency::AgencyData::instance();
    agency::User* target = data.findUserByLogin(login);
    if (!target) {
        return;
    }
    if (target->isAdmin && countAdmins(data.usersHead()) <= 1) {
        QMessageBox::warning(this, QStringLiteral("Пользователи"),
                             QStringLiteral("Нельзя удалить единственного администратора."));
        return;
    }
    if (data.currentUser() && data.currentUser()->login == login) {
        QMessageBox::warning(this, QStringLiteral("Пользователи"),
                             QStringLiteral("Удалите себя через профиль пользователя или создайте другого админа."));
        return;
    }

    if (QMessageBox::question(this, QStringLiteral("Удаление"),
                              QStringLiteral("Удалить пользователя %1?").arg(login))
        != QMessageBox::Yes) {
        return;
    }
    data.removeUserByLogin(login);
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    refreshUsers();
}

void AdminWindow::onAddTour()
{
    // Создание нового тура через диалог: сначала валидация, затем запись в хранилище.
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Новый тур"));
    auto* lay = new QFormLayout(&dlg);
    auto* title = new QLineEdit(&dlg);
    auto* country = new QLineEdit(&dlg);
    auto* price = new QSpinBox(&dlg);
    price->setRange(1, 50000000);
    auto* days = new QSpinBox(&dlg);
    days->setRange(1, 3650);
    auto* hotel = new QLineEdit(&dlg);
    auto* address = new QLineEdit(&dlg);
    auto* stars = new QSpinBox(&dlg);
    stars->setRange(1, 5);
    stars->setValue(4);
    auto* dateStart = new QDateEdit(&dlg);
    auto* dateEnd = new QDateEdit(&dlg);
    dateStart->setCalendarPopup(true);
    dateEnd->setCalendarPopup(true);
    dateStart->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateEnd->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    const QDate today = QDate::currentDate();
    dateStart->setDate(today);
    dateEnd->setDate(today.addDays(6));
    lay->addRow(QStringLiteral("Название тура:"), title);
    lay->addRow(QStringLiteral("Страна:"), country);
    lay->addRow(QStringLiteral("Цена:"), price);
    lay->addRow(QStringLiteral("Дней:"), days);
    lay->addRow(QStringLiteral("Отель:"), hotel);
    lay->addRow(QStringLiteral("Адрес отеля:"), address);
    lay->addRow(QStringLiteral("Звёзды (1–5):"), stars);
    lay->addRow(QStringLiteral("Дата начала:"), dateStart);
    lay->addRow(QStringLiteral("Дата окончания:"), dateEnd);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    lay->addRow(box);
    QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    agency::AgencyData& data = agency::AgencyData::instance();
    const int id = data.nextTourId();
    const QString n = title->text().trimmed();
    const QString c = country->text().trimmed();
    const QString h = hotel->text().trimmed();
    const QString addr = address->text().trimmed();
    const QString ds = dateStart->date().toString(Qt::ISODate);
    const QString de = dateEnd->date().toString(Qt::ISODate);
    if (n.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Туры"), QStringLiteral("Укажите название тура."));
        return;
    }
    const agency::ValidationResult tv =
        agency::validateTourFields(c, price->value(), days->value(), h, addr, stars->value(), ds, de);
    if (!tv.ok) {
        QMessageBox::warning(this, QStringLiteral("Туры"), tv.errorMessage);
        return;
    }
    if (!data.addTour(id, n, c, price->value(), days->value(), h, addr, stars->value(), ds, de)) {
        QMessageBox::warning(this, QStringLiteral("Туры"),
                             QStringLiteral("Не удалось добавить тур. Проверьте название и остальные поля."));
        return;
    }
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    onFilterTours();
}

void AdminWindow::onEditTour()
{
    // Редактируем выбранный тур и сохраняем только после повторной валидации полей.
    const int row = ui->toursTable->currentRow();
    if (row < 0 || row >= adminTourView_.size()) {
        QMessageBox::information(this, QStringLiteral("Туры"), QStringLiteral("Выберите тур."));
        return;
    }
    agency::Tour* t = adminTourView_[row];

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Редактирование тура"));
    auto* lay = new QFormLayout(&dlg);
    auto* title = new QLineEdit(t->title.trimmed().isEmpty() ? agency::tourDisplayName(t) : t->title, &dlg);
    auto* country = new QLineEdit(t->country, &dlg);
    auto* price = new QSpinBox(&dlg);
    price->setRange(1, 50000000);
    price->setValue(t->price);
    auto* days = new QSpinBox(&dlg);
    days->setRange(1, 3650);
    days->setValue(t->days);
    auto* hotel = new QLineEdit(t->hotel, &dlg);
    auto* address = new QLineEdit(t->hotelAddress, &dlg);
    auto* stars = new QSpinBox(&dlg);
    stars->setRange(1, 5);
    stars->setValue(std::max(1, t->hotelStars));
    QDate dsInit = QDate::fromString(t->dateStart, Qt::ISODate);
    QDate deInit = QDate::fromString(t->dateEnd, Qt::ISODate);
    if (!dsInit.isValid()) {
        dsInit = QDate::currentDate();
    }
    if (!deInit.isValid()) {
        deInit = dsInit;
    }
    auto* dateStart = new QDateEdit(dsInit, &dlg);
    auto* dateEnd = new QDateEdit(deInit, &dlg);
    dateStart->setCalendarPopup(true);
    dateEnd->setCalendarPopup(true);
    dateStart->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    dateEnd->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    lay->addRow(QStringLiteral("Название тура:"), title);
    lay->addRow(QStringLiteral("Страна:"), country);
    lay->addRow(QStringLiteral("Цена:"), price);
    lay->addRow(QStringLiteral("Дней:"), days);
    lay->addRow(QStringLiteral("Отель:"), hotel);
    lay->addRow(QStringLiteral("Адрес отеля:"), address);
    lay->addRow(QStringLiteral("Звёзды (1–5):"), stars);
    lay->addRow(QStringLiteral("Дата начала:"), dateStart);
    lay->addRow(QStringLiteral("Дата окончания:"), dateEnd);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    lay->addRow(box);
    QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    agency::AgencyData& data = agency::AgencyData::instance();
    const QString n = title->text().trimmed();
    const QString c = country->text().trimmed();
    const QString h = hotel->text().trimmed();
    const QString addr = address->text().trimmed();
    const QString ds = dateStart->date().toString(Qt::ISODate);
    const QString de = dateEnd->date().toString(Qt::ISODate);
    if (n.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Туры"), QStringLiteral("Укажите название тура."));
        return;
    }
    if (!data.updateTour(t->id, n, c, price->value(), days->value(), h, addr, stars->value(), ds, de)) {
        QMessageBox::warning(this, QStringLiteral("Туры"),
                             QStringLiteral("Не удалось сохранить тур. Проверьте название и остальные поля."));
        return;
    }
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    onFilterTours();
}

void AdminWindow::onDeleteTour()
{
    const int row = ui->toursTable->currentRow();
    if (row < 0 || row >= adminTourView_.size()) {
        QMessageBox::information(this, QStringLiteral("Туры"), QStringLiteral("Выберите тур."));
        return;
    }
    const int id = adminTourView_[row]->id;
    if (QMessageBox::question(this, QStringLiteral("Туры"),
                              QStringLiteral("Удалить тур ID %1? Связанные бронирования будут удалены.").arg(id))
        != QMessageBox::Yes) {
        return;
    }
    agency::AgencyData& data = agency::AgencyData::instance();
    data.deleteTourById(id);
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    onFilterTours();
}

void AdminWindow::onLogout()
{
    agency::AgencyData& data = agency::AgencyData::instance();
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Сохранение"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
    }
    close();
}



