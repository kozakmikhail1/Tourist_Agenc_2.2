#include "models/transportcompany.h"

TransportCompany::TransportCompany(const QString& name, TransportType type)
    : TouristService(name, 0.0), transportType_(type) {
}

QString TransportCompany::getDescription() const {
    return QString("Transport Company: %1, Type: %2, Schedules: %3")
        .arg(getName(), transportTypeToString(transportType_), QString::number(schedules_.size()));
}

void TransportCompany::addSchedule(const TransportSchedule& schedule) {
    schedules_.append(schedule);
}

void TransportCompany::removeSchedule(int index) {
    if (index >= 0 && index < schedules_.size()) {
        schedules_.removeAt(index);
    }
}

TransportSchedule* TransportCompany::getSchedule(int index) {
    if (index >= 0 && index < schedules_.size()) {
        return &schedules_[index];
    }
    return nullptr;
}

QString TransportCompany::transportTypeToString(TransportType type) {
    switch (type) {
        case TransportType::Airplane: return "Самолет";
        case TransportType::Bus: return "Автобус";
        case TransportType::Train: return "Поезд";
        case TransportType::Ship: return "Корабль";
        default: return "Неизвестно";
    }
}

TransportCompany::TransportType TransportCompany::stringToTransportType(const QString& str) {
    // Поддержка русского языка
    if (str == "Самолет" || str == "Airplane") return TransportType::Airplane;
    if (str == "Автобус" || str == "Bus") return TransportType::Bus;
    if (str == "Поезд" || str == "Train") return TransportType::Train;
    if (str == "Корабль" || str == "Ship") return TransportType::Ship;
    return TransportType::Airplane;
}

