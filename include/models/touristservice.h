#ifndef TOURISTSERVICE_H
#define TOURISTSERVICE_H

#include <QString>
#include <QDate>
#include <memory>

// Абстрактный базовый класс для всех туристических услуг
// Демонстрирует: виртуальные функции и абстрактные классы
class TouristService {
public:
    explicit TouristService(const QString& name = "", double price = 0.0);
    virtual ~TouristService() = default;

    QString getName() const { return name_; }
    void setName(const QString& name) { name_ = name; }

    double getPrice() const { return price_; }
    void setPrice(double price) { price_ = price; }

    // Чистые виртуальные функции - делают класс абстрактным
    virtual QString getType() const = 0;
    virtual QString getDescription() const = 0;
    
    // Виртуальная функция с реализацией по умолчанию
    virtual double calculateCost() const { return price_; }
    
    // Виртуальная функция для вывода в поток
    virtual void writeToStream(std::ostream& os) const;
    virtual void readFromStream(std::istream& is);

protected:
    // Protected методы для доступа к приватным членам в производных классах
    void setNameProtected(const QString& name) { name_ = name; }
    void setPriceProtected(double price) { price_ = price; }
    const QString& getNameProtected() const { return name_; }
    double getPriceProtected() const { return price_; }

    // Hidden friend операторы для работы с потоками
    friend std::ostream& operator<<(std::ostream& os, const TouristService& service) {
        service.writeToStream(os);
        return os;
    }
    
    friend std::istream& operator>>(std::istream& is, TouristService& service) {
        service.readFromStream(is);
        return is;
    }

private:
    QString name_;
    double price_;
};

#endif // TOURISTSERVICE_H














