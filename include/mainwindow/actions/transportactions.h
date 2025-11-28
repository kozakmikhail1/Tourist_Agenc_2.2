#ifndef TRANSPORTACTIONS_H
#define TRANSPORTACTIONS_H

#include "mainwindow/actions/action.h"
#include "models/transportcompany.h"
#include "containers/datacontainer.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

// Класс-оператор для добавления транспортной компании
class AddTransportCompanyAction : public Action {
    Q_OBJECT

public:
    AddTransportCompanyAction(DataContainer<TransportCompany>* companies, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Добавить транспортную компанию"; }

private:
    DataContainer<TransportCompany>* companies_;
    QTableWidget* table_;
    QWidget* parent_;
};

// Класс-оператор для редактирования транспортной компании
class EditTransportCompanyAction : public Action {
    Q_OBJECT

public:
    EditTransportCompanyAction(DataContainer<TransportCompany>* companies, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Редактировать транспортную компанию"; }

private:
    DataContainer<TransportCompany>* companies_;
    QTableWidget* table_;
    QWidget* parent_;
};

// Класс-оператор для удаления транспортной компании
class DeleteTransportCompanyAction : public Action {
    Q_OBJECT

public:
    DeleteTransportCompanyAction(DataContainer<TransportCompany>* companies, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Удалить транспортную компанию"; }

private:
    DataContainer<TransportCompany>* companies_;
    QTableWidget* table_;
    QWidget* parent_;
};

// Класс-оператор для показа информации о транспортной компании
class ShowTransportCompanyInfoAction : public Action {
    Q_OBJECT

public:
    ShowTransportCompanyInfoAction(DataContainer<TransportCompany>* companies, QTableWidget* table, QWidget* parent);
    void execute() override;
    QString description() const override { return "Показать информацию о транспортной компании"; }

private:
    DataContainer<TransportCompany>* companies_;
    QTableWidget* table_;
    QWidget* parent_;
};

// Класс-оператор для обновления таблицы транспортных компаний
class RefreshTransportCompaniesAction : public Action {
    Q_OBJECT

public:
    RefreshTransportCompaniesAction(DataContainer<TransportCompany>* companies, QTableWidget* table);
    void execute() override;
    QString description() const override { return "Обновить список транспортных компаний"; }

private:
    DataContainer<TransportCompany>* companies_;
    QTableWidget* table_;
};

#endif // TRANSPORTACTIONS_H


