cmake_minimum_required(VERSION 3.16)
project(TouristAgency VERSION 1.0.0 LANGUAGES CXX)

# Экспорт compile_commands.json для SonarQube анализа
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Для MSVC требуются флаги для правильной поддержки C++17
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
    # Статическая линковка runtime библиотек для избежания проблем с DLL
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    # Альтернативный вариант - динамическая линковка, но с правильной версией
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt6_standard_project_setup()

# Исходные файлы (.cpp)
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/models/country.cpp
    src/models/transportcompany.cpp
    src/models/hotel.cpp
    src/models/room.cpp
    src/models/tour.cpp
    src/models/order.cpp
    src/models/touristservice.cpp
    src/containers/datacontainer.cpp
    src/utils/filemanager.cpp
    src/utils/streamfilemanager.cpp
    src/dialogs/countrydialog.cpp
    src/dialogs/companydialog.cpp
    src/dialogs/hoteldialog.cpp
    src/dialogs/tourdialog.cpp
    src/dialogs/searchdialog.cpp
    src/dialogs/booktourdialog.cpp
    src/dialogs/roomdialog.cpp
    src/dialogs/scheduledialog.cpp
    src/dialogs/toursetuphelper.cpp
    src/dialogs/booktourcostcalculator.cpp
    src/mainwindow/tablemanager.cpp
    src/mainwindow/filtermanager.cpp
    src/mainwindow/filtercomboupdater.cpp
    src/mainwindow/actions/action.cpp
    src/mainwindow/actions/countryactions.cpp
)

# Заголовочные файлы (.h) - теперь в include/
set(HEADERS
    include/mainwindow.h
    include/models/country.h
    include/models/transportcompany.h
    include/models/hotel.h
    include/models/room.h
    include/models/tour.h
    include/models/order.h
    include/models/touristservice.h
    include/containers/datacontainer.h
    include/containers/iterator.h
    include/utils/filemanager.h
    include/utils/streamfilemanager.h
    include/dialogs/countrydialog.h
    include/dialogs/companydialog.h
    include/dialogs/hoteldialog.h
    include/dialogs/tourdialog.h
    include/dialogs/searchdialog.h
    include/dialogs/booktourdialog.h
    include/dialogs/roomdialog.h
    include/dialogs/scheduledialog.h
    include/dialogs/toursetuphelper.h
    include/dialogs/booktourcostcalculator.h
    include/mainwindow/tablemanager.h
    include/mainwindow/filtermanager.h
    include/mainwindow/filtercomboupdater.h
    include/mainwindow/actions/action.h
    include/mainwindow/actions/countryactions.h
)

# UI файлы (остаются в src/)
set(UI_FILES
    src/mainwindow.ui
    src/dialogs/countrydialog.ui
    src/dialogs/companydialog.ui
    src/dialogs/hoteldialog.ui
    src/dialogs/tourdialog.ui
    src/dialogs/searchdialog.ui
    src/dialogs/booktourdialog.ui
    src/dialogs/roomdialog.ui
    src/dialogs/scheduledialog.ui
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES})

# Установка include директорий - важно делать это после add_executable
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Для правильной работы с codecvt и стандартной библиотекой
if(MSVC)
    # Убеждаемся, что используется правильная версия стандартной библиотеки
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _HAS_STD_BYTE=0
    )
endif()

# Установка правильных путей для поиска DLL
if(WIN32)
    # Добавляем путь к Qt DLL в переменную окружения PATH для запуска
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=${Qt6_DIR}/../../../bin;$ENV{PATH}"
    )
endif()
