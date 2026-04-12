#ifndef TOURACTIONS_H
#define TOURACTIONS_H

#include "mainwindow/actions/action.h"
#include "models/tour.h"
#include "containers/datacontainer.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class AddTourAction : public Action {
    Q_OBJECT

public:
    AddTourAction(DataContainer<Tour>* tours, 
                  DataContainer<Country>* countries,
                  DataContainer<Hotel>* hotels,
                  DataContainer<TransportCompany>* companies,
                  QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Добавить тур"; }

private:
    DataContainer<Tour>* tours_;
    DataContainer<Country>* countries_;
    DataContainer<Hotel>* hotels_;
    DataContainer<TransportCompany>* companies_;
    QTableWidget* table_;
    QWidget* parent_;
};

class EditTourAction : public Action {
    Q_OBJECT

public:
    EditTourAction(DataContainer<Tour>* tours,
                  DataContainer<Country>* countries,
                  DataContainer<Hotel>* hotels,
                  DataContainer<TransportCompany>* companies,
                  QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Редактировать тур"; }

private:
    DataContainer<Tour>* tours_;
    DataContainer<Country>* countries_;
    DataContainer<Hotel>* hotels_;
    DataContainer<TransportCompany>* companies_;
    QTableWidget* table_;
    QWidget* parent_;
};

class DeleteTourAction : public Action {
    Q_OBJECT

public:
    DeleteTourAction(DataContainer<Tour>* tours, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Удалить тур"; }

private:
    DataContainer<Tour>* tours_;
    QTableWidget* table_;
    QWidget* parent_;
};

class ShowTourInfoAction : public Action {
    Q_OBJECT

public:
    ShowTourInfoAction(DataContainer<Tour>* tours, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Показать информацию о туре"; }

private:
    DataContainer<Tour>* tours_;
    QTableWidget* table_;
    QWidget* parent_;
};

class SearchToursAction : public Action {
    Q_OBJECT

public:
    SearchToursAction(DataContainer<Tour>* tours, QWidget* parent);
    void execute() override;
    QString description() const override { return "Поиск туров"; }

private:
    DataContainer<Tour>* tours_;
    QWidget* parent_;
};

class RefreshToursAction : public Action {
    Q_OBJECT

public:
    RefreshToursAction(DataContainer<Tour>* tours, QTableWidget* table);
    void execute() override;
    QString description() const override { return "Обновить список туров"; }

private:
    DataContainer<Tour>* tours_;
    QTableWidget* table_;
};

#endif


