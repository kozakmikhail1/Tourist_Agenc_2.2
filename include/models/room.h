#ifndef ROOM_H
#define ROOM_H

#include "models/touristservice.h"
#include <QString>

class Room : public TouristService {
public:
    enum class RoomType {
        Single,
        Double,
        Suite,
        Apartment
    };

    explicit Room(const QString& name = "", RoomType type = RoomType::Single, 
         double pricePerNight = 0.0, int capacity = 1);
    
    QString getType() const override { return "Room"; }
    QString getDescription() const override;
    
    RoomType getRoomType() const { return roomType_; }
    void setRoomType(RoomType type) { roomType_ = type; }
    
    int getCapacity() const { return capacity_; }
    void setCapacity(int capacity) { capacity_ = capacity; }
    
    double getPricePerNight() const { return pricePerNight_; }
    void setPricePerNight(double price) { pricePerNight_ = price; }
    
    static QString roomTypeToString(RoomType type);
    static RoomType stringToRoomType(const QString& str);

private:
    RoomType roomType_;
    int capacity_;
    double pricePerNight_;
};

#endif



