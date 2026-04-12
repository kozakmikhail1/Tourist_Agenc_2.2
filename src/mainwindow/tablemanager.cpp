#include "mainwindow/tablemanager.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include "utils/numericsortitem.h"

TableManager::TableManager() = default;

void TableManager::updateCountriesTable(QTableWidget* table, const DataContainer<Country>& countries) {
    table->setRowCount(countries.size());
    
    int row = 0;
    for (const auto& country : countries.getData()) {
        QTableWidgetItem* nameItem = new QTableWidgetItem(country.getName());
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 0, nameItem);
        
        QTableWidgetItem* continentItem = new QTableWidgetItem(country.getContinent());
        continentItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, continentItem);
        
        QTableWidgetItem* capitalItem = new QTableWidgetItem(country.getCapital());
        capitalItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 2, capitalItem);
        
        QTableWidgetItem* currencyItem = new QTableWidgetItem(country.getCurrency());
        currencyItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 3, currencyItem);
        ++row;
    }
}

void TableManager::updateHotelsTable(QTableWidget* table, const DataContainer<Hotel>& hotels) {
    table->setRowCount(hotels.size());
    
    int row = 0;
    for (const auto& hotel : hotels.getData()) {
        QTableWidgetItem* nameItem = new QTableWidgetItem(hotel.getName());
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 0, nameItem);
        
        QTableWidgetItem* countryItem = new QTableWidgetItem(hotel.getCountry());
        countryItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, countryItem);
        
        QTableWidgetItem* starsItem = new QTableWidgetItem(QString::number(hotel.getStars()));
        starsItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        table->setItem(row, 2, starsItem);
        
        QTableWidgetItem* addressItem = new QTableWidgetItem(hotel.getAddress());
        addressItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 3, addressItem);
        
        QTableWidgetItem* roomsItem = new QTableWidgetItem(QString::number(hotel.getRoomCount()));
        roomsItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        table->setItem(row, 4, roomsItem);
        ++row;
    }
}

void TableManager::updateTransportCompaniesTable(QTableWidget* table, const DataContainer<TransportCompany>& companies) {
    table->setRowCount(companies.size());
    
    int row = 0;
    for (const auto& company : companies.getData()) {
        QTableWidgetItem* nameItem = new QTableWidgetItem(company.getName());
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 0, nameItem);
        
        QTableWidgetItem* typeItem = new QTableWidgetItem(TransportCompany::transportTypeToString(company.getTransportType()));
        typeItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, typeItem);
        
        QTableWidgetItem* scheduleItem = new QTableWidgetItem(QString::number(company.getScheduleCount()));
        scheduleItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        table->setItem(row, 2, scheduleItem);
        ++row;
    }
}

void TableManager::updateToursTable(QTableWidget* table, const DataContainer<Tour>& tours) {
    table->setRowCount(tours.size());
    
    int row = 0;
    for (const auto& tour : tours.getData()) {
        QTableWidgetItem* nameItem = new QTableWidgetItem(tour.getName());
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 0, nameItem);
        
        QTableWidgetItem* countryItem = new QTableWidgetItem(tour.getCountry());
        countryItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, countryItem);
        
        QTableWidgetItem* startDateItem = new QTableWidgetItem(tour.getStartDate().toString("yyyy-MM-dd"));
        startDateItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        table->setItem(row, 2, startDateItem);
        
        QTableWidgetItem* endDateItem = new QTableWidgetItem(tour.getEndDate().toString("yyyy-MM-dd"));
        endDateItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        table->setItem(row, 3, endDateItem);
        
        double cost = tour.calculateCost();
        NumericSortItem* costItem = new NumericSortItem(QString::number(cost, 'f', 2) + " руб", cost);
        costItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        table->setItem(row, 4, costItem);
        ++row;
    }
}

void TableManager::updateOrdersTable(QTableWidget* table, const DataContainer<Order>& orders) {
    table->setRowCount(orders.size());
    
    int row = 0;
    for (const auto& order : orders.getData()) {
        QTableWidgetItem* tourItem = new QTableWidgetItem(order.getTour().getName());
        tourItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 0, tourItem);
        
        QTableWidgetItem* clientItem = new QTableWidgetItem(order.getClientName());
        clientItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, clientItem);
        
        QTableWidgetItem* phoneItem = new QTableWidgetItem(order.getClientPhone());
        phoneItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 2, phoneItem);
        
        QTableWidgetItem* emailItem = new QTableWidgetItem(order.getClientEmail());
        emailItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 3, emailItem);
        
        double cost = order.getTotalCost();
        NumericSortItem* costItem = new NumericSortItem(QString::number(cost, 'f', 2) + " руб", cost);
        costItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        table->setItem(row, 4, costItem);
        
        QTableWidgetItem* statusItem = new QTableWidgetItem(order.getStatus());
        statusItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 5, statusItem);
        ++row;
    }
}

int TableManager::getSelectedRow(QTableWidget* table) const {
    return table->currentRow();
}

int TableManager::getSelectedIndex(QTableWidget* table, int column) const {
    int row = getSelectedRow(table);
    if (row < 0) {
        return -1;
    }
    
    QTableWidgetItem* item = table->item(row, column);
    if (!item) {
        return -1;
    }
    
    return item->data(Qt::UserRole).toInt();
}


