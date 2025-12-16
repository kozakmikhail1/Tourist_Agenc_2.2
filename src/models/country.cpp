#include "models/country.h"
#include <sstream>

Country::Country(const QString& name, const QString& continent)
    : TouristService(name, 0.0), continent_(continent) {
}

QString Country::getDescription() const {
    return QString("Country: %1, Continent: %2, Capital: %3")
        .arg(getName(), continent_, capital_);
}

Country& Country::operator=(const Country& other) {
    if (this != &other) {
        setName(other.getName());
        continent_ = other.continent_;
        capital_ = other.capital_;
        currency_ = other.currency_;
        setPrice(other.getPrice());
    }
    return *this;
}

void Country::writeToFile(std::ofstream& ofs) const {
    ofs << "COUNTRY\n";
    ofs << getName().toStdString() << "\n";
    ofs << continent_.toStdString() << "\n";
    ofs << capital_.toStdString() << "\n";
    ofs << currency_.toStdString() << "\n";
}

void Country::readFromFile(std::ifstream& ifs) {
    std::string line;
    std::getline(ifs, line);
    
    std::string name;
    std::string continent;
    std::string capital;
    std::string currency;
    std::getline(ifs, name);
    std::getline(ifs, continent);
    std::getline(ifs, capital);
    std::getline(ifs, currency);
    
    setName(QString::fromStdString(name));
    continent_ = QString::fromStdString(continent);
    capital_ = QString::fromStdString(capital);
    currency_ = QString::fromStdString(currency);
}
