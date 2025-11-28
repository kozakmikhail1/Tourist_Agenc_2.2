#include "dialogs/hoteldialog.h"
#include "ui_hoteldialog.h"
#include "dialogs/roomdialog.h"
#include <QMessageBox>
#include <QInputDialog>

HotelDialog::HotelDialog(QWidget *parent, DataContainer<Country>* countries, Hotel* hotel)
    : QDialog(parent)
    , ui(std::make_unique<Ui::HotelDialog>())
    , countries_(countries)
    , hotel_(hotel)
{
    ui->setupUi(this);
    
    // Заполняем комбо-бокс стран
    if (countries_) {
        for (const auto& country : countries_->getData()) {
            ui->countryCombo->addItem(country.getName());
        }
    }
    
    if (hotel_) {
        ui->nameEdit->setText(hotel_->getName());
        ui->starsSpin->setValue(hotel_->getStars());
        ui->addressEdit->setText(hotel_->getAddress());
        
        int index = ui->countryCombo->findText(hotel_->getCountry());
        if (index >= 0) ui->countryCombo->setCurrentIndex(index);
        
        rooms_ = hotel_->getRooms();
        updateRoomsTable();
    }
    
    connect(ui->addRoomButton, &QPushButton::clicked, this, &HotelDialog::addRoom);
    connect(ui->editRoomButton, &QPushButton::clicked, this, &HotelDialog::editRoom);
    connect(ui->deleteRoomButton, &QPushButton::clicked, this, &HotelDialog::deleteRoom);
    
    // Подключаем кнопки диалога
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &HotelDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &HotelDialog::reject);
}

HotelDialog::~HotelDialog() = default;

Hotel HotelDialog::getHotel() const {
    Hotel hotel;
    hotel.setName(ui->nameEdit->text());
    hotel.setCountry(ui->countryCombo->currentText());
    hotel.setStars(ui->starsSpin->value());
    hotel.setAddress(ui->addressEdit->text());
    
    for (const auto& room : rooms_) {
        hotel.addRoom(room);
    }
    
    return hotel;
}

void HotelDialog::accept() {
    if (ui->nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название отеля");
        return;
    }
    if (rooms_.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Добавьте хотя бы один номер");
        return;
    }
    QDialog::accept();
}

void HotelDialog::addRoom() {
    RoomDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Room room = dialog.getRoom();
        rooms_.append(room);
        updateRoomsTable();
    }
}

void HotelDialog::editRoom() {
    int row = ui->roomsTable->currentRow();
    if (row < 0 || row >= rooms_.size()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите номер для редактирования");
        return;
    }
    
    Room& room = rooms_[row];
    RoomDialog dialog(this, &room);
    if (dialog.exec() == QDialog::Accepted) {
        room = dialog.getRoom();
        updateRoomsTable();
    }
}

void HotelDialog::deleteRoom() {
    int row = ui->roomsTable->currentRow();
    if (row < 0 || row >= rooms_.size()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите номер для удаления");
        return;
    }
    
    rooms_.removeAt(row);
    updateRoomsTable();
}

void HotelDialog::updateRoomsTable() {
    ui->roomsTable->setRowCount(rooms_.size());
    
    int row = 0;
    for (const auto& room : rooms_) {
        ui->roomsTable->setItem(row, 0, new QTableWidgetItem(room.getName()));
        ui->roomsTable->setItem(row, 1, 
            new QTableWidgetItem(Room::roomTypeToString(room.getRoomType())));
        ui->roomsTable->setItem(row, 2, 
            new QTableWidgetItem(QString::number(room.getPricePerNight(), 'f', 2)));
        ui->roomsTable->setItem(row, 3, 
            new QTableWidgetItem(QString::number(room.getCapacity())));
        ++row;
    }
}

