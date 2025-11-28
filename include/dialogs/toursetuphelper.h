#ifndef TOURSETUPHELPER_H
#define TOURSETUPHELPER_H

#include <QString>
#include "models/tour.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include "containers/datacontainer.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

// Вспомогательный класс для настройки тура в диалоге
class TourSetupHelper {
public:
    TourSetupHelper(DataContainer<Country>* countries,
                   DataContainer<Hotel>* hotels,
                   DataContainer<TransportCompany>* companies,
                   DataContainer<Tour>* tours);
    
    bool findExistingTour(const Tour& tour, int& tourIndex) const;
    void setupTransportAndSchedule(const Tour& tour, 
                                   QComboBox* countryCombo,
                                   QComboBox* transportCombo,
                                   QComboBox* scheduleCombo) const;
    void setupHotelAndRoom(const Tour& tour,
                           QComboBox* countryCombo,
                           QComboBox* hotelCombo,
                           QComboBox* roomCombo) const;
    Hotel getSelectedHotel(const QString& country, 
                          QComboBox* countryCombo,
                          QComboBox* hotelCombo,
                          QComboBox* roomCombo) const;
    void setupTourTransport(Tour& tour,
                           QComboBox* countryCombo,
                           QComboBox* transportCombo,
                           QComboBox* scheduleCombo) const;

private:
    DataContainer<Country>* countries_;
    DataContainer<Hotel>* hotels_;
    DataContainer<TransportCompany>* companies_;
    DataContainer<Tour>* tours_;
    
    TransportCompany* findSelectedTransportCompany(QComboBox* countryCombo,
                                                   QComboBox* transportCombo) const;
    QString findCountryCapital(const QString& selectedCountry) const;
    QSet<QString> collectCitiesInCountry(const QString& selectedCountry) const;
    void findAndSetSchedule(QComboBox* scheduleCombo, TransportCompany* company,
                           const TransportSchedule& schedule, int scheduleIndex) const;
    int findScheduleComboIndex(QComboBox* scheduleCombo, int scheduleIndex) const;
};

#endif // TOURSETUPHELPER_H


