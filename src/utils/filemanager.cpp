#include "utils/filemanager.h"
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QLocale>

FileManager::FileManager() {
    dataPath_ = "data";
    QDir dir;
    if (!dir.exists(dataPath_)) {
        dir.mkpath(dataPath_);
    }
}

void FileManager::openFileForWriting(QFile& file, const QString& filename) const {
    file.setFileName(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw FileException(QString("Cannot open file for writing: %1").arg(filename));
    }
}

void FileManager::openFileForReading(QFile& file, const QString& filename) const {
    file.setFileName(filename);
    if (!file.exists()) {
        qWarning() << "File does not exist:" << filename;
        throw FileException(QString("File does not exist: %1").arg(filename));
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw FileException(QString("Cannot open file for reading: %1").arg(filename));
    }
}

void FileManager::validateFileHeader(QTextStream& in, const QString& expectedHeader) const {
    QString header = in.readLine();
    if (header != expectedHeader) {
        throw FileException("Invalid file format");
    }
}

void FileManager::saveCountries(const DataContainer<Country>& countries, const QString& filename) const {
    QFile file;
    openFileForWriting(file, filename);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);

    out << "COUNTRIES\n";
    out << countries.size() << "\n";

    for (const auto& country : countries.getData()) {
        out << country.getName() << "\n";
        out << country.getContinent() << "\n";
        out << country.getCapital() << "\n";
        out << country.getCurrency() << "\n";
    }
}

void FileManager::loadCountries(DataContainer<Country>& countries, const QString& filename) const {
    QFile file;
    openFileForReading(file, filename);
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Encoding::Utf8);
    validateFileHeader(in, "COUNTRIES");

    int count = in.readLine().toInt();
    if (count <= 0) {
        qWarning() << "Invalid count in countries file:" << count;
        return;
    }
    
    countries.clear();

    for (int i = 0; i < count; ++i) {
        Country country;
        QString name = in.readLine();
        QString continent = in.readLine();
        QString capital = in.readLine();
        QString currency = in.readLine();
        
        if (name.isEmpty() || continent.isEmpty() || capital.isEmpty() || currency.isEmpty()) {
            throw FileException(QString("Incomplete data for country at index %1").arg(i));
        }
        
        country.setName(name);
        country.setContinent(continent);
        country.setCapital(capital);
        country.setCurrency(currency);
        countries.add(country);
    }
    
}

void FileManager::saveHotels(const DataContainer<Hotel>& hotels, const QString& filename) const {
    QFile file;
    openFileForWriting(file, filename);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);

    out << "HOTELS\n";
    out << hotels.size() << "\n";

    for (const auto& hotel : hotels.getData()) {
        saveHotelToStream(out, hotel);
    }
}

void FileManager::loadHotels(DataContainer<Hotel>& hotels, const QString& filename) const {
    QFile file;
    openFileForReading(file, filename);
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Encoding::Utf8);
    validateFileHeader(in, "HOTELS");

    int count = in.readLine().toInt();
    hotels.clear();

    for (int i = 0; i < count; ++i) {
        Hotel hotel;
        QString hotelName = in.readLine().trimmed();
        if (hotelName.isEmpty()) {
            qWarning() << "Skipping hotel at index" << i << "- empty name";
            skipInvalidHotelLines(in, 4);
            continue;
        }
        
        QString hotelCountry = in.readLine().trimmed();
        if (hotelCountry.isEmpty()) {
            qWarning() << "Skipping hotel" << hotelName << "at index" << i << "- empty country";
            skipInvalidHotelLines(in, 3);
            continue;
        }
        
        bool ok;
        QString starsStr = in.readLine().trimmed();
        int stars = starsStr.toInt(&ok);
        if (!ok || stars < 1 || stars > 7) {
            throw FileException(QString("Invalid stars for hotel at index %1: '%2' (expected number 1-7, got '%3'). This usually means the previous hotel has incorrect room count.")
                .arg(i).arg(hotelName).arg(starsStr));
        }
        
        QString hotelAddress = in.readLine().trimmed();
        
        hotel.setName(hotelName);
        hotel.setCountry(hotelCountry);
        hotel.setStars(stars);
        hotel.setAddress(hotelAddress);
        
        QString roomCountStr = in.readLine().trimmed();
        int roomCount = roomCountStr.toInt(&ok);
        if (!ok || roomCount < 0) {
            throw FileException(QString("Invalid room count for hotel '%1' (index %2): expected number, got '%3'")
                .arg(hotel.getName()).arg(i).arg(roomCountStr));
        }
        
        for (int j = 0; j < roomCount; ++j) {
            if (in.atEnd()) {
                throw FileException(QString("Unexpected end of file while reading room %1 for hotel '%2' (index %3)")
                    .arg(j).arg(hotel.getName()).arg(i));
            }
            
            Room room = readRoomFromStream(in, hotel.getName(), i, j);
            hotel.addRoom(room);
        }
        
        hotels.add(hotel);
    }
    
}

void FileManager::saveTransportCompanies(const DataContainer<TransportCompany>& companies, const QString& filename) const {
    QFile file;
    openFileForWriting(file, filename);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);

    out << "TRANSPORT_COMPANIES\n";
    out << companies.size() << "\n";

    for (const auto& company : companies.getData()) {
        saveTransportCompanyToStream(out, company);
    }
}

void FileManager::loadTransportCompanies(DataContainer<TransportCompany>& companies, const QString& filename) const {
    QFile file;
    openFileForReading(file, filename);
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Encoding::Utf8);
    validateFileHeader(in, "TRANSPORT_COMPANIES");

    int count = in.readLine().toInt();
    companies.clear();

    for (int i = 0; i < count; ++i) {
        TransportCompany company = loadTransportCompanyFromStream(in);
        companies.add(company);
    }
    
}

void FileManager::saveTours(const DataContainer<Tour>& tours, const QString& filename) const {
    QFile file;
    openFileForWriting(file, filename);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);

    out << "TOURS\n";
    out << tours.size() << "\n";

    for (const auto& tour : tours.getData()) {
        out << tour.getName() << "\n";
        out << tour.getCountry() << "\n";
        out << tour.getStartDate().toString(Qt::ISODate) << "\n";
        out << tour.getEndDate().toString(Qt::ISODate) << "\n";
        
        saveHotelToStream(out, tour.getHotel());
        
        saveTransportCompanyToStream(out, tour.getTransportCompany());
        
        saveScheduleToStream(out, tour.getTransportSchedule());
    }
}

void FileManager::loadTours(DataContainer<Tour>& tours, const QString& filename) const {
    QFile file;
    openFileForReading(file, filename);
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Encoding::Utf8);
    validateFileHeader(in, "TOURS");

    int count = in.readLine().toInt();
    tours.clear();

    for (int i = 0; i < count; ++i) {
        try {
            Tour tour;
            QString name = in.readLine().trimmed();
            QString country = in.readLine().trimmed();
            QString startDateStr = in.readLine().trimmed();
            QString endDateStr = in.readLine().trimmed();
            
            if (name.isEmpty() || country.isEmpty()) {
                qWarning() << "Skipping tour at index" << i << "- empty name or country";
                continue;
            }
        
        tour.setName(name);
        tour.setCountry(country);
        
        QDate startDate = QDate::fromString(startDateStr, Qt::ISODate);
        QDate endDate = QDate::fromString(endDateStr, Qt::ISODate);
        
        if (startDate.isValid()) {
            tour.setStartDate(startDate);
        }
        if (endDate.isValid()) {
            tour.setEndDate(endDate);
        }
        
        Hotel hotel = loadHotelFromStream(in);
        tour.setHotel(hotel);
        
        TransportCompany transport = loadTransportCompanyFromStream(in);
        tour.setTransportCompany(transport);
        
        TransportSchedule selectedSchedule = loadScheduleFromStream(in);
        tour.setTransportSchedule(selectedSchedule);
        
        tours.add(tour);
        } catch (const FileException& e) {
            qWarning() << "Error loading tour at index" << i << ":" << e.what();
        }
    }
    
}

void FileManager::saveOrders(const DataContainer<Order>& orders, const QString& filename) const {
    QFile file;
    openFileForWriting(file, filename);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);

    out << "ORDERS\n";
    out << orders.size() << "\n";

    for (const auto& order : orders.getData()) {
        out << order.getClientName() << "\n";
        out << order.getClientPhone() << "\n";
        out << order.getClientEmail() << "\n";
        out << "2" << "\n";
        out << order.getOrderDate().date().toString(Qt::ISODate) << "\n";
        
        const Tour& tour = order.getTour();
        out << tour.getName() << "\n";
        out << tour.getCountry() << "\n";
        out << tour.getStartDate().toString(Qt::ISODate) << "\n";
        out << tour.getEndDate().toString(Qt::ISODate) << "\n";
        
        saveHotelToStream(out, tour.getHotel());
        
        saveTransportCompanyToStream(out, tour.getTransportCompany());
        
        saveScheduleToStream(out, tour.getTransportSchedule());

        out << order.getStatus() << "\n";
    }
}

void FileManager::loadOrders(DataContainer<Order>& orders, const QString& filename) const {
    QFile file;
    openFileForReading(file, filename);
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Encoding::Utf8);
    validateFileHeader(in, "ORDERS");

    int count = in.readLine().toInt();
    orders.clear();

    for (int i = 0; i < count; ++i) {
        try {
            QString clientName = in.readLine().trimmed();
            QString clientPhone = in.readLine().trimmed();
            QString clientEmail = in.readLine().trimmed();
            int peopleCount = in.readLine().toInt();
            QDate orderDate = QDate::fromString(in.readLine().trimmed(), Qt::ISODate);
            
            Tour tour;
            QString tourName = in.readLine().trimmed();
            QString tourCountry = in.readLine().trimmed();
            QDate startDate = QDate::fromString(in.readLine().trimmed(), Qt::ISODate);
            QDate endDate = QDate::fromString(in.readLine().trimmed(), Qt::ISODate);
            
            tour.setName(tourName);
            tour.setCountry(tourCountry);
            tour.setStartDate(startDate);
            tour.setEndDate(endDate);
            
            Hotel hotel = loadHotelFromStream(in);
            tour.setHotel(hotel);
            
            TransportCompany transport = loadTransportCompanyFromStream(in);
            tour.setTransportCompany(transport);
            
            TransportSchedule selectedSchedule = loadScheduleFromStream(in);
            tour.setTransportSchedule(selectedSchedule);
            
            Order order;
            order.setTour(tour);
            order.setClientName(clientName);
            order.setClientPhone(clientPhone);
            order.setClientEmail(clientEmail);
            QDateTime orderDateTime(orderDate, QTime(0, 0));
            order.setOrderDate(orderDateTime);
            
            QString status = determineOrderStatus(in, i, count);
            order.setStatus(status);
            
            orders.add(order);
        } catch (const FileException& e) {
            qWarning() << "Error loading order at index" << i << ":" << e.what();
        }
    }
    
}

void FileManager::saveAll(const DataContainer<Country>& countries,
                          const DataContainer<Hotel>& hotels,
                          const DataContainer<TransportCompany>& companies,
                          const DataContainer<Tour>& tours,
                          const DataContainer<Order>& orders,
                          const QString& basePath) const {
    saveCountries(countries, basePath + "/countries.txt");
    saveHotels(hotels, basePath + "/hotels.txt");
    saveTransportCompanies(companies, basePath + "/transport_companies.txt");
    saveTours(tours, basePath + "/tours.txt");
    saveOrders(orders, basePath + "/orders.txt");
}

void FileManager::loadAll(DataContainer<Country>& countries,
                          DataContainer<Hotel>& hotels,
                          DataContainer<TransportCompany>& companies,
                          DataContainer<Tour>& tours,
                          DataContainer<Order>& orders,
                          const QString& basePath) const {
    loadCountries(countries, basePath + "/countries.txt");
    loadHotels(hotels, basePath + "/hotels.txt");
    loadTransportCompanies(companies, basePath + "/transport_companies.txt");
    loadTours(tours, basePath + "/tours.txt");
    loadOrders(orders, basePath + "/orders.txt");
}

Room FileManager::readRoomFromStream(QTextStream& in, const QString& hotelName, int hotelIndex, int roomIndex) const {
    Room room;
    QString roomName = in.readLine().trimmed();
    QString roomTypeStr = in.readLine().trimmed();
    QString priceStr = in.readLine().trimmed();
    QString capacityStr = in.readLine().trimmed();
    
    if (roomName.isEmpty() || roomTypeStr.isEmpty() || priceStr.isEmpty() || capacityStr.isEmpty()) {
        throw FileException(QString("Incomplete room data for hotel '%1' (index %7), room %2. RoomName: '%3', RoomType: '%4', Price: '%5', Capacity: '%6'")
            .arg(hotelName).arg(roomIndex).arg(roomName).arg(roomTypeStr).arg(priceStr).arg(capacityStr).arg(hotelIndex));
    }
    
    bool ok;
    int roomType = roomTypeStr.toInt(&ok);
    if (!ok || roomType < 0 || roomType > 3) {
        throw FileException(QString("Invalid room type for hotel '%1' (index %4), room %2: '%3'")
            .arg(hotelName).arg(roomIndex).arg(roomTypeStr).arg(hotelIndex));
    }
    
    QLocale locale(QLocale::C);
    double price = locale.toDouble(priceStr, &ok);
    if (!ok) {
        price = priceStr.toDouble(&ok);
    }
    if (!ok || price < 0) {
        throw FileException(QString("Invalid price for hotel '%1' (index %4), room %2: expected number, got '%3'")
            .arg(hotelName).arg(roomIndex).arg(priceStr).arg(hotelIndex));
    }
    
    int capacity = capacityStr.toInt(&ok);
    if (!ok || capacity <= 0) {
        throw FileException(QString("Invalid capacity for hotel '%1' (index %4), room %2: '%3'")
            .arg(hotelName).arg(roomIndex).arg(capacityStr).arg(hotelIndex));
    }
    
    room.setName(roomName);
    room.setRoomType(static_cast<Room::RoomType>(roomType));
    room.setPricePerNight(price);
    room.setCapacity(capacity);
    return room;
}

void FileManager::skipInvalidHotelLines(QTextStream& in, int linesToSkip) const {
    for (int skip = 0; skip < linesToSkip; ++skip) {
        if (!in.atEnd()) {
            in.readLine();
        }
    }
}

QString FileManager::determineOrderStatus(QTextStream& in, int orderIndex, int totalOrders) const {
    QString defaultStatus = "В обработке";
    
    if (in.atEnd()) {
        return defaultStatus;
    }
    
    qint64 pos = in.device()->pos();
    QString peekLine = in.readLine().trimmed();
    
    QStringList validStatuses = {"В обработке", "Подтвержден", "Оплачен", "Завершен", "Отменен"};
    bool isValidStatus = validStatuses.contains(peekLine);
    
    bool looksLikeClientName = peekLine.contains(" ") && 
                             !peekLine.contains("@") && 
                             !peekLine.startsWith("+") &&
                             !peekLine.contains("client@") &&
                             peekLine.length() > 5 && peekLine.length() < 100 &&
                             (!peekLine.contains("-") || (peekLine.contains("-") && peekLine.length() > 15));
    
    if (isValidStatus) {
        return peekLine;
    }
    
    if (looksLikeClientName && orderIndex < totalOrders - 1) {
        in.device()->seek(pos);
        return defaultStatus;
    }
    
    if (orderIndex == totalOrders - 1) {
        if (!looksLikeClientName && peekLine.length() > 0 && peekLine.length() < 50) {
            return peekLine;
        }
        in.device()->seek(pos);
        return defaultStatus;
    }
    
    in.device()->seek(pos);
    return defaultStatus;
}

void FileManager::saveHotelToStream(QTextStream& out, const Hotel& hotel) const {
    out << hotel.getName() << "\n";
    out << hotel.getCountry() << "\n";
    out << hotel.getStars() << "\n";
    out << hotel.getAddress() << "\n";
    out << hotel.getRoomCount() << "\n";
    
    for (const auto& room : hotel.getRooms()) {
        saveRoomToStream(out, room);
    }
}

void FileManager::saveRoomToStream(QTextStream& out, const Room& room) const {
    out << room.getName() << "\n";
    out << static_cast<int>(room.getRoomType()) << "\n";
    out << room.getPricePerNight() << "\n";
    out << room.getCapacity() << "\n";
}

void FileManager::saveTransportCompanyToStream(QTextStream& out, const TransportCompany& company) const {
    out << company.getName() << "\n";
    out << static_cast<int>(company.getTransportType()) << "\n";
    out << company.getScheduleCount() << "\n";
    
    for (const auto& schedule : company.getSchedules()) {
        saveScheduleToStream(out, schedule);
    }
}

void FileManager::saveScheduleToStream(QTextStream& out, const TransportSchedule& schedule) const {
    out << schedule.departureCity << "\n";
    out << schedule.arrivalCity << "\n";
    out << schedule.departureDate.toString(Qt::ISODate) << "\n";
    out << schedule.arrivalDate.toString(Qt::ISODate) << "\n";
    out << schedule.price << "\n";
    out << schedule.availableSeats << "\n";
}

Hotel FileManager::loadHotelFromStream(QTextStream& in) const {
    Hotel hotel;
    QString hotelName = in.readLine().trimmed();
    QString hotelCountry = in.readLine().trimmed();
    int hotelStars = in.readLine().toInt();
    QString hotelAddress = in.readLine().trimmed();
    int roomCount = in.readLine().toInt();
    
    hotel.setName(hotelName);
    hotel.setCountry(hotelCountry);
    hotel.setStars(hotelStars);
    hotel.setAddress(hotelAddress);
    
    for (int j = 0; j < roomCount; ++j) {
        Room room = loadRoomFromStream(in);
        hotel.addRoom(room);
    }
    
    return hotel;
}

Room FileManager::loadRoomFromStream(QTextStream& in) const {
    Room room;
    room.setName(in.readLine().trimmed());
    room.setRoomType(static_cast<Room::RoomType>(in.readLine().toInt()));
    room.setPricePerNight(in.readLine().toDouble());
    room.setCapacity(in.readLine().toInt());
    return room;
}

TransportCompany FileManager::loadTransportCompanyFromStream(QTextStream& in) const {
    TransportCompany company;
    company.setName(in.readLine());
    company.setTransportType(static_cast<TransportCompany::TransportType>(in.readLine().toInt()));
    
    int scheduleCount = in.readLine().toInt();
    for (int j = 0; j < scheduleCount; ++j) {
        TransportSchedule schedule = loadScheduleFromStream(in);
        company.addSchedule(schedule);
    }
    
    return company;
}

TransportSchedule FileManager::loadScheduleFromStream(QTextStream& in) const {
    TransportSchedule schedule;
    schedule.departureCity = in.readLine().trimmed();
    schedule.arrivalCity = in.readLine().trimmed();
    schedule.departureDate = QDate::fromString(in.readLine().trimmed(), Qt::ISODate);
    schedule.arrivalDate = QDate::fromString(in.readLine().trimmed(), Qt::ISODate);
    schedule.price = in.readLine().toDouble();
    schedule.availableSeats = in.readLine().toInt();
    return schedule;
}

