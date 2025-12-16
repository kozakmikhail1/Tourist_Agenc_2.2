#ifndef COMPANYDIALOG_H
#define COMPANYDIALOG_H

#include <QDialog>
#include <memory>
#include "models/transportcompany.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CompanyDialog; }
QT_END_NAMESPACE

class CompanyDialog : public QDialog {
    Q_OBJECT

public:
    explicit CompanyDialog(QWidget *parent = nullptr, TransportCompany* company = nullptr);
    ~CompanyDialog();
    
    TransportCompany getCompany() const;

private slots:
    void accept() override;
    void addSchedule();
    void editSchedule();
    void deleteSchedule();

private:
    std::unique_ptr<Ui::CompanyDialog> ui;
    TransportCompany* company_;
    QVector<TransportSchedule> schedules_;
    
    void updateSchedulesTable();
};

#endif



