#include "dialogs/scheduledialog.h"
#include "ui_scheduledialog.h"
#include <QMessageBox>
#include <QDate>

ScheduleDialog::ScheduleDialog(QWidget *parent, TransportSchedule* schedule)
    : QDialog(parent)
    , ui(std::make_unique<Ui::ScheduleDialog>())
    , schedule_(schedule)
{
    ui->setupUi(this);
    
    ui->departureDateEdit->setDate(QDate::currentDate());
    ui->arrivalDateEdit->setDate(QDate::currentDate().addDays(1));
    
    if (schedule_) {
        ui->departureCityEdit->setText(schedule_->departureCity);
        ui->arrivalCityEdit->setText(schedule_->arrivalCity);
        ui->departureDateEdit->setDate(schedule_->departureDate);
        ui->arrivalDateEdit->setDate(schedule_->arrivalDate);
        ui->priceSpin->setValue(schedule_->price);
        ui->seatsSpin->setValue(schedule_->availableSeats);
    }
    
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ScheduleDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ScheduleDialog::reject);
    
    connect(ui->departureDateEdit, &QDateEdit::dateChanged, this, [this]() {
        if (ui->departureDateEdit->date() > ui->arrivalDateEdit->date()) {
            ui->arrivalDateEdit->setDate(ui->departureDateEdit->date().addDays(1));
        }
    });
}

ScheduleDialog::~ScheduleDialog() = default;

TransportSchedule ScheduleDialog::getSchedule() const {
    TransportSchedule schedule;
    schedule.departureCity = ui->departureCityEdit->text();
    schedule.arrivalCity = ui->arrivalCityEdit->text();
    schedule.departureDate = ui->departureDateEdit->date();
    schedule.arrivalDate = ui->arrivalDateEdit->date();
    schedule.price = ui->priceSpin->value();
    schedule.availableSeats = ui->seatsSpin->value();
    return schedule;
}

void ScheduleDialog::accept() {
    if (ui->departureCityEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите город отправления");
        return;
    }
    if (ui->arrivalCityEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите город прибытия");
        return;
    }
    if (ui->departureDateEdit->date() >= ui->arrivalDateEdit->date()) {
        QMessageBox::warning(this, "Ошибка", "Дата прибытия должна быть позже даты отправления");
        return;
    }
    QDialog::accept();
}









