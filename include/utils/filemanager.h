#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "containers/datacontainer.h"
#include "models/country.h"
#include "models/hotel.h"
#include "models/transportcompany.h"
#include "models/tour.h"
#include "models/order.h"
#include <QString>
#include <QTextStream>
#include <exception>

// Исключение для ошибок работы с файлами
class FileException : public std::exception {
public:
    explicit FileException(const QString& message) : message_(message.toStdString()) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

// Менеджер для работы с файлами (Qt версия)
class FileManager {
public:
    FileManager();

    // Сохранение данных
    void saveCountries(const DataContainer<Country>& countries, const QString& filename) const;
    void saveHotels(const DataContainer<Hotel>& hotels, const QString& filename) const;
    void saveTransportCompanies(const DataContainer<TransportCompany>& companies, const QString& filename) const;
    void saveTours(const DataContainer<Tour>& tours, const QString& filename) const;
    void saveOrders(const DataContainer<Order>& orders, const QString& filename) const;

    // Загрузка данных
    void loadCountries(DataContainer<Country>& countries, const QString& filename) const;
    void loadHotels(DataContainer<Hotel>& hotels, const QString& filename) const;
    void loadTransportCompanies(DataContainer<TransportCompany>& companies, const QString& filename) const;
    void loadTours(DataContainer<Tour>& tours, const QString& filename) const;
    void loadOrders(DataContainer<Order>& orders, const QString& filename) const;

    // Сохранение всех данных
    void saveAll(const DataContainer<Country>& countries,
                 const DataContainer<Hotel>& hotels,
                 const DataContainer<TransportCompany>& companies,
                 const DataContainer<Tour>& tours,
                 const DataContainer<Order>& orders,
                 const QString& basePath = "data") const;

    // Загрузка всех данных
    void loadAll(DataContainer<Country>& countries,
                 DataContainer<Hotel>& hotels,
                 DataContainer<TransportCompany>& companies,
                 DataContainer<Tour>& tours,
                 DataContainer<Order>& orders,
                 const QString& basePath = "data") const;

private:
    QString dataPath_;
    
    // Общие функции для работы с файлами
    void openFileForWriting(QFile& file, const QString& filename) const;
    void openFileForReading(QFile& file, const QString& filename) const;
    void validateFileHeader(QTextStream& in, const QString& expectedHeader) const;
    
    // Вспомогательные функции для сохранения
    void saveHotelToStream(QTextStream& out, const Hotel& hotel) const;
    void saveRoomToStream(QTextStream& out, const Room& room) const;
    void saveTransportCompanyToStream(QTextStream& out, const TransportCompany& company) const;
    void saveScheduleToStream(QTextStream& out, const TransportSchedule& schedule) const;
    
    // Вспомогательные функции для загрузки
    Hotel loadHotelFromStream(QTextStream& in) const;
    Room loadRoomFromStream(QTextStream& in) const;
    TransportCompany loadTransportCompanyFromStream(QTextStream& in) const;
    TransportSchedule loadScheduleFromStream(QTextStream& in) const;
    
    Room readRoomFromStream(QTextStream& in, const QString& hotelName, int hotelIndex, int roomIndex) const;
    void skipInvalidHotelLines(QTextStream& in, int linesToSkip) const;
    QString determineOrderStatus(QTextStream& in, int orderIndex, int totalOrders) const;
};

#endif // FILEMANAGER_H



