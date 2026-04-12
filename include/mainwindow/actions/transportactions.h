#ifndef TRANSPORTACTIONS_H
#define TRANSPORTACTIONS_H

#include "mainwindow/actions/action.h"
#include "models/transportcompany.h"
#include "containers/datacontainer.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

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

#endif


