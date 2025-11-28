#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H

#include <QString>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE

// Вспомогательный класс для управления фильтрацией
class FilterManager {
public:
    FilterManager();
    
    void applyCountriesFilters(QTableWidget* table,
                              QLineEdit* searchEdit,
                              QComboBox* continentCombo,
                              QComboBox* currencyCombo);
    void applyHotelsFilters(QTableWidget* table,
                           QLineEdit* searchEdit,
                           QComboBox* countryCombo,
                           QComboBox* starsCombo);
    void applyTransportFilters(QTableWidget* table,
                             QLineEdit* searchEdit,
                             QComboBox* typeCombo);
    void applyToursFilters(QTableWidget* table,
                          QLineEdit* searchEdit,
                          QComboBox* countryCombo,
                          QLineEdit* minPriceEdit,
                          QLineEdit* maxPriceEdit);
    void applyOrdersFilters(QTableWidget* table,
                           QLineEdit* searchEdit,
                           QComboBox* statusCombo,
                           QLineEdit* minCostEdit,
                           QLineEdit* maxCostEdit);

private:
    bool matchesSearchText(QTableWidget* table, int row, const QString& searchText) const;
    double extractCostFromItem(QTableWidgetItem* item) const;
};

#endif // FILTERMANAGER_H


