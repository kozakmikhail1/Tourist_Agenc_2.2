// Генерирует UTF-8 data/*.txt в формате AgencyData (9 полей тура, checksum).
// Сборка: g++ -std=c++17 -O2 tools/generate_sample_data.cpp -o tools/generate_sample_data.exe
// Запуск из корня репозитория: tools\generate_sample_data.exe

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static unsigned int hashPassword(const std::string& login, const std::string& password)
{
    const std::string combined = login + password;
    unsigned int h = 5381U;
    constexpr unsigned int kPrime = 131U;
    for (unsigned char ch : combined) {
        h = (h * kPrime) ^ static_cast<unsigned int>(ch);
    }
    return h;
}

static unsigned int simpleChecksum(const std::string& text)
{
    unsigned int s = 0U;
    for (unsigned char ch : text) {
        s = s * 131U + static_cast<unsigned int>(ch);
    }
    return s;
}

static void writeUtf8(const std::string& path, const std::string& content)
{
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

struct Account {
    std::string login;
    std::string password;
    bool isAdmin;
};

struct TourRecord {
    int id;
    std::string country;
    int price;
    int days;
    std::string hotel;
    std::string address;
    int stars;
    std::string dateStart;
    std::string dateEnd;
};

struct BookingRecord {
    int id;
    std::string userLogin;
    int tourId;
    bool cancelled;
};

int main()
{
    const std::string base = "data/";

    const std::vector<Account> accountsSeed = {
        {"admin", "admin", true},
        {"operator1", "OpSecure1", true},
        {"operator2", "ManageTours2", true},
        {"manager_anna", "AnnaAdmin9", true},
        {"maria_tourist", "travel2024", false},
        {"ivan_user", "mypass99", false},
        {"alex_trip", "alex1234", false},
        {"kate_sun", "sunpass88", false},
        {"oleg_travel", "oleg2026", false},
        {"nina_holiday", "nina7777", false},
        {"petr_client", "petrpass5", false},
        {"sergey_go", "go2026go", false},
        {"lena_beach", "beach2026", false},
        {"maks_resort", "Maks2026x", false},
        {"elena_voyage", "ElenaV99", false},
        {"dmitry_sea", "SeaDm77", false},
        {"svetlana_ski", "SkiSv55", false},
        {"andrey_city", "CityAnd3", false},
        {"olga_island", "IslandOg8", false},
        {"viktor_cruise", "CruiseVk1", false},
        {"natalia_spa", "SpaNat22", false},
        {"roman_hike", "HikeRo44", false},
        {"julia_wine", "WineJu66", false},
        {"kirill_north", "NorthKi88", false},
        {"tatiana_lake", "LakeTa11", false},
    };

    std::ostringstream accStream;
    for (const auto& acc : accountsSeed) {
        accStream << acc.login << ';' << hashPassword(acc.login, acc.password) << ';' << (acc.isAdmin ? 1 : 0)
                  << '\n';
    }
    const std::string accBody = accStream.str();
    const std::string accounts = std::string("# login;passwordHash;isAdmin\n") + accBody
        + "#checksum_accounts " + std::to_string(simpleChecksum(accBody)) + "\n";

    const std::vector<TourRecord> toursSeed = {
        {1, "Turkey", 45900, 7, "Hotel Marina", "Kemer Cad. 12, Antalya", 4, "2026-06-01", "2026-06-07"},
        {2, "Egypt", 62900, 10, "Pyramids Resort", "Giza Plateau Road 3, Cairo", 5, "2026-05-10", "2026-05-19"},
        {3, "Greece", 38900, 6, "Athens View", "Plaka 5, Athens", 4, "2026-07-05", "2026-07-10"},
        {4, "Spain", 71900, 8, "Barcelona Central", "Rambla 140, Barcelona", 4, "2026-08-12", "2026-08-19"},
        {5, "UAE", 98900, 6, "Dubai Palm", "Palm Jumeirah East, Dubai", 5, "2026-09-01", "2026-09-06"},
        {6, "Montenegro", 34900, 5, "Bay Hotel", "Budva Riviera 8", 3, "2026-06-20", "2026-06-24"},
        {7, "Thailand", 54900, 12, "Phuket Garden", "Patong Beach Road 22", 4, "2026-11-01", "2026-11-12"},
        {8, "Cyprus", 41900, 7, "Aphrodite Beach", "Limassol Seafront 9", 4, "2026-10-03", "2026-10-09"},
        {9, "Italy", 67500, 8, "Roma Historic", "Via Nazionale 71, Rome", 4, "2026-04-15", "2026-04-22"},
        {10, "France", 82300, 9, "Paris Riverside", "Quai de la Seine 4, Paris", 4, "2026-05-20", "2026-05-28"},
        {11, "Japan", 132000, 10, "Tokyo Skyline", "Shinjuku 2-8-1, Tokyo", 5, "2026-03-08", "2026-03-17"},
        {12, "Vietnam", 58700, 11, "Saigon Pearl", "District 1, Ho Chi Minh", 4, "2026-12-01", "2026-12-11"},
        {13, "Indonesia", 61400, 10, "Bali Sun Resort", "Ubud Raya 15, Bali", 4, "2026-01-14", "2026-01-23"},
        {14, "Maldives", 149500, 7, "Coral Reef Villas", "North Male Atoll", 5, "2026-02-01", "2026-02-07"},
        {15, "Georgia", 29500, 5, "Tbilisi Old Town", "Shardeni 3, Tbilisi", 3, "2026-07-18", "2026-07-22"},
        {16, "Armenia", 28900, 5, "Yerevan Plaza", "Republic Square 1, Yerevan", 3, "2026-08-05", "2026-08-09"},
        {17, "Portugal", 69900, 8, "Lisbon Harbor", "Belem waterfront 6, Lisbon", 4, "2026-09-12", "2026-09-19"},
        {18, "Czechia", 47200, 6, "Prague Castle Inn", "Mala Strana 10, Prague", 4, "2026-06-25", "2026-06-30"},
        {19, "Austria", 73100, 7, "Vienna Classic", "Ringstrasse 55, Vienna", 4, "2026-10-10", "2026-10-16"},
        {20, "South Korea", 118000, 9, "Seoul Modern", "Gangnam-daero 411, Seoul", 5, "2026-04-01", "2026-04-09"},
        {21, "Croatia", 51200, 7, "Dubrovnik Walls", "Old Town Pile 2", 4, "2026-07-01", "2026-07-07"},
        {22, "Morocco", 55800, 8, "Marrakech Riad", "Medina Derb 7, Marrakech", 4, "2026-11-15", "2026-11-22"},
        {23, "Mexico", 89500, 9, "Cancun Bay", "Hotel Zone Km 8, Cancun", 5, "2026-12-18", "2026-12-26"},
        {24, "Norway", 98800, 6, "Bergen Fjord", "Bryggen 14, Bergen", 4, "2026-08-01", "2026-08-06"},
        {25, "Switzerland", 112000, 7, "Alpine Lodge", "Grindelwald Dorfstr. 90", 5, "2026-01-20", "2026-01-26"},
        {26, "Germany", 54800, 5, "Munich Oktober", "Ludwigstr. 33, Munich", 4, "2026-09-20", "2026-09-24"},
        {27, "Netherlands", 49800, 6, "Amsterdam Canal", "Keizersgracht 120", 4, "2026-05-05", "2026-05-10"},
        {28, "Poland", 35600, 5, "Krakow Old Square", "Rynek Glowny 5", 3, "2026-06-10", "2026-06-14"},
        {29, "Tunisia", 31200, 7, "Hammamet Beach", "Yasmine Hammamet Bd", 4, "2026-10-25", "2026-10-31"},
        {30, "Sri Lanka", 67800, 10, "Colombo Pearl", "Galle Face 2, Colombo", 4, "2026-11-08", "2026-11-17"},
    };

    std::ostringstream toursStream;
    const std::vector<std::string> titleAdj = {
        "Azure", "Solar", "Velvet", "Northern", "Golden", "Crystal", "Ocean", "Summit", "Urban", "Breeze"};
    const std::vector<std::string> titleNoun = {
        "Pulse", "Voyage", "Route", "Escape", "Horizon", "Motion", "Season", "Rhythm", "Panorama", "Oasis"};
    for (const auto& tour : toursSeed) {
        const std::string title = titleAdj[(tour.id * 5 + tour.days) % titleAdj.size()] + ' '
            + titleNoun[(tour.id * 3 + tour.stars) % titleNoun.size()];
        toursStream << tour.id << ';' << tour.country << ';' << title << ';' << tour.price << ';'
                    << tour.days << ';' << tour.hotel << ';' << tour.address << ';' << tour.stars << ';'
                    << tour.dateStart << ';' << tour.dateEnd << '\n';
    }
    const std::string toursBody = toursStream.str();
    const std::string tours =
        std::string("# id;country;title;price;days;hotel;hotelAddress;hotelStars;dateStart;dateEnd\n")
        + toursBody + "#checksum_tours " + std::to_string(simpleChecksum(toursBody)) + "\n";

    const std::vector<BookingRecord> bookingsSeed = {
        {1, "maria_tourist", 1, false},
        {2, "maria_tourist", 3, false},
        {3, "ivan_user", 2, false},
        {4, "ivan_user", 7, true},
        {5, "maria_tourist", 5, false},
        {6, "alex_trip", 9, false},
        {7, "alex_trip", 14, true},
        {8, "kate_sun", 8, false},
        {9, "kate_sun", 12, false},
        {10, "oleg_travel", 6, false},
        {11, "oleg_travel", 15, true},
        {12, "nina_holiday", 4, false},
        {13, "nina_holiday", 17, false},
        {14, "petr_client", 18, false},
        {15, "petr_client", 19, false},
        {16, "sergey_go", 10, false},
        {17, "sergey_go", 20, false},
        {18, "lena_beach", 13, false},
        {19, "lena_beach", 14, true},
        {20, "maria_tourist", 11, false},
        {21, "ivan_user", 16, false},
        {22, "alex_trip", 2, false},
        {23, "kate_sun", 1, true},
        {24, "nina_holiday", 5, false},
        {25, "petr_client", 7, false},
        {26, "maks_resort", 21, false},
        {27, "maks_resort", 25, false},
        {28, "elena_voyage", 22, false},
        {29, "elena_voyage", 10, false},
        {30, "dmitry_sea", 23, false},
        {31, "dmitry_sea", 14, false},
        {32, "svetlana_ski", 24, false},
        {33, "svetlana_ski", 28, true},
        {34, "andrey_city", 26, false},
        {35, "andrey_city", 27, false},
        {36, "olga_island", 29, false},
        {37, "olga_island", 30, false},
        {38, "viktor_cruise", 4, false},
        {39, "viktor_cruise", 8, false},
        {40, "natalia_spa", 14, false},
        {41, "natalia_spa", 5, false},
        {42, "roman_hike", 6, false},
        {43, "julia_wine", 17, false},
        {44, "kirill_north", 24, false},
        {45, "tatiana_lake", 3, false},
        {46, "maks_resort", 12, false},
        {47, "lena_beach", 30, false},
        {48, "sergey_go", 21, true},
        {49, "dmitry_sea", 1, false},
        {50, "elena_voyage", 20, false},
    };

    std::ostringstream bookingsStream;
    for (const auto& booking : bookingsSeed) {
        bookingsStream << booking.id << ';' << booking.userLogin << ';' << booking.tourId << ';'
                       << (booking.cancelled ? 1 : 0) << '\n';
    }
    const std::string bookBody = bookingsStream.str();
    const std::string bookings = std::string("# id;userLogin;tourId;cancelled\n") + bookBody
        + "#checksum_bookings " + std::to_string(simpleChecksum(bookBody)) + "\n";

    writeUtf8(base + "accounts.txt", accounts);
    writeUtf8(base + "tours.txt", tours);
    writeUtf8(base + "bookings.txt", bookings);

    std::cout << "Written: " << base << "accounts.txt, tours.txt, bookings.txt\n";
    std::cout << "Users: " << accountsSeed.size() << ", tours: " << toursSeed.size()
              << ", bookings: " << bookingsSeed.size() << '\n';
    return 0;
}
