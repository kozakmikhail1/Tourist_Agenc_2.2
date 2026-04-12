#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QResizeEvent>
#include <QSet>
#include <QString>
#include <QWidget>
#include <memory>
#include "containers/datacontainer.h"
#include "models/country.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include "models/tour.h"
#include "models/order.h"
#include "utils/filemanager.h"
#include "mainwindow/tablemanager.h"
#include "mainwindow/filtermanager.h"
#include "mainwindow/filtercomboupdater.h"
#include "mainwindow/actions/action.h"
#include "mainwindow/actions/countryactions.h"
#include "mainwindow/actions/hotelactions.h"
#include "mainwindow/actions/transportactions.h"
#include "mainwindow/actions/touractions.h"
#include "mainwindow/actions/orderactions.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void saveData();
    void loadData();

    void onTabChanged(int index);
    
    void onCountriesHeaderClicked(int logicalIndex);
    void onHotelsHeaderClicked(int logicalIndex);
    void onTransportHeaderClicked(int logicalIndex);
    void onToursHeaderClicked(int logicalIndex);
    void onOrdersHeaderClicked(int logicalIndex);
    
    
    void updateCurrencyRates();
    void onCurrencyDataReceived(QNetworkReply* reply);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    
    DataContainer<Country> countries_;
    DataContainer<Hotel> hotels_;
    DataContainer<TransportCompany> transportCompanies_;
    DataContainer<Tour> tours_;
    DataContainer<Order> orders_;
    
    FileManager fileManager_;
    
    QNetworkAccessManager* networkManager_;
    QTimer* currencyTimer_;
    
    void setupUI();
    void setupCurrencyUpdater();
    void setupMenuBar();
    void setupStatusBar();
    void setupTables();
    void setupControlsAdaptivity();
    void updateTablesFontSize();
    
    TableManager* tableManager_;
    FilterManager* filterManager_;
    FilterComboUpdater* filterComboUpdater_;
    
    QMap<QString, Action*> actions_;
    
    void initializeActions();
    
    int getSelectedCountryIndex() const;
    int getSelectedHotelIndex() const;
    int getSelectedTransportIndex() const;
    int getSelectedTourIndex() const;
    int getSelectedOrderIndex() const;
    
    void linkToursWithHotelsAndTransport();
    void linkOrdersToursWithHotelsAndTransport();
    
    QString findCountryCapital(const QString& countryName) const;
    QSet<QString> collectTargetCities(const QString& tourCountry, const QString& capital) const;
    Hotel* findHotelForTour(const QString& tourCountry);
    bool matchesCity(const QString& arrivalCity, const QSet<QString>& targetCities, 
                     const QString& capital, const QString& tourCountry) const;
    bool findTransportForTour(Tour& tour, const QSet<QString>& targetCities, 
                               const QString& capital, const QDate& tourStartDate);
    
    QString findDataDirectory() const;
    bool validateDataDirectory(const QString& dataPath);
    QStringList checkRequiredFiles(const QString& dataPath);
    void clearAllData();
    struct LoadResult {
        int countries = 0;
        int hotels = 0;
        int companies = 0;
        int tours = 0;
        int orders = 0;
        QStringList errors;
    };
    LoadResult loadAllDataFiles(const QString& dataPath);
    void showLoadResults(const LoadResult& result, const QString& dataPath);
    
    QWidget* createActionButtons(int dataIndex, const QString& type);
    
    void updateCountriesTable();
    void updateHotelsTable();
    void updateTransportCompaniesTable();
    void updateToursTable();
    void updateOrdersTable();
    
    void updateCountriesFilterCombo();
    void updateHotelsFilterCombos();
    void updateTransportFilterCombo();
    void updateToursFilterCombo();
    void updateOrdersFilterCombo();
    
    void applyCountriesFilters();
    void applyHotelsFilters();
    void applyTransportFilters();
    void applyToursFilters();
    void applyOrdersFilters();
    
    void addCountry();
    void editCountry();
    void deleteCountry();
    void showCountryInfo();
    void refreshCountries();
    
    void addHotel();
    void editHotel();
    void deleteHotel();
    void showHotelInfo();
    void refreshHotels();
    
    void addTransportCompany();
    void editTransportCompany();
    void deleteTransportCompany();
    void showTransportCompanyInfo();
    void refreshTransportCompanies();
    
    void addTour();
    void editTour();
    void deleteTour();
    void showTourInfo();
    void refreshTours();
    void searchTours();
    
    void addOrder();
    void editOrder();
    void deleteOrder();
    void showOrderInfo();
    void processOrder();
    void refreshOrders();
    
    int getSelectedRow(QTableWidget* table) const;
    
    bool matchesSearchInTable(QTableWidget* table, int row, const QString& searchText, int excludeColumn = -1) const;
    double extractCostFromTableItem(QTableWidget* table, int row, int column) const;
    
    void resizeEvent(QResizeEvent* event) override;
};

#endif



