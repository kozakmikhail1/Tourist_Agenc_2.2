#include "models/room.h"
#include "models/touristservice.h"

Room::Room(const QString& name, RoomType type, double pricePerNight, int capacity)
    : TouristService(name, pricePerNight), roomType_(type), 
      capacity_(capacity), pricePerNight_(pricePerNight) {
}

QString Room::getDescription() const {
    return QString("Room: %1, Type: %2, Capacity: %3, Price per night: %4")
        .arg(getName(), roomTypeToString(roomType_), QString::number(capacity_), 
             QString::number(pricePerNight_, 'f', 2));
}

QString Room::roomTypeToString(RoomType type) {
    switch (type) {
        case RoomType::Single: return "Single";
        case RoomType::Double: return "Double";
        case RoomType::Suite: return "Suite";
        case RoomType::Apartment: return "Apartment";
        default: return "Unknown";
    }
}

Room::RoomType Room::stringToRoomType(const QString& str) {
    if (str == "Single") return RoomType::Single;
    if (str == "Double") return RoomType::Double;
    if (str == "Suite") return RoomType::Suite;
    if (str == "Apartment") return RoomType::Apartment;
    return RoomType::Single;
}

