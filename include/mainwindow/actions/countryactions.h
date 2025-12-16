#ifndef COUNTRYACTIONS_H
#define COUNTRYACTIONS_H

#include "mainwindow/actions/action.h"
#include "models/country.h"
#include "containers/datacontainer.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

class AddCountryAction : public Action {
    Q_OBJECT

public:
    AddCountryAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Добавить страну"; }

private:
    DataContainer<Country>* countries_;
    QTableWidget* table_;
    QWidget* parent_;
};

class EditCountryAction : public Action {
    Q_OBJECT

public:
    EditCountryAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Редактировать страну"; }

private:
    DataContainer<Country>* countries_;
    QTableWidget* table_;
    QWidget* parent_;
};

class DeleteCountryAction : public Action {
    Q_OBJECT

public:
    DeleteCountryAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Удалить страну"; }

private:
    DataContainer<Country>* countries_;
    QTableWidget* table_;
    QWidget* parent_;
};

class ShowCountryInfoAction : public Action {
    Q_OBJECT

public:
    ShowCountryInfoAction(DataContainer<Country>* countries, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Показать информацию о стране"; }

private:
    DataContainer<Country>* countries_;
    QTableWidget* table_;
    QWidget* parent_;
};

class RefreshCountriesAction : public Action {
    Q_OBJECT

public:
    RefreshCountriesAction();
    void execute() override;
    QString description() const override { return "Обновить список стран"; }
};

#endif


