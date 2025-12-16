#ifndef COUNTRY_H
#define COUNTRY_H

#include "models/touristservice.h"
#include <QString>
#include <iostream>
#include <fstream>

class Country : public TouristService {
public:
    explicit Country(const QString& name = "", const QString& continent = "");
    
    QString getType() const override { return "Country"; }
    QString getDescription() const override;
    
    QString getContinent() const { return continent_; }
    void setContinent(const QString& continent) { continent_ = continent; }
    
    QString getCapital() const { return capital_; }
    void setCapital(const QString& capital) { capital_ = capital; }
    
    QString getCurrency() const { return currency_; }
    void setCurrency(const QString& currency) { currency_ = currency; }
    
    friend bool operator==(const Country& lhs, const Country& rhs) {
        return lhs.getName() == rhs.getName() && lhs.continent_ == rhs.continent_ &&
               lhs.capital_ == rhs.capital_ && lhs.currency_ == rhs.currency_;
    }
    
    friend bool operator!=(const Country& lhs, const Country& rhs) {
        return !(lhs == rhs);
    }
    
    Country& operator=(const Country& other);
    
    friend std::ostream& operator<<(std::ostream& os, const Country& country) {
        os << country.getName().toStdString() << "\n"
           << country.continent_.toStdString() << "\n"
           << country.capital_.toStdString() << "\n"
           << country.currency_.toStdString() << "\n";
        return os;
    }
    
    friend std::istream& operator>>(std::istream& is, Country& country) {
        std::string name;
        std::string continent;
        std::string capital;
        std::string currency;
        std::getline(is, name);
        std::getline(is, continent);
        std::getline(is, capital);
        std::getline(is, currency);
        
        country.setName(QString::fromStdString(name));
        country.continent_ = QString::fromStdString(continent);
        country.capital_ = QString::fromStdString(capital);
        country.currency_ = QString::fromStdString(currency);
        
        return is;
    }
    
    void writeToFile(std::ofstream& ofs) const;
    void readFromFile(std::ifstream& ifs);

private:
    QString continent_;
    QString capital_ = "";
    QString currency_ = "";
};

#endif



