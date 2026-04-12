#ifndef BOOKTOURCOSTCALCULATOR_H
#define BOOKTOURCOSTCALCULATOR_H

#include <QString>
#include <QComboBox>
#include <QDateEdit>
#include "containers/datacontainer.h"
#include "models/hotel.h"
#include "models/transportcompany.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QDateEdit;
QT_END_NAMESPACE

struct BookTourUIElements {
    QComboBox* countryCombo;
    QComboBox* transportCombo;
    QComboBox* scheduleCombo;
    QComboBox* hotelCombo;
    QComboBox* roomCombo;
    QDateEdit* startDateEdit;
    QDateEdit* endDateEdit;
};

class BookTourCostCalculator {
public:
    BookTourCostCalculator(DataContainer<Hotel>* hotels,
                          DataContainer<TransportCompany>* companies,
                          const BookTourUIElements& uiElements);
    
    double calculateTotalCost() const;
    double calculateTransportCost() const;
    double calculateHotelCost() const;
    int calculateNights() const;
    double getStarMultiplier(int stars) const;
    double getCountryMultiplier(const QString& country) const;

private:
    DataContainer<Hotel>* hotels_;
    DataContainer<TransportCompany>* companies_;
    BookTourUIElements uiElements_;
};

#endif


