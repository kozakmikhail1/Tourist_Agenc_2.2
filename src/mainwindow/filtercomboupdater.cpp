#include "mainwindow/filtercomboupdater.h"
#include <QComboBox>
#include <QSet>

FilterComboUpdater::FilterComboUpdater() = default;

void FilterComboUpdater::updateCountriesFilterCombo(QComboBox* continentCombo,
                                                     QComboBox* currencyCombo,
                                                     const DataContainer<Country>& countries) {
    QSet<QString> continents;
    QSet<QString> currencies;
    
    for (const auto& country : countries.getData()) {
        if (!country.getContinent().isEmpty()) {
            continents.insert(country.getContinent());
        }
        if (!country.getCurrency().isEmpty()) {
            currencies.insert(country.getCurrency());
        }
    }
    
    QString currentContinent = continentCombo->currentText();
    QString currentCurrency = currencyCombo->currentText();
    
    continentCombo->clear();
    continentCombo->addItem("Все");
    for (const QString& continent : continents) {
        continentCombo->addItem(continent);
    }
    
    currencyCombo->clear();
    currencyCombo->addItem("Все");
    for (const QString& currency : currencies) {
        currencyCombo->addItem(currency);
    }
    
    int continentIndex = continentCombo->findText(currentContinent);
    if (continentIndex >= 0) {
        continentCombo->setCurrentIndex(continentIndex);
    }
    
    int currencyIndex = currencyCombo->findText(currentCurrency);
    if (currencyIndex >= 0) {
        currencyCombo->setCurrentIndex(currencyIndex);
    }
}

void FilterComboUpdater::updateHotelsFilterCombos(QComboBox* countryCombo,
                                                   QComboBox* starsCombo,
                                                   const DataContainer<Hotel>& hotels) {
    QSet<QString> countries;
    QSet<QString> stars;
    
    for (const auto& hotel : hotels.getData()) {
        if (!hotel.getCountry().isEmpty()) {
            countries.insert(hotel.getCountry());
        }
        stars.insert(QString::number(hotel.getStars()));
    }
    
    QString currentCountry = countryCombo->currentText();
    QString currentStars = starsCombo->currentText();
    
    countryCombo->clear();
    countryCombo->addItem("Все");
    for (const QString& country : countries) {
        countryCombo->addItem(country);
    }
    
    starsCombo->clear();
    starsCombo->addItem("Все");
    for (const QString& star : stars) {
        starsCombo->addItem(star);
    }
    
    int countryIndex = countryCombo->findText(currentCountry);
    if (countryIndex >= 0) {
        countryCombo->setCurrentIndex(countryIndex);
    }
    
    int starsIndex = starsCombo->findText(currentStars);
    if (starsIndex >= 0) {
        starsCombo->setCurrentIndex(starsIndex);
    }
}

void FilterComboUpdater::updateTransportFilterCombo(QComboBox* typeCombo,
                                                      const DataContainer<TransportCompany>& companies) {
    QSet<QString> types;
    
    for (const auto& company : companies.getData()) {
        types.insert(TransportCompany::transportTypeToString(company.getTransportType()));
    }
    
    QString currentType = typeCombo->currentText();
    
    typeCombo->clear();
    typeCombo->addItem("Все");
    for (const QString& type : types) {
        typeCombo->addItem(type);
    }
    
    int typeIndex = typeCombo->findText(currentType);
    if (typeIndex >= 0) {
        typeCombo->setCurrentIndex(typeIndex);
    }
}

void FilterComboUpdater::updateToursFilterCombo(QComboBox* countryCombo,
                                                 const DataContainer<Tour>& tours) {
    QSet<QString> countries;
    
    for (const auto& tour : tours.getData()) {
        if (!tour.getCountry().isEmpty()) {
            countries.insert(tour.getCountry());
        }
    }
    
    QString currentCountry = countryCombo->currentText();
    
    countryCombo->clear();
    countryCombo->addItem("Все");
    for (const QString& country : countries) {
        countryCombo->addItem(country);
    }
    
    int countryIndex = countryCombo->findText(currentCountry);
    if (countryIndex >= 0) {
        countryCombo->setCurrentIndex(countryIndex);
    }
}

void FilterComboUpdater::updateOrdersFilterCombo(QComboBox* statusCombo) {
    QString currentStatus = statusCombo->currentText();
    
    statusCombo->clear();
    statusCombo->addItem("Все");
    statusCombo->addItem("В обработке");
    statusCombo->addItem("Подтвержден");
    statusCombo->addItem("Отменен");
    
    int statusIndex = statusCombo->findText(currentStatus);
    if (statusIndex >= 0) {
        statusCombo->setCurrentIndex(statusIndex);
    }
}


