#include "dialogs/roomdialog.h"
#include "ui_roomdialog.h"
#include <QMessageBox>

RoomDialog::RoomDialog(QWidget *parent, Room* room)
    : QDialog(parent)
    , ui(std::make_unique<Ui::RoomDialog>())
    , room_(room)
{
    ui->setupUi(this);
    
    // Заполняем комбо-бокс типов номеров
    ui->typeCombo->addItems({"Single", "Double", "Suite", "Apartment"});
    
    if (room_) {
        ui->nameEdit->setText(room_->getName());
        ui->typeCombo->setCurrentText(Room::roomTypeToString(room_->getRoomType()));
        ui->priceSpin->setValue(room_->getPricePerNight());
        ui->capacitySpin->setValue(room_->getCapacity());
    }
    
    // Подключаем кнопки диалога
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &RoomDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &RoomDialog::reject);
}

RoomDialog::~RoomDialog() = default;

Room RoomDialog::getRoom() const {
    Room room(
        ui->nameEdit->text(),
        Room::stringToRoomType(ui->typeCombo->currentText()),
        ui->priceSpin->value(),
        ui->capacitySpin->value()
    );
    return room;
}

void RoomDialog::accept() {
    if (ui->nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название номера");
        return;
    }
    QDialog::accept();
}









