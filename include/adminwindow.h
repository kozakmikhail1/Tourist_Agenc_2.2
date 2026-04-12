#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <memory>

#include "agency/agencytypes.h"
#include "agency/algorithms.h"
#include "utils/tourtablesortcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class AdminWindow;
}
QT_END_NAMESPACE

class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(QWidget* parent = nullptr);
    ~AdminWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void refreshUsers();
    void onFilterTours();
    void onAddUser();
    void onEditUser();
    void onDeleteUser();
    void onUserBookings();
    void onAddTour();
    void onEditTour();
    void onDeleteTour();
    void onTourBookings();
    void onLogout();

private:
    void refillToursTable();
    void applySortToAdminTours();

    std::unique_ptr<Ui::AdminWindow> ui;

    QVector<agency::Tour*> adminTourView_;
    TourTableSortController tourSort_{agency::TourTableColumns::Count};
};

#endif
