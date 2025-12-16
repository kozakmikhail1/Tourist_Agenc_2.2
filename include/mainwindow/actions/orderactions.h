#ifndef ORDERACTIONS_H
#define ORDERACTIONS_H

#include "mainwindow/actions/action.h"
#include "models/order.h"
#include "containers/datacontainer.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class AddOrderAction : public Action {
    Q_OBJECT

public:
    AddOrderAction(DataContainer<Order>* orders,
                   DataContainer<Tour>* tours,
                   QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Добавить заказ"; }

private:
    DataContainer<Order>* orders_;
    DataContainer<Tour>* tours_;
    QTableWidget* table_;
    QWidget* parent_;
};

class EditOrderAction : public Action {
    Q_OBJECT

public:
    EditOrderAction(DataContainer<Order>* orders,
                   DataContainer<Tour>* tours,
                   QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Редактировать заказ"; }

private:
    DataContainer<Order>* orders_;
    DataContainer<Tour>* tours_;
    QTableWidget* table_;
    QWidget* parent_;
};

class ProcessOrderAction : public Action {
    Q_OBJECT

public:
    ProcessOrderAction(DataContainer<Order>* orders, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Обработать заказ"; }

private:
    DataContainer<Order>* orders_;
    QTableWidget* table_;
    QWidget* parent_;
};

class DeleteOrderAction : public Action {
    Q_OBJECT

public:
    DeleteOrderAction(DataContainer<Order>* orders, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Удалить заказ"; }

private:
    DataContainer<Order>* orders_;
    QTableWidget* table_;
    QWidget* parent_;
};

class ShowOrderInfoAction : public Action {
    Q_OBJECT

public:
    ShowOrderInfoAction(DataContainer<Order>* orders, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Показать информацию о заказе"; }

private:
    DataContainer<Order>* orders_;
    QTableWidget* table_;
    QWidget* parent_;
};

class RefreshOrdersAction : public Action {
    Q_OBJECT

public:
    RefreshOrdersAction(DataContainer<Order>* orders, QTableWidget* table);
    void execute() override;
    QString description() const override { return "Обновить список заказов"; }

private:
    DataContainer<Order>* orders_;
    QTableWidget* table_;
};

#endif


