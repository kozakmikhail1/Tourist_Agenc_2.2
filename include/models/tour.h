#ifndef TOUR_H
#define TOUR_H

#include "models/touristservice.h"
#include "models/country.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include <QString>
#include <QDate>
#include <memory>

class Tour : public TouristService {
public:
    explicit Tour(const QString& name = "", const QString& country = "", 
         const QDate& startDate = QDate(), const QDate& endDate = QDate());
    
    QString getType() const override { return "Tour"; }
    QString getDescription() const override;
    double calculateCost() const override;
    
    QString getCountry() const { return country_; }
    void setCountry(const QString& country) { country_ = country; }
    
    QDate getStartDate() const { return startDate_; }
    void setStartDate(const QDate& date) { startDate_ = date; }
    
    QDate getEndDate() const { return endDate_; }
    void setEndDate(const QDate& date) { endDate_ = date; }
    
    int getDuration() const;
    
    void setHotel(const Hotel& hotel) { hotel_ = hotel; }
    Hotel getHotel() const { return hotel_; }
    Hotel* getHotelPtr() { return &hotel_; }
    
    void setTransportCompany(const TransportCompany& company) { transportCompany_ = company; }
    TransportCompany getTransportCompany() const { return transportCompany_; }
    TransportCompany* getTransportCompanyPtr() { return &transportCompany_; }
    
    void setTransportSchedule(const TransportSchedule& schedule) { transportSchedule_ = schedule; }
    TransportSchedule getTransportSchedule() const { return transportSchedule_; }

private:
    QString country_;
    QDate startDate_;
    QDate endDate_;
    Hotel hotel_;
    TransportCompany transportCompany_;
    TransportSchedule transportSchedule_;
};

#endif



