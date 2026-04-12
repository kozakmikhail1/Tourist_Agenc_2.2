#ifndef TOURSORTHEADERVIEW_H
#define TOURSORTHEADERVIEW_H

#include <QHeaderView>

class QMouseEvent;

class TourSortHeaderView : public QHeaderView {
    Q_OBJECT

public:
    explicit TourSortHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

signals:
    /** Логический столбец и модификаторы на момент отпускания кнопки мыши (надёжнее, чем QGuiApplication::keyboardModifiers). */
    void sortSectionClicked(int logicalIndex, Qt::KeyboardModifiers modifiers);

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
};

#endif
