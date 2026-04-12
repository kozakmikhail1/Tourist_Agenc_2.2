#ifndef USERWINDOW_H
#define USERWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <memory>

#include "agency/agencytypes.h"
#include "agency/algorithms.h"
#include "utils/tourtablesortcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class UserWindow;
}
QT_END_NAMESPACE

class UserWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit UserWindow(QWidget* parent = nullptr);
    ~UserWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void refreshTours();
    void onFilter();
    void onBook();
    void onProfile();
    void onBookings();
    void onLogout();

private:
    void refillTourTable();
    void applySortToCurrentTours();

    std::unique_ptr<Ui::UserWindow> ui;

    QVector<agency::Tour*> currentTourPointers_;
    TourTableSortController tourSort_{agency::TourTableColumns::Count};
};

#endif
