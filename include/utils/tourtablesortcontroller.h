#ifndef TOURTABLESORTCONTROLLER_H
#define TOURTABLESORTCONTROLLER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <Qt>
#include <utility>

class QTableWidget;

/**
 * Многоуровневая сортировка:
 * - обычный клик — один ключ (или цикл Asc/Desc/сброс по тому же столбцу);
 * - Shift или Ctrl + клик — добавить столбец как следующий ключ (цикл Asc/Desc/убрать из цепочки).
 */
class TourTableSortController {
public:
    explicit TourTableSortController(int columnCount);

    void onHeaderClicked(int section, Qt::KeyboardModifiers modifiers);

    /** Пусто — без сортировки (порядок как после фильтра). Порядок вектора = приоритет: 0 — главный ключ. */
    QVector<std::pair<int, bool>> sortOrder() const;

    void applyToTableHeader(QTableWidget* table) const;

    QString statusHint() const;

    static const QStringList& baseLabels();

private:
    enum class Phase { None, Asc, Desc };

    struct ActiveKey {
        int col = 0;
        Phase phase = Phase::Asc;
    };

    static Phase nextPhase(Phase p);
    static QString phaseArrow(Phase p);
    static QString superscriptRank(int n);

    static int indexInChain(const QVector<ActiveKey>& chain, int col);

    int columnCount_;
    QVector<ActiveKey> chain_;
};

#endif
