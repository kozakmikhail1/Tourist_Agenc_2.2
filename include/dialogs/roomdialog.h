#ifndef ROOMDIALOG_H
#define ROOMDIALOG_H

#include <QDialog>
#include <memory>
#include "models/room.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RoomDialog; }
QT_END_NAMESPACE

// Forward declaration для включения в .cpp
// Полное определение Ui::RoomDialog находится в ui_roomdialog.h

class RoomDialog : public QDialog {
    Q_OBJECT

public:
    explicit RoomDialog(QWidget *parent = nullptr, Room* room = nullptr);
    ~RoomDialog();
    
    Room getRoom() const;

private slots:
    void accept() override;

private:
    std::unique_ptr<Ui::RoomDialog> ui;
    Room* room_;
};

#endif // ROOMDIALOG_H










