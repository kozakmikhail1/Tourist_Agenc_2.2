#ifndef TOURTABLEDISPLAY_H
#define TOURTABLEDISPLAY_H

class QTableWidget;

namespace tour_widgets {

/** Узкие столбцы: дни, звёзды, цена, даты; широкие: отель и адрес (Stretch). */
void applyTourTableColumnLayout(QTableWidget* table, bool hideIdColumn);

} // namespace tour_widgets

#endif
