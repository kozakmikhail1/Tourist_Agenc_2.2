#ifndef HOTELACTIONS_H
#define HOTELACTIONS_H

#include "mainwindow/actions/action.h"
#include "models/hotel.h"
#include "containers/datacontainer.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class AddHotelAction : public Action {
    Q_OBJECT

public:
    AddHotelAction(DataContainer<Hotel>* hotels, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Добавить отель"; }

private:
    DataContainer<Hotel>* hotels_;
    QTableWidget* table_;
    QWidget* parent_;
};

class EditHotelAction : public Action {
    Q_OBJECT

public:
    EditHotelAction(DataContainer<Hotel>* hotels, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Редактировать отель"; }

private:
    DataContainer<Hotel>* hotels_;
    QTableWidget* table_;
    QWidget* parent_;
};

class DeleteHotelAction : public Action {
    Q_OBJECT

public:
    DeleteHotelAction(DataContainer<Hotel>* hotels, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Удалить отель"; }

private:
    DataContainer<Hotel>* hotels_;
    QTableWidget* table_;
    QWidget* parent_;
};

class ShowHotelInfoAction : public Action {
    Q_OBJECT

public:
    ShowHotelInfoAction(DataContainer<Hotel>* hotels, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Показать информацию об отеле"; }

private:
    DataContainer<Hotel>* hotels_;
    QTableWidget* table_;
    QWidget* parent_;
};

class RefreshHotelsAction : public Action {
    Q_OBJECT

public:
    RefreshHotelsAction(DataContainer<Hotel>* hotels, QTableWidget* table);
    void execute() override;
    QString description() const override { return "Обновить список отелей"; }

private:
    DataContainer<Hotel>* hotels_;
    QTableWidget* table_;
};

#endif


