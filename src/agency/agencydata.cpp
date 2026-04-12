#include "agency/agencydata.h"
#include "agency/algorithms.h"
#include "agency/validation.h"

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QStringConverter>
#include <algorithm>

namespace agency {

namespace {

QString usersFilePath(const QString& dir)
{
    return QDir(dir).filePath(QStringLiteral("accounts.txt"));
}

QString toursFilePath(const QString& dir)
{
    return QDir(dir).filePath(QStringLiteral("tours.txt"));
}

QString bookingsFilePath(const QString& dir)
{
    return QDir(dir).filePath(QStringLiteral("bookings.txt"));
}

void ensureDataDir(const QString& dir)
{
    QDir d;
    d.mkpath(dir);
}

quint32 simpleChecksum(const QString& text)
{
    quint32 s = 0;
    const QChar* p = text.constData();
    const int n = text.size();
    for (int i = 0; i < n; ++i) {
        s = s * 131u + static_cast<quint32>(p[i].unicode());
    }
    return s;
}

/** РЈРґР°Р»СЏРµС‚ Р·Р°РІРµСЂС€Р°СЋС‰СѓСЋ СЃС‚СЂРѕРєСѓ РєРѕРЅС‚СЂРѕР»СЊРЅРѕР№ СЃСѓРјРјС‹, РµСЃР»Рё РµСЃС‚СЊ. */
bool stripChecksumLine(QStringList& lines, const QString& tag, quint32* expected, bool* found)
{
    *found = false;
    *expected = 0;
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }
    if (lines.isEmpty()) {
        return true;
    }
    const QString last = lines.last().trimmed();
    if (!last.startsWith(tag)) {
        return true;
    }
    const QString numPart = last.mid(tag.length()).trimmed();
    bool ok = false;
    const quint32 v = numPart.toUInt(&ok);
    if (!ok) {
        return false;
    }
    *expected = v;
    *found = true;
    lines.removeLast();
    return true;
}

QStringList readAllLines(QFile& f)
{
    QStringList lines;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        QString line = in.readLine();
        // UTF-8 BOM РІ РЅР°С‡Р°Р»Рµ С„Р°Р№Р»Р° РјРѕР¶РµС‚ РѕСЃС‚Р°С‚СЊСЃСЏ РЅР° РїРµСЂРІРѕР№ СЃС‚СЂРѕРєРµ
        // Иногда BOM попадает в первый символ первой строки — убираем его вручную.
        if (!line.isEmpty() && line.at(0) == QChar(0xFEFF)) {
            line.remove(0, 1);
        }
        lines.append(line);
    }
    return lines;
}

int toursFileQualityScore(const QString& dir)
{
    QFile f(toursFilePath(dir));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    const QStringList lines = readAllLines(f);
    f.close();

    int totalRows = 0;
    int rowsExtended = 0;
    int nonEmptyAddress = 0;
    int placeholderAddress = 0;
    QSet<QString> startDates;
    QSet<int> starsValues;

    for (const QString& raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }

        const QStringList parts = line.split(QLatin1Char(';'));
        if (parts.size() < 5) {
            continue;
        }
        ++totalRows;

        int addrIndex = -1;
        int starsIndex = -1;
        int startIndex = -1;
        if (parts.size() >= 10) {
            addrIndex = 6;
            starsIndex = 7;
            startIndex = 8;
        } else if (parts.size() >= 9) {
            addrIndex = 5;
            starsIndex = 6;
            startIndex = 7;
        } else {
            continue;
        }
        ++rowsExtended;

        const QString addr = parts[addrIndex].trimmed();
        if (!addr.isEmpty()) {
            ++nonEmptyAddress;
        }
        const QString addrLower = addr.toLower();
        if (addrLower.contains(QStringLiteral("адрес не указан"))
            || addrLower.contains(QStringLiteral("устаревш"))
            || addrLower.contains(QStringLiteral("р°рґс"))) {
            ++placeholderAddress;
        }

        bool starsOk = false;
        const int stars = parts[starsIndex].trimmed().toInt(&starsOk);
        if (starsOk) {
            starsValues.insert(stars);
        }

        const QString ds = parts[startIndex].trimmed();
        if (!ds.isEmpty()) {
            startDates.insert(ds);
        }
    }

    if (totalRows == 0) {
        return 0;
    }

    int score = 0;
    score += rowsExtended * 4;
    score += nonEmptyAddress * 5;
    score -= placeholderAddress * 8;
    score += std::min(static_cast<int>(startDates.size()), 12) * 2;
    score += std::min(static_cast<int>(starsValues.size()), 5) * 2;
    score -= std::max(0, totalRows - rowsExtended) * 6;
    return score;
}

ValidationResult validateTourTitle(const QString& title)
{
    const QString t = title.trimmed();
    if (t.isEmpty()) {
        return ValidationResult::failure(QStringLiteral("Укажите название тура."));
    }
    if (t.contains(QLatin1Char(';'))) {
        return ValidationResult::failure(QStringLiteral("Название тура не может содержать символ «;»."));
    }
    if (t.size() > 200) {
        return ValidationResult::failure(QStringLiteral("Название тура — до 200 символов."));
    }
    return ValidationResult::success();
}

} // namespace

AgencyData& AgencyData::instance()
{
    static AgencyData inst;
    return inst;
}

AgencyData::AgencyData() = default;

AgencyData::~AgencyData()
{
    clearAll();
}

QString AgencyData::dataDirectory() const
{
    // Ищем data не только рядом с exe, но и выше по дереву директорий.
    // Это упрощает запуск из IDE, где рабочая директория может отличаться.
    // РС‰РµРј РїР°РїРєСѓ data РІ РЅРµСЃРєРѕР»СЊРєРёС… С‚РёРїРёС‡РЅС‹С… РјРµСЃС‚Р°С…: СЂСЏРґРѕРј СЃ .exe, РІРІРµСЂС… РїРѕ РґРµСЂРµРІСѓ РѕС‚ exe,
    // РѕС‚ С‚РµРєСѓС‰РµРіРѕ РєР°С‚Р°Р»РѕРіР° РїСЂРѕС†РµСЃСЃР° (С‡Р°СЃС‚Рѕ РєРѕСЂРµРЅСЊ РїСЂРѕРµРєС‚Р° РІ Qt Creator), РІРІРµСЂС… РѕС‚ cwd.
    QSet<QString> seen;
    QStringList candidates;
    auto push = [&](const QString& path) {
        const QString abs = QDir(path).absolutePath();
        if (seen.contains(abs)) {
            return;
        }
        seen.insert(abs);
        candidates.append(abs);
    };

    const QString appDir = QCoreApplication::applicationDirPath();
    push(QDir(appDir).filePath(QStringLiteral("data")));

    QDir fromExe(appDir);
    for (int i = 0; i < 18; ++i) {
        push(fromExe.filePath(QStringLiteral("data")));
        if (!fromExe.cdUp()) {
            break;
        }
    }

    const QString cwd = QDir::currentPath();
    push(QDir(cwd).filePath(QStringLiteral("data")));
    QDir fromCwd(cwd);
    for (int i = 0; i < 12; ++i) {
        push(fromCwd.filePath(QStringLiteral("data")));
        if (!fromCwd.cdUp()) {
            break;
        }
    }

    int bestScore = -1;
    QString bestDir;
    for (const QString& dir : candidates) {
        const bool hasAccounts = QFile::exists(usersFilePath(dir));
        const bool hasTours = QFile::exists(toursFilePath(dir));
        const bool hasBookings = QFile::exists(bookingsFilePath(dir));
        // Предпочитаем директории, где туры уже в полном формате и с реальными адресами.
        int score = (hasTours ? 40 : 0) + (hasAccounts ? 20 : 0) + (hasBookings ? 10 : 0);
        if (hasTours) {
            score += toursFileQualityScore(dir);
        }
        if (score > bestScore) {
            bestScore = score;
            bestDir = dir;
        }
    }
    if (bestScore > 0) {
        return bestDir;
    }
    return QDir(appDir).filePath(QStringLiteral("data"));
}

void AgencyData::appendMessage(const QString& msg)
{
    lastMessages_.append(msg);
}

void AgencyData::clearAll()
{
    deleteTourNodes();
    deleteUserNodes();
    deleteBookingNodes();
    toursHead_ = nullptr;
    usersHead_ = nullptr;
    bookingsHead_ = nullptr;
    currentUser_ = nullptr;
    loginTable_.clear();
}

void AgencyData::deleteTourNodes()
{
    Tour* t = toursHead_;
    while (t) {
        Tour* nx = t->next;
        delete t;
        t = nx;
    }
}

void AgencyData::deleteUserNodes()
{
    User* u = usersHead_;
    while (u) {
        User* nx = u->next;
        delete u;
        u = nx;
    }
}

void AgencyData::deleteBookingNodes()
{
    Booking* b = bookingsHead_;
    while (b) {
        Booking* nx = b->next;
        delete b;
        b = nx;
    }
}

void AgencyData::appendUser(User* u)
{
    u->next = usersHead_;
    usersHead_ = u;
}

void AgencyData::appendTour(Tour* t)
{
    t->next = toursHead_;
    toursHead_ = t;
}

void AgencyData::appendBooking(Booking* b)
{
    b->next = bookingsHead_;
    bookingsHead_ = b;
}

User* AgencyData::findUserByLogin(const QString& login) const
{
    for (User* u = usersHead_; u; u = u->next) {
        if (u->login == login) {
            return u;
        }
    }
    return nullptr;
}

bool AgencyData::tryLogin(const QString& login, const QString& passwordPlain)
{
    User* u = findUserByLogin(login);
    if (!u) {
        return false;
    }
    const QString h = hashPassword(login, passwordPlain);
    if (h != u->passwordHash) {
        return false;
    }
    currentUser_ = u;
    return true;
}

bool AgencyData::registerUser(const QString& login, const QString& passwordPlain, bool asAdmin)
{
    const ValidationResult lv = validateLogin(login);
    if (!lv.ok) {
        return false;
    }
    const ValidationResult pv = validatePassword(passwordPlain);
    if (!pv.ok) {
        return false;
    }
    const QString trimmed = login.trimmed();
    if (loginTable_.contains(trimmed)) {
        return false;
    }
    auto* u = new User;
    u->login = trimmed;
    u->passwordHash = hashPassword(trimmed, passwordPlain);
    u->isAdmin = asAdmin;
    u->next = nullptr;
    appendUser(u);
    loginTable_.insert(trimmed);
    return true;
}

bool AgencyData::removeUserByLogin(const QString& login)
{
    User** pp = &usersHead_;
    while (*pp) {
        if ((*pp)->login == login) {
            User* dead = *pp;
            *pp = dead->next;
            if (currentUser_ == dead) {
                currentUser_ = nullptr;
            }
            loginTable_.remove(login);
            delete dead;
            Booking** bp = &bookingsHead_;
            while (*bp) {
                if ((*bp)->userLogin == login) {
                    Booking* bd = *bp;
                    *bp = bd->next;
                    delete bd;
                } else {
                    bp = &(*bp)->next;
                }
            }
            return true;
        }
        pp = &(*pp)->next;
    }
    return false;
}

bool AgencyData::changeUserPassword(const QString& login, const QString& newPasswordPlain)
{
    const ValidationResult pv = validatePassword(newPasswordPlain);
    if (!pv.ok) {
        return false;
    }
    User* u = findUserByLogin(login);
    if (!u) {
        return false;
    }
    u->passwordHash = hashPassword(login, newPasswordPlain);
    return true;
}

bool AgencyData::changeUserLogin(const QString& oldLogin,
                                 const QString& newLogin,
                                 const QString& currentPasswordPlain)
{
    const ValidationResult lv = validateLogin(newLogin);
    if (!lv.ok) {
        return false;
    }
    const ValidationResult pv = validatePasswordLoginAttempt(currentPasswordPlain);
    if (!pv.ok) {
        return false;
    }
    User* u = findUserByLogin(oldLogin);
    if (!u) {
        return false;
    }
    const QString trimmedNew = newLogin.trimmed();
    if (trimmedNew == oldLogin) {
        return false;
    }
    if (findUserByLogin(trimmedNew)) {
        return false;
    }
    if (hashPassword(oldLogin, currentPasswordPlain) != u->passwordHash) {
        return false;
    }
    // После смены логина пересчитываем хеш с новым login,
    // потому что login входит в формулу hashPassword.
    loginTable_.remove(oldLogin);
    loginTable_.insert(trimmedNew);
    u->login = trimmedNew;
    u->passwordHash = hashPassword(trimmedNew, currentPasswordPlain);
    // Синхронизируем логин пользователя во всех его бронированиях.
    for (Booking* b = bookingsHead_; b; b = b->next) {
        if (b->userLogin == oldLogin) {
            b->userLogin = trimmedNew;
        }
    }
    return true;
}

bool AgencyData::updateUserAdminFlag(const QString& login, bool isAdmin)
{
    User* u = findUserByLogin(login);
    if (!u) {
        return false;
    }
    u->isAdmin = isAdmin;
    return true;
}

bool AgencyData::addTour(int id,
                         const QString& title,
                         const QString& country,
                         int price,
                         int days,
                         const QString& hotel,
                         const QString& hotelAddress,
                         int hotelStars,
                         const QString& dateStart,
                         const QString& dateEnd)
{
    if (findTourById(id)) {
        return false;
    }
    const ValidationResult nv = validateTourTitle(title);
    if (!nv.ok) {
        return false;
    }
    const ValidationResult tv =
        validateTourFields(country, price, days, hotel, hotelAddress, hotelStars, dateStart, dateEnd);
    if (!tv.ok) {
        return false;
    }
    auto* t = new Tour;
    t->id = id;
    t->title = title.trimmed();
    t->country = country.trimmed();
    t->price = price;
    t->days = days;
    t->hotel = hotel.trimmed();
    t->hotelAddress = hotelAddress.trimmed();
    t->hotelStars = hotelStars;
    t->dateStart = dateStart.trimmed();
    t->dateEnd = dateEnd.trimmed();
    t->next = nullptr;
    appendTour(t);
    return true;
}

bool AgencyData::updateTour(int id,
                            const QString& title,
                            const QString& country,
                            int price,
                            int days,
                            const QString& hotel,
                            const QString& hotelAddress,
                            int hotelStars,
                            const QString& dateStart,
                            const QString& dateEnd)
{
    Tour* t = findTourById(id);
    if (!t) {
        return false;
    }
    const ValidationResult nv = validateTourTitle(title);
    if (!nv.ok) {
        return false;
    }
    const ValidationResult tv =
        validateTourFields(country, price, days, hotel, hotelAddress, hotelStars, dateStart, dateEnd);
    if (!tv.ok) {
        return false;
    }
    t->title = title.trimmed();
    t->country = country.trimmed();
    t->price = price;
    t->days = days;
    t->hotel = hotel.trimmed();
    t->hotelAddress = hotelAddress.trimmed();
    t->hotelStars = hotelStars;
    t->dateStart = dateStart.trimmed();
    t->dateEnd = dateEnd.trimmed();
    return true;
}

bool AgencyData::deleteTourById(int id)
{
    Tour** pp = &toursHead_;
    while (*pp) {
        if ((*pp)->id == id) {
            Tour* dead = *pp;
            *pp = dead->next;
            delete dead;
            Booking** bp = &bookingsHead_;
            while (*bp) {
                if ((*bp)->tourId == id) {
                    Booking* bd = *bp;
                    *bp = bd->next;
                    delete bd;
                } else {
                    bp = &(*bp)->next;
                }
            }
            return true;
        }
        pp = &(*pp)->next;
    }
    return false;
}

Tour* AgencyData::findTourById(int id) const
{
    for (Tour* t = toursHead_; t; t = t->next) {
        if (t->id == id) {
            return t;
        }
    }
    return nullptr;
}

int AgencyData::nextTourId() const
{
    int m = 0;
    for (Tour* t = toursHead_; t; t = t->next) {
        m = std::max(m, t->id);
    }
    return m + 1;
}

int AgencyData::nextBookingId() const
{
    int m = 0;
    for (Booking* b = bookingsHead_; b; b = b->next) {
        m = std::max(m, b->id);
    }
    return m + 1;
}

bool AgencyData::addBooking(const QString& userLogin, int tourId)
{
    if (!findTourById(tourId)) {
        return false;
    }
    auto* b = new Booking;
    b->id = nextBookingId();
    b->userLogin = userLogin;
    b->tourId = tourId;
    b->cancelled = false;
    b->next = nullptr;
    appendBooking(b);
    return true;
}

bool AgencyData::cancelBookingForUser(int bookingId, const QString& userLogin)
{
    for (Booking* b = bookingsHead_; b; b = b->next) {
        if (b->id == bookingId && b->userLogin == userLogin && !b->cancelled) {
            b->cancelled = true;
            return true;
        }
    }
    return false;
}

QVector<Booking*> AgencyData::bookingsForUser(const QString& userLogin) const
{
    QVector<Booking*> out;
    for (Booking* b = bookingsHead_; b; b = b->next) {
        if (b->userLogin == userLogin) {
            out.push_back(b);
        }
    }
    return out;
}

QVector<Booking*> AgencyData::bookingsForTour(int tourId) const
{
    QVector<Booking*> out;
    for (Booking* b = bookingsHead_; b; b = b->next) {
        if (b->tourId == tourId) {
            out.push_back(b);
        }
    }
    return out;
}

QString AgencyData::bookedActiveToursSummaryForUser(const QString& userLogin) const
{
    QStringList parts;
    for (Booking* b = bookingsHead_; b; b = b->next) {
        if (b->userLogin != userLogin || b->cancelled) {
            continue;
        }
        Tour* t = findTourById(b->tourId);
        if (t) {
            parts.append(QStringLiteral("%1 (ID %2)").arg(tourDisplayName(t)).arg(t->id));
        } else {
            parts.append(QStringLiteral("ID %1 (тур не найден)").arg(b->tourId));
        }
    }
    if (parts.isEmpty()) {
        return QStringLiteral("\u2014");
    }
    return parts.join(QStringLiteral("; "));
}

void AgencyData::loadFromFiles()
{
    clearAll();
    lastMessages_.clear();

    const QString dir = dataDirectory();
    ensureDataDir(dir);

    const QString accPath = usersFilePath(dir);
    if (!QFile::exists(accPath)) {
        appendMessage(QStringLiteral("Файл accounts.txt не найден — будет создан при сохранении."));
    } else {
        QFile fu(accPath);
        if (!fu.open(QIODevice::ReadOnly | QIODevice::Text)) {
            appendMessage(QStringLiteral("Не удалось открыть accounts.txt для чтения."));
        } else {
            QStringList lines = readAllLines(fu);
            fu.close();

            quint32 expectedCk = 0;
            bool hasCk = false;
            if (!stripChecksumLine(lines, QStringLiteral("#checksum_accounts "), &expectedCk, &hasCk)) {
                appendMessage(QStringLiteral("accounts.txt: ошибка разбора контрольной суммы."));
            }

            QString accContentForVerify;
            QSet<QString> seenLogins;

            for (const QString& raw : lines) {
                const QString line = raw.trimmed();
                if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
                    continue;
                }
                const QStringList p = line.split(QLatin1Char(';'));
                if (p.size() < 3) {
                    appendMessage(QStringLiteral("accounts.txt: строка с неверным числом полей пропущена."));
                    continue;
                }
                const QString login = p[0];
                if (seenLogins.contains(login)) {
                    appendMessage(
                        QStringLiteral("accounts.txt: повтор логина «%1» — строка пропущена.").arg(login));
                    continue;
                }
                seenLogins.insert(login);
                const QString flag = (p[2].trimmed() == QLatin1String("1")) ? QLatin1String("1") : QLatin1String("0");
                accContentForVerify +=
                    QStringLiteral("%1;%2;%3\n").arg(p[0], p[1], flag);

                auto* u = new User;
                u->login = login;
                u->passwordHash = p[1];
                u->isAdmin = (flag == QLatin1String("1"));
                u->next = nullptr;
                appendUser(u);
            }

            if (hasCk) {
                if (simpleChecksum(accContentForVerify) != expectedCk) {
                    appendMessage(QStringLiteral(
                        "accounts.txt: РєРѕРЅС‚СЂРѕР»СЊРЅР°СЏ СЃСѓРјРјР° РЅРµ СЃРѕРІРїР°РґР°РµС‚ вЂ” РІРѕР·РјРѕР¶РЅС‹ СЂСѓС‡РЅС‹Рµ РїСЂР°РІРєРё РёР»Рё РїРѕРІСЂРµР¶РґРµРЅРёРµ."));
                }
            } else if (!lines.isEmpty()) {
                appendMessage(QStringLiteral("accounts.txt: нет контрольной суммы (устаревший формат)."));
            }
        }
    }

    if (!usersHead_) {
        registerUser(QStringLiteral("admin"), QStringLiteral("admin"), true);
        appendMessage(QStringLiteral("Создана учётная запись по умолчанию: admin / admin."));
    } else {
        loginTable_.rebuildFromUserList(usersHead_);
    }

    const QString tourPath = toursFilePath(dir);
    if (QFile::exists(tourPath)) {
        QFile ft(tourPath);
        if (!ft.open(QIODevice::ReadOnly | QIODevice::Text)) {
            appendMessage(QStringLiteral("Не удалось открыть tours.txt для чтения."));
        } else {
            QStringList lines = readAllLines(ft);
            ft.close();

            quint32 expectedCk = 0;
            bool hasCk = false;
            if (!stripChecksumLine(lines, QStringLiteral("#checksum_tours "), &expectedCk, &hasCk)) {
                appendMessage(QStringLiteral("tours.txt: ошибка разбора контрольной суммы."));
            }

            QString toursContentForVerify;
            QSet<int> seenTourIds;
            int legacyTourLineCount = 0;

            for (const QString& raw : lines) {
                const QString line = raw.trimmed();
                if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
                    continue;
                }
                const QStringList p = line.split(QLatin1Char(';'));
                if (p.size() < 5) {
                    appendMessage(QStringLiteral("tours.txt: строка с неверным форматом пропущена."));
                    continue;
                }
                bool ok = false;
                const int id = p[0].toInt(&ok);
                if (!ok || id < 1) {
                    appendMessage(QStringLiteral("tours.txt: неверный ID тура — строка пропущена."));
                    continue;
                }
                if (seenTourIds.contains(id)) {
                    appendMessage(QStringLiteral("tours.txt: повтор ID %1 — строка пропущена.").arg(id));
                    continue;
                }

                QString country = p[1];
                QString title;
                int price = 0;
                int days = 0;
                QString hotel;
                QString hotelAddress;
                int hotelStars = 3;
                QString dateStart;
                QString dateEnd;
                QString verifyLine;

                if (p.size() >= 10) {
                    title = p[2].trimmed();
                    price = p[3].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверная цена — строка пропущена."));
                        continue;
                    }
                    days = p[4].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверное число дней — строка пропущена."));
                        continue;
                    }
                    hotel = p[5];
                    hotelAddress = p[6].trimmed();
                    hotelStars = p[7].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверная звёздность — строка пропущена."));
                        continue;
                    }
                    dateStart = p[8].trimmed();
                    dateEnd = p[9].trimmed();
                    verifyLine = QStringLiteral("%1;%2;%3;%4;%5;%6;%7;%8;%9;%10\n")
                                     .arg(id)
                                     .arg(country)
                                     .arg(title)
                                     .arg(price)
                                     .arg(days)
                                     .arg(hotel)
                                     .arg(hotelAddress)
                                     .arg(hotelStars)
                                     .arg(dateStart)
                                     .arg(dateEnd);
                } else if (p.size() >= 9) {
                    price = p[2].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверная цена — строка пропущена."));
                        continue;
                    }
                    days = p[3].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверное число дней — строка пропущена."));
                        continue;
                    }
                    hotel = p[4];
                    hotelAddress = p[5].trimmed();
                    hotelStars = p[6].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверная звёздность — строка пропущена."));
                        continue;
                    }
                    dateStart = p[7].trimmed();
                    dateEnd = p[8].trimmed();
                    verifyLine = QStringLiteral("%1;%2;%3;%4;%5;%6;%7;%8;%9\n")
                                     .arg(id)
                                     .arg(country)
                                     .arg(price)
                                     .arg(days)
                                     .arg(hotel)
                                     .arg(hotelAddress)
                                     .arg(hotelStars)
                                     .arg(dateStart)
                                     .arg(dateEnd);
                } else if (p.size() == 5) {
                    // Обратная совместимость: поддержка старого формата тура (5 полей).
                    ++legacyTourLineCount;
                    price = p[2].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверная цена — строка пропущена."));
                        continue;
                    }
                    days = p[3].toInt(&ok);
                    if (!ok) {
                        appendMessage(QStringLiteral("tours.txt: неверное число дней — строка пропущена."));
                        continue;
                    }
                    hotel = p[4];
                    hotelAddress = QStringLiteral("адрес не указан (устаревший формат)");
                    hotelStars = 3;
                    const QDate base(2026, 6, 1);
                    dateStart = base.toString(Qt::ISODate);
                    dateEnd = base.addDays(std::max(0, days - 1)).toString(Qt::ISODate);
                    verifyLine = QStringLiteral("%1;%2;%3;%4;%5\n")
                                     .arg(id)
                                     .arg(country)
                                     .arg(price)
                                     .arg(days)
                                     .arg(hotel);
                } else {
                    appendMessage(QStringLiteral(
                        "tours.txt: ожидается 5, 9 или 10 полей — строка пропущена."));
                    continue;
                }

                const ValidationResult tv =
                    validateTourFields(country, price, days, hotel, hotelAddress, hotelStars, dateStart, dateEnd);
                if (!tv.ok) {
                    appendMessage(QStringLiteral("tours.txt: %1").arg(tv.errorMessage));
                    continue;
                }
                seenTourIds.insert(id);
                toursContentForVerify += verifyLine;

                auto* t = new Tour;
                t->id = id;
                t->title = title;
                t->country = country;
                t->price = price;
                t->days = days;
                t->hotel = hotel;
                t->hotelAddress = hotelAddress;
                t->hotelStars = hotelStars;
                t->dateStart = dateStart;
                t->dateEnd = dateEnd;
                t->next = nullptr;
                appendTour(t);
            }

            if (legacyTourLineCount > 0) {
                appendMessage(QStringLiteral(
                    "tours.txt: РїСЂРѕС‡РёС‚Р°РЅРѕ СЃС‚СЂРѕРє РІ СЃС‚Р°СЂРѕРј С„РѕСЂРјР°С‚Рµ (5 РїРѕР»РµР№): %1 вЂ” РґР»СЏ РЅРёС… Р·Р°РґР°РЅС‹ Р°РґСЂРµСЃ, Р·РІС‘Р·РґС‹ Рё РґР°С‚С‹ РїРѕ СѓРјРѕР»С‡Р°РЅРёСЋ.")
                                  .arg(legacyTourLineCount));
            }
            if (hasCk) {
                if (simpleChecksum(toursContentForVerify) != expectedCk) {
                    appendMessage(QStringLiteral(
                        "tours.txt: РєРѕРЅС‚СЂРѕР»СЊРЅР°СЏ СЃСѓРјРјР° РЅРµ СЃРѕРІРїР°РґР°РµС‚ вЂ” РІРѕР·РјРѕР¶РЅС‹ СЂСѓС‡РЅС‹Рµ РїСЂР°РІРєРё РёР»Рё РїРѕРІСЂРµР¶РґРµРЅРёРµ."));
                }
            } else if (!lines.isEmpty()) {
                appendMessage(QStringLiteral("tours.txt: нет контрольной суммы (устаревший формат)."));
            }
        }
    } else {
        appendMessage(QStringLiteral(
            "Р¤Р°Р№Р» tours.txt РЅРµ РЅР°Р№РґРµРЅ вЂ” РєР°С‚Р°Р»РѕРі С‚СѓСЂРѕРІ РїСѓСЃС‚. РџРѕР»РѕР¶РёС‚Рµ РїР°РїРєСѓ data СЂСЏРґРѕРј СЃ РїСЂРѕРіСЂР°РјРјРѕР№ РёР»Рё РІ РєРѕСЂРµРЅСЊ РїСЂРѕРµРєС‚Р°."));
    }

    const QString bookPath = bookingsFilePath(dir);
    if (QFile::exists(bookPath)) {
        QFile fb(bookPath);
        if (!fb.open(QIODevice::ReadOnly | QIODevice::Text)) {
            appendMessage(QStringLiteral("Не удалось открыть bookings.txt для чтения."));
        } else {
            QStringList lines = readAllLines(fb);
            fb.close();

            quint32 expectedCk = 0;
            bool hasCk = false;
            if (!stripChecksumLine(lines, QStringLiteral("#checksum_bookings "), &expectedCk, &hasCk)) {
                appendMessage(QStringLiteral("bookings.txt: ошибка разбора контрольной суммы."));
            }

            QString bookContentForVerify;

            for (const QString& raw : lines) {
                const QString line = raw.trimmed();
                if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
                    continue;
                }
                const QStringList p = line.split(QLatin1Char(';'));
                if (p.size() < 4) {
                    appendMessage(QStringLiteral("bookings.txt: строка с неверным форматом пропущена."));
                    continue;
                }
                bool ok = false;
                const int bid = p[0].toInt(&ok);
                if (!ok || bid < 1) {
                    appendMessage(QStringLiteral("bookings.txt: неверный ID бронирования — строка пропущена."));
                    continue;
                }
                const int tourId = p[2].toInt(&ok);
                if (!ok) {
                    appendMessage(QStringLiteral("bookings.txt: неверный tourId — строка пропущена."));
                    continue;
                }
                const QString ulogin = p[1];
                if (ulogin.isEmpty()) {
                    appendMessage(QStringLiteral("bookings.txt: пустой логин — строка пропущена."));
                    continue;
                }
                bookContentForVerify += QStringLiteral("%1;%2;%3;%4\n")
                                             .arg(bid)
                                             .arg(ulogin)
                                             .arg(tourId)
                                             .arg(p[3].trimmed() == QLatin1String("1") ? 1 : 0);

                auto* b = new Booking;
                b->id = bid;
                b->userLogin = ulogin;
                b->tourId = tourId;
                b->cancelled = (p[3].trimmed() == QLatin1String("1"));
                b->next = nullptr;
                appendBooking(b);

                if (!findUserByLogin(ulogin)) {
                    appendMessage(QStringLiteral(
                        "bookings.txt: Р±СЂРѕРЅРёСЂРѕРІР°РЅРёРµ %1 вЂ” РїРѕР»СЊР·РѕРІР°С‚РµР»СЊ В«%2В» РЅРµ РЅР°Р№РґРµРЅ (СЃРёСЂРѕС‚СЃРєР°СЏ Р·Р°РїРёСЃСЊ).")
                                      .arg(bid)
                                      .arg(ulogin));
                }
                if (!findTourById(tourId)) {
                    appendMessage(QStringLiteral(
                        "bookings.txt: Р±СЂРѕРЅРёСЂРѕРІР°РЅРёРµ %1 вЂ” С‚СѓСЂ %2 РЅРµ РЅР°Р№РґРµРЅ (СЃРёСЂРѕС‚СЃРєР°СЏ Р·Р°РїРёСЃСЊ).")
                                      .arg(bid)
                                      .arg(tourId));
                }
            }

            if (hasCk) {
                if (simpleChecksum(bookContentForVerify) != expectedCk) {
                    appendMessage(QStringLiteral(
                        "bookings.txt: РєРѕРЅС‚СЂРѕР»СЊРЅР°СЏ СЃСѓРјРјР° РЅРµ СЃРѕРІРїР°РґР°РµС‚ вЂ” РІРѕР·РјРѕР¶РЅС‹ СЂСѓС‡РЅС‹Рµ РїСЂР°РІРєРё РёР»Рё РїРѕРІСЂРµР¶РґРµРЅРёРµ."));
                }
            } else if (!lines.isEmpty()) {
                appendMessage(QStringLiteral("bookings.txt: нет контрольной суммы (устаревший формат)."));
            }
        }
    } else {
        appendMessage(QStringLiteral("bookings.txt not found - bookings history is empty."));
    }
}

bool AgencyData::saveToFiles()
{
    lastMessages_.clear();
    const QString dir = dataDirectory();
    ensureDataDir(dir);

    auto writeAccounts = [&]() -> bool {
        QSaveFile f(usersFilePath(dir));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            appendMessage(QStringLiteral("Ошибка записи accounts.txt."));
            return false;
        }
        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);
        out << "# login;passwordHash;isAdmin\n";
        QString accContent;
        for (User* u = usersHead_; u; u = u->next) {
            const QString line = QStringLiteral("%1;%2;%3\n")
                                     .arg(u->login, u->passwordHash,
                                          u->isAdmin ? QLatin1String("1") : QLatin1String("0"));
            out << line;
            accContent += line;
        }
        out << "#checksum_accounts " << simpleChecksum(accContent) << "\n";
        if (!f.commit()) {
            appendMessage(QStringLiteral("Не удалось завершить запись accounts.txt."));
            return false;
        }
        return true;
    };

    auto writeTours = [&]() -> bool {
        QSaveFile f(toursFilePath(dir));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            appendMessage(QStringLiteral("Ошибка записи tours.txt."));
            return false;
        }
        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);
        out << "# id;country;title;price;days;hotel;hotelAddress;hotelStars;dateStart;dateEnd\n";
        // Сохраняем туры в расширенном формате (10 полей, включая название тура).
        QString toursContent;
        for (Tour* t = toursHead_; t; t = t->next) {
            const QString line = QStringLiteral("%1;%2;%3;%4;%5;%6;%7;%8;%9;%10\n")
                                     .arg(t->id)
                                     .arg(t->country)
                                     .arg(t->title)
                                     .arg(t->price)
                                     .arg(t->days)
                                     .arg(t->hotel)
                                     .arg(t->hotelAddress)
                                     .arg(t->hotelStars)
                                     .arg(t->dateStart)
                                     .arg(t->dateEnd);
            out << line;
            toursContent += line;
        }
        out << "#checksum_tours " << simpleChecksum(toursContent) << "\n";
        if (!f.commit()) {
            appendMessage(QStringLiteral("Не удалось завершить запись tours.txt."));
            return false;
        }
        return true;
    };

    auto writeBookings = [&]() -> bool {
        QSaveFile f(bookingsFilePath(dir));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            appendMessage(QStringLiteral("Ошибка записи bookings.txt."));
            return false;
        }
        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);
        out << "# id;userLogin;tourId;cancelled\n";
        QString bookContent;
        for (Booking* b = bookingsHead_; b; b = b->next) {
            const QString line = QStringLiteral("%1;%2;%3;%4\n")
                                     .arg(b->id)
                                     .arg(b->userLogin)
                                     .arg(b->tourId)
                                     .arg(b->cancelled ? 1 : 0);
            out << line;
            bookContent += line;
        }
        out << "#checksum_bookings " << simpleChecksum(bookContent) << "\n";
        if (!f.commit()) {
            appendMessage(QStringLiteral("Не удалось завершить запись bookings.txt."));
            return false;
        }
        return true;
    };

    if (!writeAccounts()) {
        return false;
    }
    if (!writeTours()) {
        return false;
    }
    if (!writeBookings()) {
        return false;
    }
    return true;
}

} // namespace agency



