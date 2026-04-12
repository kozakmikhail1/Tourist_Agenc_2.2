#ifndef AGENCYDATA_H
#define AGENCYDATA_H

#include "agencytypes.h"
#include "loginhashtable.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace agency {

/** Ядро данных: связные списки, файлы, операции из ТЗ курсовой. */
class AgencyData {
public:
    static AgencyData& instance();

    void loadFromFiles();
    /** Сохраняет все файлы. false — не удалось записать хотя бы один файл (см. lastLoadMessages). */
    bool saveToFiles();

    /** Сообщения последней загрузки/сохранения (ошибки, предупреждения, контрольные суммы). */
    QStringList lastLoadMessages() const { return lastMessages_; }

    Tour* toursHead() const { return toursHead_; }
    User* usersHead() const { return usersHead_; }
    Booking* bookingsHead() const { return bookingsHead_; }

    User* currentUser() const { return currentUser_; }
    void setCurrentUser(User* u) { currentUser_ = u; }

    QString dataDirectory() const;

    User* findUserByLogin(const QString& login) const;
    bool tryLogin(const QString& login, const QString& passwordPlain);
    bool registerUser(const QString& login, const QString& passwordPlain, bool asAdmin);

    bool removeUserByLogin(const QString& login);
    bool changeUserPassword(const QString& login, const QString& newPasswordPlain);
    /** Смена логина: проверяется текущий пароль, хеш пересчитывается (логин входит в формулу хеша). */
    bool changeUserLogin(const QString& oldLogin, const QString& newLogin, const QString& currentPasswordPlain);
    bool updateUserAdminFlag(const QString& login, bool isAdmin);

    bool addTour(int id,
                 const QString& title,
                 const QString& country,
                 int price,
                 int days,
                 const QString& hotel,
                 const QString& hotelAddress,
                 int hotelStars,
                 const QString& dateStart,
                 const QString& dateEnd);
    bool updateTour(int id,
                    const QString& title,
                    const QString& country,
                    int price,
                    int days,
                    const QString& hotel,
                    const QString& hotelAddress,
                    int hotelStars,
                    const QString& dateStart,
                    const QString& dateEnd);
    bool deleteTourById(int id);
    Tour* findTourById(int id) const;

    int nextTourId() const;
    int nextBookingId() const;

    bool addBooking(const QString& userLogin, int tourId);
    bool cancelBookingForUser(int bookingId, const QString& userLogin);
    QVector<Booking*> bookingsForUser(const QString& userLogin) const;
    QVector<Booking*> bookingsForTour(int tourId) const;

    /** Активные (не отменённые) бронирования: краткая строка для таблицы администратора. */
    QString bookedActiveToursSummaryForUser(const QString& userLogin) const;

    LoginHashTable& loginTable() { return loginTable_; }

    void clearAll();

private:
    AgencyData();
    ~AgencyData();

    AgencyData(const AgencyData&) = delete;
    AgencyData& operator=(const AgencyData&) = delete;

    void appendUser(User* u);
    void appendTour(Tour* t);
    void appendBooking(Booking* b);

    void deleteTourNodes();
    void deleteUserNodes();
    void deleteBookingNodes();

    Tour* toursHead_ = nullptr;
    User* usersHead_ = nullptr;
    Booking* bookingsHead_ = nullptr;

    User* currentUser_ = nullptr;
    LoginHashTable loginTable_;

    QStringList lastMessages_;

    void appendMessage(const QString& msg);
};

} // namespace agency

#endif
