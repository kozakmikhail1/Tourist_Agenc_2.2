#include "models/order.h"
#include <sstream>

int Order::nextId_ = 1;

Order::Order() : id_(nextId_++) {
    orderDate_ = QDateTime::currentDateTime();
}

Order::Order(const Tour& tour, const QString& clientName, const QString& clientPhone, const QString& clientEmail)
    : id_(nextId_++), tour_(tour), clientName_(clientName), 
      clientPhone_(clientPhone), clientEmail_(clientEmail) {
    orderDate_ = QDateTime::currentDateTime();
}

QString Order::toString() const {
    return QString("Order #%1: %2, Client: %3, Cost: %4, Status: %5")
        .arg(QString::number(id_), tour_.getName(), clientName_, 
             QString::number(getTotalCost(), 'f', 2), status_);
}

// Операторы теперь hidden friends, определены в заголовочном файле

// Работа с файлами через потоки
void Order::writeToFile(std::ofstream& ofs) const {
    ofs << "ORDER\n";
    ofs << id_ << "\n";
    ofs << tour_.getName().toStdString() << "\n";
    ofs << clientName_.toStdString() << "\n";
    ofs << clientPhone_.toStdString() << "\n";
    ofs << orderDate_.toString(Qt::ISODate).toStdString() << "\n";
    ofs << status_.toStdString() << "\n";
}

void Order::readFromFile(std::ifstream& ifs) {
    std::string line;
    std::getline(ifs, line); // Пропускаем заголовок "ORDER"
    
    std::string tourName;
    std::string clientName;
    std::string clientPhone;
    std::string dateStr;
    std::string status;
    ifs >> id_;
    ifs.ignore(); // Пропускаем символ новой строки
    std::getline(ifs, tourName);
    std::getline(ifs, clientName);
    std::getline(ifs, clientPhone);
    std::getline(ifs, dateStr);
    std::getline(ifs, status);
    
    clientName_ = QString::fromStdString(clientName);
    clientPhone_ = QString::fromStdString(clientPhone);
    orderDate_ = QDateTime::fromString(QString::fromStdString(dateStr), Qt::ISODate);
    status_ = QString::fromStdString(status);
}
