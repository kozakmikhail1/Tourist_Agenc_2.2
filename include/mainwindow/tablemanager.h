#ifndef TABLEMANAGER_H
#define TABLEMANAGER_H

#include <QTableWidget>
#include "containers/datacontainer.h"
#include "models/country.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include "models/tour.h"
#include "models/order.h"

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class TableManager {
public:
    TableManager();
    
    void updateCountriesTable(QTableWidget* table, const DataContainer<Country>& countries);
    void updateHotelsTable(QTableWidget* table, const DataContainer<Hotel>& hotels);
    void updateTransportCompaniesTable(QTableWidget* table, const DataContainer<TransportCompany>& companies);
    void updateToursTable(QTableWidget* table, const DataContainer<Tour>& tours);
    void updateOrdersTable(QTableWidget* table, const DataContainer<Order>& orders);
    
    int getSelectedRow(QTableWidget* table) const;
    int getSelectedIndex(QTableWidget* table, int column) const;
};

#endif


