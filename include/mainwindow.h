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
// Forward declaration для включения в .cpp
// Полное определение Ui::MainWindow находится в ui_mainwindow.h
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    // Файловые операции
    void saveData();
    void loadData();

    // Вкладки
    void onTabChanged(int index);
    
    // Обработчики для трехрежимной сортировки
    void onCountriesHeaderClicked(int logicalIndex);
    void onHotelsHeaderClicked(int logicalIndex);
    void onTransportHeaderClicked(int logicalIndex);
    void onToursHeaderClicked(int logicalIndex);
    void onOrdersHeaderClicked(int logicalIndex);
    
    
    // Курсы валют
    void updateCurrencyRates();
    void onCurrencyDataReceived(QNetworkReply* reply);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    
    // Контейнеры данных
    DataContainer<Country> countries_;
    DataContainer<Hotel> hotels_;
    DataContainer<TransportCompany> transportCompanies_;
    DataContainer<Tour> tours_;
    DataContainer<Order> orders_;
    
    FileManager fileManager_;
    
    // Для работы с курсами валют
    QNetworkAccessManager* networkManager_;
    QTimer* currencyTimer_;
    
    void setupUI();
    void setupCurrencyUpdater();
    void setupMenuBar();
    void setupStatusBar();
    void setupTables();
    void setupControlsAdaptivity();
    void updateTablesFontSize();
    
    // Вспомогательные классы для уменьшения количества методов
    TableManager* tableManager_;
    FilterManager* filterManager_;
    FilterComboUpdater* filterComboUpdater_;
    
    // Классы-операторы для выполнения действий (Command pattern)
    QMap<QString, Action*> actions_;
    
    // Инициализация действий
    void initializeActions();
    
    int getSelectedCountryIndex() const;  // Получает реальный индекс страны в контейнере
    int getSelectedHotelIndex() const;  // Получает реальный индекс отеля в контейнере
    int getSelectedTransportIndex() const;  // Получает реальный индекс транспортной компании в контейнере
    int getSelectedTourIndex() const;  // Получает реальный индекс тура в контейнере
    int getSelectedOrderIndex() const;  // Получает реальный индекс заказа в контейнере
    
    // Вспомогательные функции для связывания данных
    void linkToursWithHotelsAndTransport();
    void linkOrdersToursWithHotelsAndTransport();
    
    // Вспомогательные функции для linkToursWithHotelsAndTransport
    QString findCountryCapital(const QString& countryName) const;
    QSet<QString> collectTargetCities(const QString& tourCountry, const QString& capital) const;
    Hotel* findHotelForTour(const QString& tourCountry);
    bool matchesCity(const QString& arrivalCity, const QSet<QString>& targetCities, 
                     const QString& capital, const QString& tourCountry) const;
    bool findTransportForTour(Tour& tour, const QSet<QString>& targetCities, 
                               const QString& capital, const QDate& tourStartDate);
    
    // Вспомогательные функции для loadData
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
    
    // Вспомогательная функция для создания кнопок действий
    QWidget* createActionButtons(int dataIndex, const QString& type);
    
    // Обновление таблиц
    void updateCountriesTable();
    void updateHotelsTable();
    void updateTransportCompaniesTable();
    void updateToursTable();
    void updateOrdersTable();
    
    // Обновление фильтров
    void updateCountriesFilterCombo();
    void updateHotelsFilterCombos();
    void updateTransportFilterCombo();
    void updateToursFilterCombo();
    void updateOrdersFilterCombo();
    
    // Применение фильтров
    void applyCountriesFilters();
    void applyHotelsFilters();
    void applyTransportFilters();
    void applyToursFilters();
    void applyOrdersFilters();
    
    // Операции со странами
    void addCountry();
    void editCountry();
    void deleteCountry();
    void showCountryInfo();
    void refreshCountries();
    
    // Операции с отелями
    void addHotel();
    void editHotel();
    void deleteHotel();
    void showHotelInfo();
    void refreshHotels();
    
    // Операции с транспортными компаниями
    void addTransportCompany();
    void editTransportCompany();
    void deleteTransportCompany();
    void showTransportCompanyInfo();
    void refreshTransportCompanies();
    
    // Операции с турами
    void addTour();
    void editTour();
    void deleteTour();
    void showTourInfo();
    void refreshTours();
    void searchTours();
    
    // Операции с заказами
    void addOrder();
    void editOrder();
    void deleteOrder();
    void showOrderInfo();
    void processOrder();
    void refreshOrders();
    
    // Вспомогательные методы
    int getSelectedRow(QTableWidget* table) const;
    
    // Вспомогательные функции для фильтрации и поиска
    bool matchesSearchInTable(QTableWidget* table, int row, const QString& searchText, int excludeColumn = -1) const;
    double extractCostFromTableItem(QTableWidget* table, int row, int column) const;
    
    // Переопределение для адаптивного размера шрифта
    void resizeEvent(QResizeEvent* event) override;
};

#endif // MAINWINDOW_H



