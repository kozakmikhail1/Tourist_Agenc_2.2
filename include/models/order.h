#ifndef ORDER_H
#define ORDER_H

#include "models/tour.h"
#include <QString>
#include <QDate>
#include <QDateTime>
#include <iostream>
#include <fstream>

class Order {
public:
    Order();
    Order(const Tour& tour, const QString& clientName, const QString& clientPhone, const QString& clientEmail = "");
    
    int getId() const { return id_; }
    
    Tour getTour() const { return tour_; }
    void setTour(const Tour& tour) { tour_ = tour; }
    
    QString getClientName() const { return clientName_; }
    void setClientName(const QString& name) { clientName_ = name; }
    
    QString getClientPhone() const { return clientPhone_; }
    void setClientPhone(const QString& phone) { clientPhone_ = phone; }
    
    QString getClientEmail() const { return clientEmail_; }
    void setClientEmail(const QString& email) { clientEmail_ = email; }
    
    QDateTime getOrderDate() const { return orderDate_; }
    void setOrderDate(const QDateTime& date) { orderDate_ = date; }
    
    double getTotalCost() const { return tour_.calculateCost(); }
    
    QString getStatus() const { return status_; }
    void setStatus(const QString& status) { status_ = status; }
    
    QString toString() const;
    
    friend bool operator==(const Order& lhs, const Order& rhs) {
        return lhs.id_ == rhs.id_;
    }
    
    friend bool operator!=(const Order& lhs, const Order& rhs) {
        return !(lhs == rhs);
    }
    
    friend bool operator<(const Order& lhs, const Order& rhs) {
        return lhs.getTotalCost() < rhs.getTotalCost();
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Order& order) {
        os << order.id_ << "\n";
        os << order.tour_.getName().toStdString() << "\n";
        os << order.clientName_.toStdString() << "\n";
        os << order.clientPhone_.toStdString() << "\n";
        os << order.orderDate_.toString(Qt::ISODate).toStdString() << "\n";
        os << order.status_.toStdString() << "\n";
        return os;
    }
    
    friend std::istream& operator>>(std::istream& is, Order& order) {
        std::string tourName;
        std::string clientName;
        std::string clientPhone;
        std::string dateStr;
        std::string status;
        is >> order.id_;
        std::getline(is, tourName);
        std::getline(is, clientName);
        std::getline(is, clientPhone);
        std::getline(is, dateStr);
        std::getline(is, status);
        
        order.clientName_ = QString::fromStdString(clientName);
        order.clientPhone_ = QString::fromStdString(clientPhone);
        order.orderDate_ = QDateTime::fromString(QString::fromStdString(dateStr), Qt::ISODate);
        order.status_ = QString::fromStdString(status);
        
        return is;
    }
    
    void writeToFile(std::ofstream& ofs) const;
    void readFromFile(std::ifstream& ifs);

private:
    static int nextId_;
    int id_;
    Tour tour_;
    QString clientName_;
    QString clientPhone_;
    QString clientEmail_;
    QDateTime orderDate_ = QDateTime::currentDateTime();
    QString status_ = "В обработке";
};

#endif



