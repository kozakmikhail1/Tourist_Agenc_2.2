#include "models/hotel.h"
#include <algorithm>

Hotel::Hotel(const QString& name, const QString& country, int stars, const QString& address)
    : TouristService(name, 0.0), country_(country), stars_(stars), address_(address) {
}

QString Hotel::getDescription() const {
    return QString("Hotel: %1, Country: %2, Stars: %3, Address: %4, Rooms: %5")
        .arg(getName(), country_, QString::number(stars_), address_, QString::number(rooms_.size()));
}

double Hotel::calculateCost() const {
    double total = 0.0;
    for (const auto& room : rooms_) {
        total += room.getPricePerNight();
    }
    return total;
}

void Hotel::addRoom(const Room& room) {
    rooms_.append(room);
}

void Hotel::removeRoom(int index) {
    if (index >= 0 && index < rooms_.size()) {
        rooms_.removeAt(index);
    }
}

Room* Hotel::getRoom(int index) {
    if (index >= 0 && index < rooms_.size()) {
        return &rooms_[index];
    }
    return nullptr;
}

const Room* Hotel::getRoom(int index) const {
    if (index >= 0 && index < rooms_.size()) {
        return &rooms_[index];
    }
    return nullptr;
}

