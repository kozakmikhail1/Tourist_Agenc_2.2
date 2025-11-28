#ifndef TRANSPORTCOMPANY_H
#define TRANSPORTCOMPANY_H

#include "models/touristservice.h"
#include <QString>
#include <QDate>
#include <QVector>

// Структура для графика перевозок
struct TransportSchedule {
    QString departureCity;
    QString arrivalCity;
    QDate departureDate;
    QDate arrivalDate;
    double price = 0.0;
    int availableSeats = 0;
    
    TransportSchedule() = default;
};

// Второй уровень наследования - Транспортная компания
class TransportCompany : public TouristService {
public:
    enum class TransportType {
        Airplane,
        Bus,
        Train,
        Ship
    };

    explicit TransportCompany(const QString& name = "", TransportType type = TransportType::Airplane);
    
    QString getType() const override { return "TransportCompany"; }
    QString getDescription() const override;
    
    TransportType getTransportType() const { return transportType_; }
    void setTransportType(TransportType type) { transportType_ = type; }
    
    void addSchedule(const TransportSchedule& schedule);
    void removeSchedule(int index);
    QVector<TransportSchedule> getSchedules() const { return schedules_; }
    TransportSchedule* getSchedule(int index);
    int getScheduleCount() const { return schedules_.size(); }
    
    static QString transportTypeToString(TransportType type);
    static TransportType stringToTransportType(const QString& str);

private:
    TransportType transportType_;
    QVector<TransportSchedule> schedules_;
};

#endif // TRANSPORTCOMPANY_H



