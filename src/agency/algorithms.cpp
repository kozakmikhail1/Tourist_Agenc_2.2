#include "agency/algorithms.h"
#include "agency/agencytypes.h"

#include <algorithm>

namespace agency {

QString tourDisplayName(const Tour* tour)
{
    if (!tour) {
        return QStringLiteral("?");
    }
    const QString explicitTitle = tour->title.trimmed();
    if (!explicitTitle.isEmpty()) {
        return explicitTitle;
    }

    // ASCII-only fallback title to avoid any UI encoding artifacts.
    static const QString firstWords[] = {
        QStringLiteral("Azure"),  QStringLiteral("Solar"),  QStringLiteral("Velvet"),
        QStringLiteral("Nordic"), QStringLiteral("Golden"), QStringLiteral("Crystal"),
        QStringLiteral("Ocean"),  QStringLiteral("Summit"), QStringLiteral("Urban"),
        QStringLiteral("Breeze"), QStringLiteral("Lunar"),  QStringLiteral("Coral"),
        QStringLiteral("Skyline"), QStringLiteral("Silent"), QStringLiteral("Wild"),
    };
    static const QString secondWords[] = {
        QStringLiteral("Voyage"),   QStringLiteral("Route"),   QStringLiteral("Escape"),
        QStringLiteral("Horizon"),  QStringLiteral("Motion"),  QStringLiteral("Season"),
        QStringLiteral("Rhythm"),   QStringLiteral("Panorama"), QStringLiteral("Pulse"),
        QStringLiteral("Oasis"),    QStringLiteral("Harbor"),  QStringLiteral("Summit"),
        QStringLiteral("Wave"),     QStringLiteral("Path"),    QStringLiteral("Trail"),
        QStringLiteral("Resonance"), QStringLiteral("Formula"), QStringLiteral("Atlas"),
    };

    const int firstCount = static_cast<int>(sizeof(firstWords) / sizeof(firstWords[0]));
    const int secondCount = static_cast<int>(sizeof(secondWords) / sizeof(secondWords[0]));

    const int seed = std::max(0, tour->id) * 11 + std::max(1, tour->days) * 3
        + std::max(1, tour->hotelStars) * 5;
    const int firstIndex = seed % firstCount;
    const int secondIndex = (seed * 7 + tour->id * 3) % secondCount;

    return QStringLiteral("%1 %2").arg(firstWords[firstIndex], secondWords[secondIndex]);
}

QString hashPassword(const QString& login, const QString& password)
{
    // Р”РµС‚РµСЂРјРёРЅРёСЂРѕРІР°РЅРЅС‹Р№ С…РµС€ login+password.
    // РСЃРїРѕР»СЊР·СѓРµС‚СЃСЏ РєР°Рє РїСЂРё СЂРµРіРёСЃС‚СЂР°С†РёРё, С‚Р°Рє Рё РїСЂРё Р»РѕРіРёРЅРµ.
    const QString combined = login + password;
    quint32 h = 5381U;
    constexpr quint32 kPrime = 131U;
    const QChar* p = combined.constData();
    const int len = combined.size();
    for (int i = 0; i < len; ++i) {
        h = (h * kPrime) ^ static_cast<quint32>(p[i].unicode());
    }
    return QString::number(h);
}

static bool matchCountry(const QString& tourCountry, const QString& filter)
{
    if (filter.trimmed().isEmpty()) {
        return true;
    }
    return tourCountry.contains(filter, Qt::CaseInsensitive);
}

QVector<Tour*> filterTours(const Tour* head,
                           const QString& countryFilter,
                           int minPrice,
                           int maxPrice,
                           int minDays,
                           int maxDays)
{
    QVector<Tour*> out;
    // РћРґРёРЅ РїСЂРѕС…РѕРґ РїРѕ СЃРІСЏР·РЅРѕРјСѓ СЃРїРёСЃРєСѓ С‚СѓСЂРѕРІ СЃ РЅРµР·Р°РІРёСЃРёРјС‹РјРё С„РёР»СЊС‚СЂР°РјРё.
    for (const Tour* t = head; t; t = t->next) {
        if (!matchCountry(t->country, countryFilter)) {
            continue;
        }
        if (minPrice > 0 && t->price < minPrice) {
            continue;
        }
        if (maxPrice > 0 && t->price > maxPrice) {
            continue;
        }
        if (minDays > 0 && t->days < minDays) {
            continue;
        }
        if (maxDays > 0 && t->days > maxDays) {
            continue;
        }
        out.push_back(const_cast<Tour*>(t));
    }
    return out;
}

QVector<Tour*> toursListToVector(const Tour* head)
{
    QVector<Tour*> out;
    for (const Tour* t = head; t; t = t->next) {
        out.push_back(const_cast<Tour*>(t));
    }
    return out;
}

static int compareTours(Tour* a, Tour* b, TourSortKey key)
{
    if (key == TourSortKey::ByPrice) {
        return a->price - b->price;
    }
    return a->days - b->days;
}

static int cmpOriented(Tour* a, Tour* b, TourSortKey key, bool ascending)
{
    const int d = compareTours(a, b, key);
    return ascending ? d : -d;
}

static void quickSortImpl(QVector<Tour*>& a, int lo, int hi, TourSortKey key, bool ascending)
{
    if (lo >= hi) {
        return;
    }
    // РљР»Р°СЃСЃРёС‡РµСЃРєРѕРµ СЂР°Р·Р±РёРµРЅРёРµ РҐРѕР°СЂР° РїРѕ РѕРїРѕСЂРЅРѕРјСѓ СЌР»РµРјРµРЅС‚Сѓ РёР· СЃРµСЂРµРґРёРЅС‹.
    Tour* pivot = a[(lo + hi) / 2];
    int i = lo;
    int j = hi;
    while (i <= j) {
        while (cmpOriented(a[i], pivot, key, ascending) < 0) {
            ++i;
        }
        while (cmpOriented(a[j], pivot, key, ascending) > 0) {
            --j;
        }
        if (i <= j) {
            std::swap(a[i], a[j]);
            ++i;
            --j;
        }
    }
    if (lo < j) {
        quickSortImpl(a, lo, j, key, ascending);
    }
    if (i < hi) {
        quickSortImpl(a, i, hi, key, ascending);
    }
}

void quickSortTours(QVector<Tour*>& items, TourSortKey key, bool ascending)
{
    if (items.size() <= 1) {
        return;
    }
    quickSortImpl(items, 0, items.size() - 1, key, ascending);
}

static int cmpCol(Tour* a, Tour* b, int col)
{
    switch (col) {
    case TourTableColumns::Id:
        return a->id - b->id;
    case TourTableColumns::Country:
        return QString::localeAwareCompare(a->country, b->country);
    case TourTableColumns::Title:
        return QString::localeAwareCompare(tourDisplayName(a), tourDisplayName(b));
    case TourTableColumns::Price:
        return a->price - b->price;
    case TourTableColumns::Days:
        return a->days - b->days;
    case TourTableColumns::Hotel:
        return QString::localeAwareCompare(a->hotel, b->hotel);
    case TourTableColumns::Address:
        return QString::localeAwareCompare(a->hotelAddress, b->hotelAddress);
    case TourTableColumns::Stars:
        return a->hotelStars - b->hotelStars;
    case TourTableColumns::DateStart:
        return QString::compare(a->dateStart, b->dateStart, Qt::CaseInsensitive);
    case TourTableColumns::DateEnd:
        return QString::compare(a->dateEnd, b->dateEnd, Qt::CaseInsensitive);
    default:
        return 0;
    }
}

void sortToursByColumnOrder(QVector<Tour*>& items, const QVector<std::pair<int, bool>>& columnOrderAsc)
{
    if (items.size() <= 1 || columnOrderAsc.isEmpty()) {
        return;
    }
    // stable_sort СЃРѕС…СЂР°РЅСЏРµС‚ РїРѕСЂСЏРґРѕРє РїРѕ РїСЂРµРґС‹РґСѓС‰РёРј РєР»СЋС‡Р°Рј
    // РїСЂРё СЂР°РІРµРЅСЃС‚РІРµ С‚РµРєСѓС‰РµРіРѕ СЃСЂР°РІРЅРµРЅРёСЏ.
    std::stable_sort(items.begin(), items.end(), [&](Tour* a, Tour* b) {
        for (const auto& e : columnOrderAsc) {
            const int col = e.first;
            const bool asc = e.second;
            const int d = cmpCol(a, b, col);
            if (d == 0) {
                continue;
            }
            return asc ? (d < 0) : (d > 0);
        }
        return false;
    });
}

} // namespace agency


