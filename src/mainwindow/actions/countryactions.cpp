#include "mainwindow/actions/countryactions.h"
#include "dialogs/countrydialog.h"
#include "mainwindow/tablemanager.h"
#include "mainwindow/filtercomboupdater.h"
#include "mainwindow/filtermanager.h"
#include <QTableWidget>
#include <QMessageBox>
#include <QTableWidgetItem>

AddCountryAction::AddCountryAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent)
    : Action(parent)
    , countries_(countries)
    , table_(table)
    , parent_(parent)
{
}

void AddCountryAction::execute() {
    CountryDialog dialog(parent_);
    if (dialog.exec() == QDialog::Accepted) {
        Country country = dialog.getCountry();
        countries_->add(country);
        emit executed();
    }
}

EditCountryAction::EditCountryAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent)
    : Action(parent)
    , countries_(countries)
    , table_(table)
    , parent_(parent)
{
}

void EditCountryAction::execute() {
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::warning(parent_, "Предупреждение", "Выберите страну для редактирования");
        return;
    }
    
    QTableWidgetItem* item = table_->item(row, 0);
    if (!item) {
        return;
    }
    
    int dataIndex = item->data(Qt::UserRole).toInt();
    Country* country = countries_->get(dataIndex);
    if (!country) {
        return;
    }
    
    CountryDialog dialog(parent_, country);
    if (dialog.exec() == QDialog::Accepted) {
        *country = dialog.getCountry();
        emit executed();
    }
}

DeleteCountryAction::DeleteCountryAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent)
    : Action(parent)
    , countries_(countries)
    , table_(table)
    , parent_(parent)
{
}

void DeleteCountryAction::execute() {
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::warning(parent_, "Предупреждение", "Выберите страну для удаления");
        return;
    }
    
    QTableWidgetItem* item = table_->item(row, 0);
    if (!item) {
        return;
    }
    
    int dataIndex = item->data(Qt::UserRole).toInt();
    
    int ret = QMessageBox::question(parent_, "Подтверждение", 
                                    "Вы уверены, что хотите удалить эту страну?",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        countries_->remove(dataIndex);
        emit executed();
    }
}

ShowCountryInfoAction::ShowCountryInfoAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent)
    : Action(parent)
    , countries_(countries)
    , table_(table)
    , parent_(parent)
{
}

void ShowCountryInfoAction::execute() {
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::warning(parent_, "Предупреждение", "Выберите страну для просмотра информации");
        return;
    }
    
    QTableWidgetItem* item = table_->item(row, 0);
    if (!item) {
        return;
    }
    
    int dataIndex = item->data(Qt::UserRole).toInt();
    Country* country = countries_->get(dataIndex);
    if (!country) {
        return;
    }
    
    QString info = QString("Название: %1\nКонтинент: %2\nСтолица: %3\nВалюта: %4")
        .arg(country->getName())
        .arg(country->getContinent())
        .arg(country->getCapital())
        .arg(country->getCurrency());
    
    QMessageBox::information(parent_, "Информация о стране", info);
}

RefreshCountriesAction::RefreshCountriesAction()
    : Action()
{
}

void RefreshCountriesAction::execute() {
    emit executed();
}


