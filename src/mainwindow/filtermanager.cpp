#include "mainwindow/filtermanager.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QComboBox>
#include "utils/numericsortitem.h"

FilterManager::FilterManager() = default;

bool FilterManager::matchesSearchText(QTableWidget* table, int row, const QString& searchText) const {
    if (searchText.isEmpty()) {
        return true;
    }
    
    QString searchLower = searchText.toLower();
    int columnCount = table->columnCount();
    
    for (int col = 0; col < columnCount; ++col) {
        QTableWidgetItem* item = table->item(row, col);
        if (item && item->text().toLower().contains(searchLower)) {
            return true;
        }
    }
    return false;
}

double FilterManager::extractCostFromItem(QTableWidgetItem* item) const {
    if (!item) {
        return 0.0;
    }
    
    QVariant sortData = item->data(Qt::UserRole);
    if (sortData.isValid() && sortData.canConvert<double>()) {
        return sortData.toDouble();
    }
    
    QString costText = item->text();
    costText.remove(" руб");
    costText = costText.trimmed();
    bool ok;
    double cost = costText.toDouble(&ok);
    return ok ? cost : 0.0;
}

void FilterManager::applyCountriesFilters(QTableWidget* table,
                                         QLineEdit* searchEdit,
                                         QComboBox* continentCombo,
                                         QComboBox* currencyCombo) {
    QString searchText = searchEdit->text().trimmed();
    QString continentFilter = continentCombo->currentText();
    QString currencyFilter = currencyCombo->currentText();
    
    for (int row = 0; row < table->rowCount(); ++row) {
        bool visible = true;
        
        if (!matchesSearchText(table, row, searchText)) {
            visible = false;
        }
        
        if (visible && !continentFilter.isEmpty() && continentFilter != "Все") {
            QTableWidgetItem* item = table->item(row, 1);
            if (!item || item->text() != continentFilter) {
                visible = false;
            }
        }
        
        if (visible && !currencyFilter.isEmpty() && currencyFilter != "Все") {
            QTableWidgetItem* item = table->item(row, 3);
            if (!item || item->text() != currencyFilter) {
                visible = false;
            }
        }
        
        table->setRowHidden(row, !visible);
    }
}

void FilterManager::applyHotelsFilters(QTableWidget* table,
                                      QLineEdit* searchEdit,
                                      QComboBox* countryCombo,
                                      QComboBox* starsCombo) {
    QString searchText = searchEdit->text().trimmed();
    QString countryFilter = countryCombo->currentText();
    QString starsFilter = starsCombo->currentText();
    
    for (int row = 0; row < table->rowCount(); ++row) {
        bool visible = true;
        
        if (!matchesSearchText(table, row, searchText)) {
            visible = false;
        }
        
        if (visible && !countryFilter.isEmpty() && countryFilter != "Все") {
            QTableWidgetItem* item = table->item(row, 1);
            if (!item || item->text() != countryFilter) {
                visible = false;
            }
        }
        
        if (visible && !starsFilter.isEmpty() && starsFilter != "Все") {
            QTableWidgetItem* item = table->item(row, 2);
            if (!item || item->text() != starsFilter) {
                visible = false;
            }
        }
        
        table->setRowHidden(row, !visible);
    }
}

void FilterManager::applyTransportFilters(QTableWidget* table,
                                          QLineEdit* searchEdit,
                                          QComboBox* typeCombo) {
    QString searchText = searchEdit->text().trimmed();
    QString typeFilter = typeCombo->currentText();
    
    for (int row = 0; row < table->rowCount(); ++row) {
        bool visible = true;
        
        if (!matchesSearchText(table, row, searchText)) {
            visible = false;
        }
        
        if (visible && !typeFilter.isEmpty() && typeFilter != "Все") {
            QTableWidgetItem* item = table->item(row, 1);
            if (!item || item->text() != typeFilter) {
                visible = false;
            }
        }
        
        table->setRowHidden(row, !visible);
    }
}

void FilterManager::applyToursFilters(QTableWidget* table,
                                      QLineEdit* searchEdit,
                                      QComboBox* countryCombo,
                                      QLineEdit* minPriceEdit,
                                      QLineEdit* maxPriceEdit) {
    QString searchText = searchEdit->text().trimmed();
    QString countryFilter = countryCombo->currentText();
    bool hasMinPrice = !minPriceEdit->text().trimmed().isEmpty();
    bool hasMaxPrice = !maxPriceEdit->text().trimmed().isEmpty();
    double minPrice = hasMinPrice ? minPriceEdit->text().toDouble() : 0.0;
    double maxPrice = hasMaxPrice ? maxPriceEdit->text().toDouble() : std::numeric_limits<double>::max();
    
    for (int row = 0; row < table->rowCount(); ++row) {
        bool visible = true;
        
        if (!matchesSearchText(table, row, searchText)) {
            visible = false;
        }
        
        if (visible && !countryFilter.isEmpty() && countryFilter != "Все") {
            QTableWidgetItem* item = table->item(row, 1);
            if (!item || item->text() != countryFilter) {
                visible = false;
            }
        }
        
        if (visible && (hasMinPrice || hasMaxPrice)) {
            QTableWidgetItem* costItem = table->item(row, 4);
            double cost = extractCostFromItem(costItem);
            if (cost < minPrice || cost > maxPrice) {
                visible = false;
            }
        }
        
        table->setRowHidden(row, !visible);
    }
}

void FilterManager::applyOrdersFilters(QTableWidget* table,
                                       QLineEdit* searchEdit,
                                       QComboBox* statusCombo,
                                       QLineEdit* minCostEdit,
                                       QLineEdit* maxCostEdit) {
    QString searchText = searchEdit->text().trimmed();
    QString statusFilter = statusCombo->currentText();
    bool hasMinCost = !minCostEdit->text().trimmed().isEmpty();
    bool hasMaxCost = !maxCostEdit->text().trimmed().isEmpty();
    double minCost = hasMinCost ? minCostEdit->text().toDouble() : 0.0;
    double maxCost = hasMaxCost ? maxCostEdit->text().toDouble() : std::numeric_limits<double>::max();
    
    for (int row = 0; row < table->rowCount(); ++row) {
        bool visible = true;
        
        if (!matchesSearchText(table, row, searchText)) {
            visible = false;
        }
        
        if (visible && !statusFilter.isEmpty() && statusFilter != "Все") {
            QTableWidgetItem* item = table->item(row, 5);
            if (!item || item->text() != statusFilter) {
                visible = false;
            }
        }
        
        if (visible && (hasMinCost || hasMaxCost)) {
            QTableWidgetItem* costItem = table->item(row, 4);
            double cost = extractCostFromItem(costItem);
            if (cost < minCost || cost > maxCost) {
                visible = false;
            }
        }
        
        table->setRowHidden(row, !visible);
    }
}


