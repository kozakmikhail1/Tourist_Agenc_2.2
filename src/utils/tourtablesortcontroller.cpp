#include "utils/tourtablesortcontroller.h"

#include <algorithm>

#include <QTableWidget>
#include <QTableWidgetItem>

namespace {

const QStringList kBaseLabels = {
    QStringLiteral("ID"),
    QStringLiteral("Название тура"),
    QStringLiteral("Страна"),
    QStringLiteral("Цена"),
    QStringLiteral("Дни"),
    QStringLiteral("Отель"),
    QStringLiteral("Адрес отеля"),
    QStringLiteral("Звёзды"),
    QStringLiteral("Начало"),
    QStringLiteral("Конец"),
};

} // namespace

TourTableSortController::TourTableSortController(int columnCount)
    : columnCount_(columnCount)
{
}

int TourTableSortController::indexInChain(const QVector<ActiveKey>& chain, int col)
{
    for (int i = 0; i < chain.size(); ++i) {
        if (chain[i].col == col) {
            return i;
        }
    }
    return -1;
}

const QStringList& TourTableSortController::baseLabels()
{
    return kBaseLabels;
}

TourTableSortController::Phase TourTableSortController::nextPhase(Phase p)
{
    switch (p) {
    case Phase::None:
        return Phase::Asc;
    case Phase::Asc:
        return Phase::Desc;
    case Phase::Desc:
        return Phase::None;
    }
    return Phase::None;
}

QString TourTableSortController::phaseArrow(Phase p)
{
    switch (p) {
    case Phase::Asc:
        return QStringLiteral(" \u2191");
    case Phase::Desc:
        return QStringLiteral(" \u2193");
    default:
        return QString();
    }
}

QString TourTableSortController::superscriptRank(int n)
{
    static const QString digits[] = {
        QStringLiteral("\u2070"), QStringLiteral("\u00b9"), QStringLiteral("\u00b2"),
        QStringLiteral("\u00b3"), QStringLiteral("\u2074"), QStringLiteral("\u2075"),
        QStringLiteral("\u2076"), QStringLiteral("\u2077"), QStringLiteral("\u2078"),
        QStringLiteral("\u2079"),
    };
    if (n >= 0 && n <= 9) {
        return digits[n];
    }
    return QStringLiteral("(%1)").arg(n);
}

void TourTableSortController::onHeaderClicked(int section, Qt::KeyboardModifiers modifiers)
{
    if (section < 0 || section >= columnCount_) {
        return;
    }

    // Обычный клик сортирует по одному столбцу.
    // Shift/Ctrl + клик добавляет или изменяет ключ в цепочке мультисортировки.
    const bool addKey = modifiers.testFlag(Qt::ShiftModifier) || modifiers.testFlag(Qt::ControlModifier);

    if (!addKey) {
        if (chain_.size() == 1 && chain_[0].col == section) {
            const Phase nx = nextPhase(chain_[0].phase);
            if (nx == Phase::None) {
                chain_.clear();
            } else {
                chain_[0].phase = nx;
            }
        } else {
            chain_.clear();
            chain_.push_back({section, Phase::Asc});
        }
        return;
    }

    const int pos = indexInChain(chain_, section);
    if (pos < 0) {
        chain_.push_back({section, Phase::Asc});
        return;
    }

    const Phase nx = nextPhase(chain_[pos].phase);
    if (nx == Phase::None) {
        chain_.removeAt(pos);
    } else {
        chain_[pos].phase = nx;
    }
}

QVector<std::pair<int, bool>> TourTableSortController::sortOrder() const
{
    QVector<std::pair<int, bool>> out;
    out.reserve(chain_.size());
    for (const ActiveKey& k : chain_) {
        out.push_back({k.col, k.phase == Phase::Asc});
    }
    return out;
}

void TourTableSortController::applyToTableHeader(QTableWidget* table) const
{
    if (!table) {
        return;
    }
    // В заголовке показываем направление и, при необходимости, приоритет ключа.
    const int n = std::min(columnCount_, static_cast<int>(kBaseLabels.size()));
    const bool showRank = chain_.size() > 1;
    for (int c = 0; c < n; ++c) {
        QString text = kBaseLabels.at(c);
        const int idx = indexInChain(chain_, c);
        if (idx >= 0) {
            if (showRank) {
                text += superscriptRank(idx + 1);
            }
            text += phaseArrow(chain_[idx].phase);
        }
        table->setHorizontalHeaderItem(c, new QTableWidgetItem(text));
    }
}

QString TourTableSortController::statusHint() const
{
    // Текст для status bar, чтобы пользователь видел текущие правила сортировки.
    const QVector<std::pair<int, bool>> ord = sortOrder();
    if (ord.isEmpty()) {
        return QStringLiteral("Сортировка: клик по заголовку; Shift или Ctrl + клик — второй ключ.");
    }
    QStringList parts;
    int r = 1;
    for (const auto& e : ord) {
        const int col = e.first;
        const bool asc = e.second;
        if (col < 0 || col >= kBaseLabels.size()) {
            continue;
        }
        parts.append(QStringLiteral("%1 %2 %3")
                           .arg(kBaseLabels.at(col))
                           .arg(asc ? QStringLiteral("\u2191") : QStringLiteral("\u2193"))
                           .arg(ord.size() > 1 ? QStringLiteral("(%1)").arg(r++) : QString()));
    }
    return QStringLiteral("Сортировка: %1").arg(parts.join(QStringLiteral(", ")));
}


