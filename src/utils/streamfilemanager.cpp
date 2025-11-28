#include "utils/streamfilemanager.h"
#include <iostream>
#include <filesystem>
#include <sstream>

StreamFileManager::StreamFileManager(const std::string& basePath)
    : basePath_(basePath) {
    ensureDirectoryExists(std::filesystem::path(basePath_));
}

void StreamFileManager::ensureDirectoryExists(const std::filesystem::path& path) const {
    try {
        if (!path.empty() && !std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
    } catch (const std::exception& e) {
        throw StreamFileException("Cannot create directory: " + std::string(e.what()));
    }
}

void StreamFileManager::saveCountries(const DataContainer<Country>& countries, 
                                       const std::string& filename) const {
    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    
    if (!ofs.is_open()) {
        throw StreamFileException("Cannot open file for writing: " + filename);
    }
    
    try {
        ofs << "COUNTRIES\n";
        ofs << countries.size() << "\n";
        
        for (const auto& country : countries.getData()) {
            country.writeToFile(ofs);
        }
    } catch (const std::exception& e) {
        ofs.close();
        throw StreamFileException("Error writing to file: " + std::string(e.what()));
    }
    
    ofs.close();
}

void StreamFileManager::loadCountries(DataContainer<Country>& countries, 
                                      const std::string& filename) const {
    std::ifstream ifs(filename, std::ios::in);
    
    if (!ifs.is_open()) {
        // Файл не существует - это нормально при первой загрузке
        return;
    }
    
    try {
        std::string header;
        std::getline(ifs, header);
        
        if (header != "COUNTRIES") {
            throw StreamFileException("Invalid file format: " + filename);
        }
        
        int count;
        ifs >> count;
        ifs.ignore(); // Пропускаем символ новой строки
        
        countries.clear();
        
        for (int i = 0; i < count; ++i) {
            Country country;
            country.readFromFile(ifs);
            countries.add(country);
        }
    } catch (const std::exception& e) {
        ifs.close();
        throw StreamFileException("Error reading from file: " + std::string(e.what()));
    }
    
    ifs.close();
}

void StreamFileManager::saveOrders(const DataContainer<Order>& orders, 
                                   const std::string& filename) const {
    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    
    if (!ofs.is_open()) {
        throw StreamFileException("Cannot open file for writing: " + filename);
    }
    
    try {
        ofs << "ORDERS\n";
        ofs << orders.size() << "\n";
        
        for (const auto& order : orders.getData()) {
            order.writeToFile(ofs);
        }
    } catch (const std::exception& e) {
        ofs.close();
        throw StreamFileException("Error writing to file: " + std::string(e.what()));
    }
    
    ofs.close();
}

void StreamFileManager::loadOrders(DataContainer<Order>& orders, 
                                   const std::string& filename) const {
    std::ifstream ifs(filename, std::ios::in);
    
    if (!ifs.is_open()) {
        // Файл не существует - это нормально при первой загрузке
        return;
    }
    
    try {
        std::string header;
        std::getline(ifs, header);
        
        if (header != "ORDERS") {
            throw StreamFileException("Invalid file format: " + filename);
        }
        
        int count;
        ifs >> count;
        ifs.ignore(); // Пропускаем символ новой строки
        
        orders.clear();
        
        for (int i = 0; i < count; ++i) {
            Order order;
            order.readFromFile(ifs);
            orders.add(order);
        }
    } catch (const std::exception& e) {
        ifs.close();
        throw StreamFileException("Error reading from file: " + std::string(e.what()));
    }
    
    ifs.close();
}

void StreamFileManager::saveAll(const DataContainer<Country>& countries,
                                const DataContainer<Order>& orders,
                                const std::string& basePath) const {
    std::string path = basePath.empty() ? basePath_ : basePath;
    ensureDirectoryExists(path);
    
    saveCountries(countries, path + "/countries_stream.txt");
    saveOrders(orders, path + "/orders_stream.txt");
}

void StreamFileManager::loadAll(DataContainer<Country>& countries,
                                DataContainer<Order>& orders,
                                const std::string& basePath) const {
    std::string path = basePath.empty() ? basePath_ : basePath;
    
    try {
        loadCountries(countries, path + "/countries_stream.txt");
        loadOrders(orders, path + "/orders_stream.txt");
    } catch (const StreamFileException& e) {
        // Игнорируем ошибки при первой загрузке
        std::cerr << "Warning: " << e.what() << std::endl;
    }
}



