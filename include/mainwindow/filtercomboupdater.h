#ifndef FILTERCOMBOUPDATER_H
#define FILTERCOMBOUPDATER_H

#include <QComboBox>
#include "containers/datacontainer.h"
#include "models/country.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include "models/tour.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

// Вспомогательный класс для обновления комбо-боксов фильтров
class FilterComboUpdater {
public:
    FilterComboUpdater();
    
    void updateCountriesFilterCombo(QComboBox* continentCombo,
                                   QComboBox* currencyCombo,
                                   const DataContainer<Country>& countries);
    void updateHotelsFilterCombos(QComboBox* countryCombo,
                                  QComboBox* starsCombo,
                                  const DataContainer<Hotel>& hotels);
    void updateTransportFilterCombo(QComboBox* typeCombo,
                                   const DataContainer<TransportCompany>& companies);
    void updateToursFilterCombo(QComboBox* countryCombo,
                               const DataContainer<Tour>& tours);
    void updateOrdersFilterCombo(QComboBox* statusCombo);
};

#endif // FILTERCOMBOUPDATER_H


