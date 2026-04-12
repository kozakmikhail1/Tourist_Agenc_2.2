#include "dialogs/companydialog.h"
#include "ui_companydialog.h"
#include "dialogs/scheduledialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QDateEdit>

CompanyDialog::CompanyDialog(QWidget *parent, TransportCompany* company)
    : QDialog(parent)
    , ui(std::make_unique<Ui::CompanyDialog>())
    , company_(company)
{
    ui->setupUi(this);
    
    ui->typeCombo->addItems({"Самолет", "Автобус", "Поезд", "Корабль"});
    
    if (company_) {
        ui->nameEdit->setText(company_->getName());
        int index = ui->typeCombo->findText(
            TransportCompany::transportTypeToString(company_->getTransportType()));
        if (index >= 0) ui->typeCombo->setCurrentIndex(index);
        
        schedules_ = company_->getSchedules();
        updateSchedulesTable();
    }
    
    connect(ui->addScheduleButton, &QPushButton::clicked, this, &CompanyDialog::addSchedule);
    connect(ui->editScheduleButton, &QPushButton::clicked, this, &CompanyDialog::editSchedule);
    connect(ui->deleteScheduleButton, &QPushButton::clicked, this, &CompanyDialog::deleteSchedule);
    
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CompanyDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &CompanyDialog::reject);
}

CompanyDialog::~CompanyDialog() = default;

TransportCompany CompanyDialog::getCompany() const {
    TransportCompany company;
    company.setName(ui->nameEdit->text());
    company.setTransportType(
        TransportCompany::stringToTransportType(ui->typeCombo->currentText()));
    
    for (const auto& schedule : schedules_) {
        company.addSchedule(schedule);
    }
    
    return company;
}

void CompanyDialog::accept() {
    if (ui->nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название компании");
        return;
    }
    QDialog::accept();
}

void CompanyDialog::addSchedule() {
    ScheduleDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        TransportSchedule schedule = dialog.getSchedule();
        schedules_.append(schedule);
        updateSchedulesTable();
    }
}

void CompanyDialog::editSchedule() {
    int row = ui->schedulesTable->currentRow();
    if (row < 0 || row >= schedules_.size()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите рейс для редактирования");
        return;
    }
    
    TransportSchedule& schedule = schedules_[row];
    ScheduleDialog dialog(this, &schedule);
    if (dialog.exec() == QDialog::Accepted) {
        schedule = dialog.getSchedule();
        updateSchedulesTable();
    }
}

void CompanyDialog::deleteSchedule() {
    int row = ui->schedulesTable->currentRow();
    if (row < 0 || row >= schedules_.size()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите рейс для удаления");
        return;
    }
    
    schedules_.removeAt(row);
    updateSchedulesTable();
}

void CompanyDialog::updateSchedulesTable() {
    ui->schedulesTable->setRowCount(schedules_.size());
    
    int row = 0;
    for (const auto& schedule : schedules_) {
        ui->schedulesTable->setItem(row, 0, new QTableWidgetItem(schedule.departureCity));
        ui->schedulesTable->setItem(row, 1, new QTableWidgetItem(schedule.arrivalCity));
        ui->schedulesTable->setItem(row, 2, 
            new QTableWidgetItem(schedule.departureDate.toString("yyyy-MM-dd")));
        ui->schedulesTable->setItem(row, 3, 
            new QTableWidgetItem(schedule.arrivalDate.toString("yyyy-MM-dd")));
        ui->schedulesTable->setItem(row, 4, 
            new QTableWidgetItem(QString::number(schedule.price, 'f', 2)));
        ui->schedulesTable->setItem(row, 5, 
            new QTableWidgetItem(QString::number(schedule.availableSeats)));
        ++row;
    }
}

