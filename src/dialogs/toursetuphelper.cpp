#include "dialogs/toursetuphelper.h"
#include <QComboBox>
#include <QLineEdit>
#include <QSet>
#include <QVariant>

TourSetupHelper::TourSetupHelper(DataContainer<Country>* countries,
                                DataContainer<Hotel>* hotels,
                                DataContainer<TransportCompany>* companies,
                                DataContainer<Tour>* tours)
    : countries_(countries)
    , hotels_(hotels)
    , companies_(companies)
    , tours_(tours)
{
}

bool TourSetupHelper::findExistingTour(const Tour& tour, int& tourIndex) const {
    if (!tours_) {
        return false;
    }
    
    for (int i = 0; i < tours_->size(); ++i) {
        Tour* existingTour = tours_->get(i);
        if (existingTour && 
            existingTour->getName() == tour.getName() &&
            existingTour->getCountry() == tour.getCountry() &&
            existingTour->getStartDate() == tour.getStartDate() &&
            existingTour->getEndDate() == tour.getEndDate()) {
            tourIndex = i;
            return true;
        }
    }
    return false;
}

QString TourSetupHelper::findCountryCapital(const QString& selectedCountry) const {
    if (!countries_) {
        return "";
    }
    
    for (const auto& country : countries_->getData()) {
        if (country.getName() == selectedCountry) {
            return country.getCapital();
        }
    }
    return "";
}

QSet<QString> TourSetupHelper::collectCitiesInCountry(const QString& selectedCountry) const {
    QSet<QString> cities;
    
    if (!hotels_) {
        return cities;
    }
    
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() != selectedCountry) {
            continue;
        }
        
        QString address = hotel.getAddress();
        if (address.isEmpty()) {
            continue;
        }
        
        QString city = address.split(',').first().trimmed();
        if (!city.isEmpty()) {
            cities.insert(city);
        }
        cities.insert(address);
    }
    
    return cities;
}

TransportCompany* TourSetupHelper::findSelectedTransportCompany(QComboBox* countryCombo,
                                                                 QComboBox* transportCombo) const {
    if (!companies_ || transportCombo->currentIndex() < 0) {
        return nullptr;
    }
    
    QString selectedCountry = countryCombo->currentText();
    QSet<QString> citiesInCountry = collectCitiesInCountry(selectedCountry);
    QString capital = findCountryCapital(selectedCountry);
    
    if (!capital.isEmpty()) {
        citiesInCountry.insert(capital);
    }
    
    int comboIndex = 0;
    for (int i = 0; i < companies_->size(); ++i) {
        TransportCompany* comp = companies_->get(i);
        if (!comp || comp->getScheduleCount() == 0) {
            continue;
        }
        
        bool hasRelevantSchedule = citiesInCountry.isEmpty();
        if (!hasRelevantSchedule) {
            for (const auto& schedule : comp->getSchedules()) {
                QString arrivalCity = schedule.arrivalCity;
                if (citiesInCountry.contains(arrivalCity) || 
                    (!capital.isEmpty() && arrivalCity.contains(capital, Qt::CaseInsensitive))) {
                    hasRelevantSchedule = true;
                    break;
                }
            }
        }
        
        if (!hasRelevantSchedule) {
            continue;
        }
        
        if (comboIndex == transportCombo->currentIndex()) {
            return comp;
        }
        comboIndex++;
    }
    return nullptr;
}

void TourSetupHelper::setupTransportAndSchedule(const Tour& tour, 
                                               QComboBox* countryCombo,
                                               QComboBox* transportCombo,
                                               QComboBox* scheduleCombo) const {
    TransportCompany transportCompany = tour.getTransportCompany();
    if (transportCompany.getName().isEmpty() || !companies_) {
        return;
    }
    
    TransportSchedule schedule = tour.getTransportSchedule();
    int comboIndex = 0;
    
    bool foundCompany = false;
    for (int i = 0; i < companies_->size() && !foundCompany; ++i) {
        TransportCompany* company = companies_->get(i);
        if (!company || company->getScheduleCount() == 0) {
            continue;
        }
        
        if (company->getName() != transportCompany.getName()) {
            comboIndex++;
            continue;
        }
        
        transportCombo->setCurrentIndex(comboIndex);
        foundCompany = true;
        
        if (!schedule.departureCity.isEmpty()) {
            findAndSetSchedule(scheduleCombo, company, schedule, -1);
        }
        return;
    }
}

void TourSetupHelper::findAndSetSchedule(QComboBox* scheduleCombo, TransportCompany* company,
                                         const TransportSchedule& schedule, int scheduleIndex) const {
    if (!scheduleCombo || !company || schedule.departureCity.isEmpty()) {
        return;
    }
    
    bool foundSchedule = false;
    int searchIndex = (scheduleIndex >= 0) ? scheduleIndex : 0;
    int maxIndex = (scheduleIndex >= 0) ? scheduleIndex + 1 : company->getScheduleCount();
    
    for (int j = searchIndex; j < maxIndex && !foundSchedule; ++j) {
        TransportSchedule* sched = company->getSchedule(j);
        if (!sched) {
            continue;
        }
        
        if (sched->departureCity == schedule.departureCity &&
            sched->arrivalCity == schedule.arrivalCity &&
            sched->departureDate == schedule.departureDate &&
            sched->arrivalDate == schedule.arrivalDate) {
            int comboIndex = findScheduleComboIndex(scheduleCombo, j);
            if (comboIndex >= 0) {
                scheduleCombo->setCurrentIndex(comboIndex);
                return;
            }
            foundSchedule = true;
        }
    }
}

void TourSetupHelper::setupHotelAndRoom(const Tour& tour,
                                        QComboBox* countryCombo,
                                        QComboBox* hotelCombo,
                                        QComboBox* roomCombo) const {
    Hotel hotel = tour.getHotel();
    if (hotel.getName().isEmpty() || !hotels_) {
        return;
    }
    
    QString selectedCountry = countryCombo->currentText();
    int comboHotelIndex = 0;
    
    for (const auto& h : hotels_->getData()) {
        if (h.getCountry() != selectedCountry) {
            continue;
        }
        
        if (h.getName() != hotel.getName()) {
            comboHotelIndex++;
            continue;
        }
        
        hotelCombo->setCurrentIndex(comboHotelIndex);
        
        if (hotel.getRoomCount() == 0) {
            break;
        }
        
        const Room* targetRoom = hotel.getRoom(0);
        if (!targetRoom) {
            break;
        }
        
        for (int i = 0; i < h.getRoomCount(); ++i) {
            const Room* room = h.getRoom(i);
            if (!room) {
                continue;
            }
            
            if (room->getName() == targetRoom->getName() &&
                room->getRoomType() == targetRoom->getRoomType() &&
                room->getPricePerNight() == targetRoom->getPricePerNight()) {
                for (int j = 0; j < roomCombo->count(); ++j) {
                    QVariant data = roomCombo->itemData(j);
                    if (data.isValid() && data.toInt() == i) {
                        roomCombo->setCurrentIndex(j);
                        return;
                    }
                }
                break;
            }
        }
        break;
    }
}

Hotel TourSetupHelper::getSelectedHotel(const QString& country, 
                                       QComboBox* countryCombo,
                                       QComboBox* hotelCombo,
                                       QComboBox* roomCombo) const {
    if (!hotels_ || hotelCombo->currentIndex() < 0) {
        return Hotel();
    }
    
    int hotelIndex = 0;
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() != country) {
            continue;
        }
        
        if (hotelIndex != hotelCombo->currentIndex()) {
            hotelIndex++;
            continue;
        }
        
        Hotel hotelCopy = hotel;
        while (hotelCopy.getRoomCount() > 0) {
            hotelCopy.removeRoom(0);
        }
        
        if (roomCombo->currentIndex() >= 0 && 
            roomCombo->currentIndex() < hotel.getRoomCount()) {
            const Room* selectedRoom = hotel.getRoom(roomCombo->currentIndex());
            if (selectedRoom) {
                hotelCopy.addRoom(*selectedRoom);
            }
        }
        return hotelCopy;
    }
    
    return Hotel();
}

void TourSetupHelper::setupTourTransport(Tour& tour,
                                         QComboBox* countryCombo,
                                         QComboBox* transportCombo,
                                         QComboBox* scheduleCombo) const {
    if (!companies_ || transportCombo->currentIndex() < 0) {
        return;
    }
    
    TransportCompany* company = findSelectedTransportCompany(countryCombo, transportCombo);
    if (!company) {
        return;
    }
    
    tour.setTransportCompany(*company);
    
    if (scheduleCombo->currentIndex() < 0 || 
        scheduleCombo->currentIndex() >= company->getScheduleCount()) {
        return;
    }
    
    TransportSchedule* schedule = company->getSchedule(scheduleCombo->currentIndex());
    if (schedule) {
        tour.setTransportSchedule(*schedule);
    }
}

int TourSetupHelper::findScheduleComboIndex(QComboBox* scheduleCombo, int scheduleIndex) const {
    if (!scheduleCombo) {
        return -1;
    }
    
    for (int k = 0; k < scheduleCombo->count(); ++k) {
        QVariant data = scheduleCombo->itemData(k);
        if (data.isValid() && data.toInt() == scheduleIndex) {
            return k;
        }
    }
    return -1;
}


