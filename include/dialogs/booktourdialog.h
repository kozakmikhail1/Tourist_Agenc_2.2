#ifndef BOOKTOURDIALOG_H
#define BOOKTOURDIALOG_H

#include <QDialog>
#include <QSet>
#include <QString>
#include <memory>
#include "models/order.h"
#include "models/tour.h"
#include "models/country.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include "containers/datacontainer.h"
#include "dialogs/booktourcostcalculator.h"
#include "dialogs/toursetuphelper.h"

QT_BEGIN_NAMESPACE
namespace Ui { class BookTourDialog; }
QT_END_NAMESPACE

// Forward declaration для включения в .cpp
// Полное определение Ui::BookTourDialog находится в ui_booktourdialog.h

struct TransportSchedule;

class BookTourDialog : public QDialog {
    Q_OBJECT

public:
    explicit BookTourDialog(QWidget *parent = nullptr,
                           DataContainer<Country>* countries = nullptr,
                           DataContainer<Hotel>* hotels = nullptr,
                           DataContainer<TransportCompany>* companies = nullptr,
                           DataContainer<Tour>* tours = nullptr);
    ~BookTourDialog();
    
    Order getOrder() const;
    void setOrder(const Order& order);  // Установка данных заказа для редактирования
    void setEditMode(bool editMode = true);  // Установка режима редактирования

private slots:
    void accept() override;
    
private:
    bool validateClientData(const QString& name, const QString& phone, const QString& email) const;
    bool validateSelectMode() const;
    bool validateCreateMode() const;
    void onCountryChanged();
    void onTransportChanged();
    void onHotelChanged();
    void onDatesChanged();
    void calculateCost();
    void calculateCostForSelectMode();
    void calculateCostForCreateMode();
    

private slots:
    void onModeChanged();

private:
    std::unique_ptr<Ui::BookTourDialog> ui;
    DataContainer<Country>* countries_;
    DataContainer<Hotel>* hotels_;
    DataContainer<TransportCompany>* companies_;
    DataContainer<Tour>* tours_;
    bool isEditMode_;  // Флаг режима редактирования
    
    void updateTransportCombo();
    void updateHotelsCombo();
    void updateRoomsCombo();
    void updateToursCombo();
    void updateUIForMode();
    
    // Вспомогательные методы
    QString findCountryCapital(const QString& selectedCountry) const;
    QSet<QString> collectCitiesInCountry(const QString& selectedCountry) const;
    TransportCompany* findSelectedTransportCompany() const;
    void populateScheduleCombo(TransportCompany* company, const QSet<QString>& citiesInCountry, const QString& capital) const;
    bool findExistingTour(const Tour& tour, int& tourIndex);
    void setupSelectMode(int tourIndex, const QString& clientName, const QString& clientPhone, const QString& clientEmail);
    void setupCreateMode(const Tour& tour, const QString& clientName, const QString& clientPhone, const QString& clientEmail);
    void setupTransportAndSchedule(const Tour& tour);
    void setupHotelAndRoom(const Tour& tour);
    Tour getTourFromSelectMode() const;
    Hotel getSelectedHotel(const QString& country) const;
    void setupTourTransport(Tour& tour) const;
    Tour getTourFromCreateMode() const;
    
    // Вспомогательные классы для уменьшения количества методов
    std::unique_ptr<BookTourCostCalculator> costCalculator_;
    std::unique_ptr<TourSetupHelper> tourSetupHelper_;
};

#endif // BOOKTOURDIALOG_H

