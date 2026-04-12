#include "utils/toursortheaderview.h"

#include <QMouseEvent>

TourSortHeaderView::TourSortHeaderView(Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent)
{
}

void TourSortHeaderView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        const int logical = logicalIndexAt(event->pos());
        if (logical >= 0) {
            // Передаем и колонку, и модификаторы клавиатуры для логики мультисортировки.
            emit sortSectionClicked(logical, event->modifiers());
        }
    }
    QHeaderView::mouseReleaseEvent(event);
}

