#ifndef AGENCY_ALGORITHMS_H
#define AGENCY_ALGORITHMS_H

#include <QString>
#include <QVector>
#include <utility>

namespace agency {

struct Tour;

QString hashPassword(const QString& login, const QString& password);
/** Человекочитаемое название тура для UI (например: "Turkey - Hotel Marina"). */
QString tourDisplayName(const Tour* tour);

/** Фильтр: страна (подстрока), цена [minPrice..maxPrice], дни [minDays..maxDays]; 0 = граница не задана. */
QVector<Tour*> filterTours(const Tour* head,
                           const QString& countryFilter,
                           int minPrice,
                           int maxPrice,
                           int minDays,
                           int maxDays);

enum class TourSortKey { ByPrice, ByDays };

void quickSortTours(QVector<Tour*>& items, TourSortKey key, bool ascending);

QVector<Tour*> toursListToVector(const Tour* head);

/** Индексы столбцов таблицы туров (0…8). */
namespace TourTableColumns {
enum Index {
    Id = 0,
    Title,
    Country,
    Price,
    Days,
    Hotel,
    Address,
    Stars,
    DateStart,
    DateEnd,
    Count
};
}

/** Многоуровневая сортировка: пары (индекс столбца, по возрастанию). */
void sortToursByColumnOrder(QVector<Tour*>& items, const QVector<std::pair<int, bool>>& columnOrderAsc);

} // namespace agency

#endif
