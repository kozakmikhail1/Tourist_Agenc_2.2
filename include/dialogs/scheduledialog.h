#ifndef SCHEDULEDIALOG_H
#define SCHEDULEDIALOG_H

#include <QDialog>
#include <memory>
#include "models/transportcompany.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ScheduleDialog; }
QT_END_NAMESPACE

class ScheduleDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScheduleDialog(QWidget *parent = nullptr, TransportSchedule* schedule = nullptr);
    ~ScheduleDialog();
    
    TransportSchedule getSchedule() const;

private slots:
    void accept() override;

private:
    std::unique_ptr<Ui::ScheduleDialog> ui;
    TransportSchedule* schedule_;
};

#endif









