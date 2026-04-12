#ifndef HOTELDIALOG_H
#define HOTELDIALOG_H

#include <QDialog>
#include <memory>
#include "models/hotel.h"
#include "models/room.h"
#include "containers/datacontainer.h"
#include "models/country.h"

QT_BEGIN_NAMESPACE
namespace Ui { class HotelDialog; }
QT_END_NAMESPACE

class HotelDialog : public QDialog {
    Q_OBJECT

public:
    explicit HotelDialog(QWidget *parent = nullptr, 
                        DataContainer<Country>* countries = nullptr,
                        Hotel* hotel = nullptr);
    ~HotelDialog();
    
    Hotel getHotel() const;

private slots:
    void accept() override;
    void addRoom();
    void editRoom();
    void deleteRoom();

private:
    std::unique_ptr<Ui::HotelDialog> ui;
    DataContainer<Country>* countries_;
    Hotel* hotel_;
    QVector<Room> rooms_;
    
    void updateRoomsTable();
};

#endif



