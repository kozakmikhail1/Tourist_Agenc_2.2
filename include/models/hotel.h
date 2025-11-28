#ifndef HOTEL_H
#define HOTEL_H

#include "models/touristservice.h"
#include "models/room.h"
#include <QString>
#include <QVector>
#include <memory>

// Второй уровень наследования - Отель
class Hotel : public TouristService {
public:
    explicit Hotel(const QString& name = "", const QString& country = "", 
          int stars = 3, const QString& address = "");
    
    QString getType() const override { return "Hotel"; }
    QString getDescription() const override;
    double calculateCost() const override;
    
    QString getCountry() const { return country_; }
    void setCountry(const QString& country) { country_ = country; }
    
    int getStars() const { return stars_; }
    void setStars(int stars) { stars_ = stars; }
    
    QString getAddress() const { return address_; }
    void setAddress(const QString& address) { address_ = address; }
    
    void addRoom(const Room& room);
    void removeRoom(int index);
    QVector<Room> getRooms() const { return rooms_; }
    Room* getRoom(int index);
    const Room* getRoom(int index) const;
    int getRoomCount() const { return rooms_.size(); }

private:
    QString country_;
    int stars_;
    QString address_;
    QVector<Room> rooms_;
};

#endif // HOTEL_H



