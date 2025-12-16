#include "models/tour.h"
#include <cmath>

Tour::Tour(const QString& name, const QString& country, 
           const QDate& startDate, const QDate& endDate)
    : TouristService(name, 0.0), country_(country), 
      startDate_(startDate), endDate_(endDate) {
}

QString Tour::getDescription() const {
    int duration = getDuration();
    return QString("Tour: %1, Country: %2, Duration: %3 days, Hotel: %4")
        .arg(getName(), country_, QString::number(duration), hotel_.getName());
}

double Tour::calculateCost() const {
    double totalCost = 0.0;
    
    totalCost += transportSchedule_.price;
    
    int nights = getDuration();
    if (nights > 0 && hotel_.getRoomCount() > 0) {
        auto rooms = hotel_.getRooms();
        if (!rooms.isEmpty()) {
            totalCost += rooms.first().getPricePerNight() * nights;
        }
    }
    
    return totalCost;
}

int Tour::getDuration() const {
    if (!startDate_.isValid() || !endDate_.isValid()) {
        return 0;
    }
    return startDate_.daysTo(endDate_);
}

