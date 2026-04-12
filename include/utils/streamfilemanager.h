#ifndef STREAMFILEMANAGER_H
#define STREAMFILEMANAGER_H

#include "containers/datacontainer.h"
#include "models/country.h"
#include "models/order.h"
#include <string>
#include <fstream>
#include <exception>
#include <filesystem>

class StreamFileManager {
public:
    explicit StreamFileManager(const std::string& basePath = "data");
    
    void saveCountries(const DataContainer<Country>& countries, const std::string& filename) const;
    void saveOrders(const DataContainer<Order>& orders, const std::string& filename) const;
    
    void loadCountries(DataContainer<Country>& countries, const std::string& filename) const;
    void loadOrders(DataContainer<Order>& orders, const std::string& filename) const;
    
    void saveAll(const DataContainer<Country>& countries,
                 const DataContainer<Order>& orders,
                 const std::string& basePath = "") const;
    
    void loadAll(DataContainer<Country>& countries,
                 DataContainer<Order>& orders,
                 const std::string& basePath = "") const;

private:
    std::string basePath_;
    
    void ensureDirectoryExists(const std::filesystem::path& path) const;
};

class StreamFileException : public std::exception {
public:
    explicit StreamFileException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

#endif



