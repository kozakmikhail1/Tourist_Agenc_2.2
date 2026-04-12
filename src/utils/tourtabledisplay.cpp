#include "utils/tourtabledisplay.h"

#include "agency/algorithms.h"

#include <QHeaderView>
#include <QTableWidget>

void tour_widgets::applyTourTableColumnLayout(QTableWidget* table, bool hideIdColumn)
{
    if (!table) {
        return;
    }
    // Централизованная раскладка ширин колонок, чтобы таблицы выглядели одинаково в разных окнах.
    QHeaderView* h = table->horizontalHeader();
    h->setStretchLastSection(false);

    table->setColumnHidden(agency::TourTableColumns::Id, hideIdColumn);

    using H = QHeaderView;
    if (!hideIdColumn) {
        h->setSectionResizeMode(agency::TourTableColumns::Id, H::Fixed);
        h->resizeSection(agency::TourTableColumns::Id, 52);
    }
    h->setSectionResizeMode(agency::TourTableColumns::Country, H::Interactive);
    h->resizeSection(agency::TourTableColumns::Country, 120);
    h->setSectionResizeMode(agency::TourTableColumns::Title, H::Interactive);
    h->resizeSection(agency::TourTableColumns::Title, 220);
    h->setSectionResizeMode(agency::TourTableColumns::Price, H::Fixed);
    h->resizeSection(agency::TourTableColumns::Price, 90);
    h->setSectionResizeMode(agency::TourTableColumns::Days, H::Fixed);
    h->resizeSection(agency::TourTableColumns::Days, 40);
    h->setSectionResizeMode(agency::TourTableColumns::Hotel, H::Stretch);
    h->setSectionResizeMode(agency::TourTableColumns::Address, H::Stretch);
    h->setSectionResizeMode(agency::TourTableColumns::Stars, H::Fixed);
    h->resizeSection(agency::TourTableColumns::Stars, 52);
    h->setSectionResizeMode(agency::TourTableColumns::DateStart, H::Fixed);
    h->resizeSection(agency::TourTableColumns::DateStart, 98);
    h->setSectionResizeMode(agency::TourTableColumns::DateEnd, H::Fixed);
    h->resizeSection(agency::TourTableColumns::DateEnd, 98);
}

