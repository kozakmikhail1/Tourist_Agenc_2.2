#include "dialogs/booktourcostcalculator.h"
#include <QComboBox>
#include <QDateEdit>

BookTourCostCalculator::BookTourCostCalculator(DataContainer<Hotel>* hotels,
                                              DataContainer<TransportCompany>* companies,
                                              const BookTourUIElements& uiElements)
    : hotels_(hotels)
    , companies_(companies)
    , uiElements_(uiElements)
{
}

int BookTourCostCalculator::calculateNights() const {
    if (!uiElements_.startDateEdit->date().isValid() || !uiElements_.endDateEdit->date().isValid()) {
        return 0;
    }
    return uiElements_.startDateEdit->date().daysTo(uiElements_.endDateEdit->date());
}

double BookTourCostCalculator::calculateTransportCost() const {
    if (!companies_ || uiElements_.transportCombo->currentIndex() < 0 || 
        uiElements_.scheduleCombo->currentIndex() < 0) {
        return 0.0;
    }
    
    TransportCompany* company = companies_->get(uiElements_.transportCombo->currentIndex());
    if (!company || uiElements_.scheduleCombo->currentIndex() >= company->getScheduleCount()) {
        return 0.0;
    }
    
    TransportSchedule* schedule = company->getSchedule(uiElements_.scheduleCombo->currentIndex());
    return schedule ? schedule->price : 0.0;
}

double BookTourCostCalculator::calculateHotelCost() const {
    int nights = calculateNights();
    if (!hotels_ || uiElements_.hotelCombo->currentIndex() < 0 || 
        uiElements_.roomCombo->currentIndex() < 0 || nights <= 0) {
        return 0.0;
    }
    
    QString selectedCountry = uiElements_.countryCombo->currentText();
    int hotelIndex = 0;
    
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() != selectedCountry) {
            continue;
        }
        
        if (hotelIndex != uiElements_.hotelCombo->currentIndex()) {
            hotelIndex++;
            continue;
        }
        
        double starMultiplier = getStarMultiplier(hotel.getStars());
        
        if (uiElements_.roomCombo->currentIndex() >= hotel.getRoomCount()) {
            break;
        }
        
        const Room* room = hotel.getRoom(uiElements_.roomCombo->currentIndex());
        if (!room) {
            break;
        }
        
        double basePrice = room->getPricePerNight();
        return basePrice * starMultiplier * nights;
    }
    
    return 0.0;
}

double BookTourCostCalculator::getStarMultiplier(int stars) const {
    switch (stars) {
        case 5: return 1.5;
        case 4: return 1.2;
        case 3: return 1.0;
        case 2: return 0.8;
        case 1: return 0.6;
        default: return 1.0;
    }
}

double BookTourCostCalculator::getCountryMultiplier(const QString& country) const {
    if (country.contains("Франция") || country.contains("Италия") || 
        country.contains("Испания")) {
        return 1.1;
    }
    if (country.contains("Египет") || country.contains("Турция")) {
        return 0.9;
    }
    return 1.0;
}

double BookTourCostCalculator::calculateTotalCost() const {
    double transportCost = calculateTransportCost();
    double hotelCost = calculateHotelCost();
    double baseTotal = transportCost + hotelCost;
    
    QString country = uiElements_.countryCombo->currentText();
    double countryMultiplier = getCountryMultiplier(country);
    
    return baseTotal * countryMultiplier;
}


