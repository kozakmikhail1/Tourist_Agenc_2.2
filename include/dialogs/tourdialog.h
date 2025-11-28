#ifndef TOURDIALOG_H
#define TOURDIALOG_H

#include <QDialog>
#include <memory>
#include "models/tour.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include "containers/datacontainer.h"
#include "models/country.h"
#include "dialogs/toursetuphelper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TourDialog; }
QT_END_NAMESPACE

// Forward declaration для включения в .cpp
// Полное определение Ui::TourDialog находится в ui_tourdialog.h

class TourDialog : public QDialog {
    Q_OBJECT

public:
    explicit TourDialog(QWidget *parent = nullptr,
                       DataContainer<Country>* countries = nullptr,
                       DataContainer<Hotel>* hotels = nullptr,
                       DataContainer<TransportCompany>* companies = nullptr,
                       Tour* tour = nullptr);
    ~TourDialog();
    
    Tour getTour() const;

private slots:
    void accept() override;
    void onCountryChanged();
    void onHotelChanged();
    void onTransportChanged();
    void calculateCost();

private:
    std::unique_ptr<Ui::TourDialog> ui;
    DataContainer<Country>* countries_;
    DataContainer<Hotel>* hotels_;
    DataContainer<TransportCompany>* companies_;
    Tour* tour_;
    std::unique_ptr<TourSetupHelper> tourSetupHelper_;
    
    void updateHotelsCombo();
    void updateRoomsCombo();
    void updateTransportCombo();
    void updateSchedulesCombo();
    
    // Вспомогательные функции для calculateCost
    double calculateTransportCost() const;
    double calculateHotelCost() const;
    TransportCompany* findSelectedTransportCompany() const;
    
    // Вспомогательные функции для getTour
    Hotel getSelectedHotel(const QString& country) const;
    void setupTourTransport(Tour& tour, const QString& country) const;
    
    // Вспомогательные функции для updateTransportCombo и updateSchedulesCombo
    bool hasRelevantScheduleForCountry(const TransportCompany& company, 
                                       const QSet<QString>& citiesInCountry, 
                                       const QString& capital) const;
    void populateScheduleCombo(TransportCompany* company, 
                               const QSet<QString>& citiesInCountry, 
                               const QString& capital) const;
};

#endif // TOURDIALOG_H



