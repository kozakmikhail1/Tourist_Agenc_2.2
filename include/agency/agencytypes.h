#ifndef AGENCYTYPES_H
#define AGENCYTYPES_H

#include <QString>

namespace agency {

struct Tour {
    int id = 0;
    QString title;
    QString country;
    int price = 0;
    int days = 0;
    /** Название отеля */
    QString hotel;
    QString hotelAddress;
    /** 1…5 */
    int hotelStars = 0;
    /** Даты проведения тура, формат yyyy-MM-dd */
    QString dateStart;
    QString dateEnd;
    Tour* next = nullptr;
};

struct User {
    QString login;
    QString passwordHash;
    bool isAdmin = false;
    User* next = nullptr;
};

struct Booking {
    int id = 0;
    QString userLogin;
    int tourId = 0;
    bool cancelled = false;
    Booking* next = nullptr;
};

} // namespace agency

#endif
