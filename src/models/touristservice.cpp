#include "models/touristservice.h"
#include <iostream>
#include <sstream>

TouristService::TouristService(const QString& name, double price)
    : name_(name), price_(price) {
}

void TouristService::writeToStream(std::ostream& os) const {
    os << name_.toStdString() << " " << price_;
}

void TouristService::readFromStream(std::istream& is) {
    std::string name;
    is >> name >> price_;
    name_ = QString::fromStdString(name);
}
