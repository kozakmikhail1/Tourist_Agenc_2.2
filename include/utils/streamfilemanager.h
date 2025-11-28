#ifndef STREAMFILEMANAGER_H
#define STREAMFILEMANAGER_H

#include "containers/datacontainer.h"
#include "models/country.h"
#include "models/order.h"
#include <string>
#include <fstream>
#include <exception>
#include <filesystem>

// Класс для работы с файлами через потоки ввода/вывода
// Демонстрирует: потоки ввода/вывода, работу с файлами, обработку исключений
class StreamFileManager {
public:
    explicit StreamFileManager(const std::string& basePath = "data");
    
    // Сохранение данных через потоки
    void saveCountries(const DataContainer<Country>& countries, const std::string& filename) const;
    void saveOrders(const DataContainer<Order>& orders, const std::string& filename) const;
    
    // Загрузка данных через потоки
    void loadCountries(DataContainer<Country>& countries, const std::string& filename) const;
    void loadOrders(DataContainer<Order>& orders, const std::string& filename) const;
    
    // Сохранение всех данных
    void saveAll(const DataContainer<Country>& countries,
                 const DataContainer<Order>& orders,
                 const std::string& basePath = "") const;
    
    // Загрузка всех данных
    void loadAll(DataContainer<Country>& countries,
                 DataContainer<Order>& orders,
                 const std::string& basePath = "") const;

private:
    std::string basePath_;
    
    // Вспомогательные методы
    void ensureDirectoryExists(const std::filesystem::path& path) const;
};

// Исключение для ошибок работы с файлами через потоки
class StreamFileException : public std::exception {
public:
    explicit StreamFileException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

#endif // STREAMFILEMANAGER_H



