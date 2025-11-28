#include "dialogs/tourdialog.h"
#include "ui_tourdialog.h"
#include <QMessageBox>
#include <QDate>
#include <QSet>
#include <QVariant>

TourDialog::TourDialog(QWidget *parent,
                       DataContainer<Country>* countries,
                       DataContainer<Hotel>* hotels,
                       DataContainer<TransportCompany>* companies,
                       Tour* tour)
    : QDialog(parent)
    , ui(std::make_unique<Ui::TourDialog>())
    , countries_(countries)
    , hotels_(hotels)
    , companies_(companies)
    , tour_(tour)
    , tourSetupHelper_(std::make_unique<TourSetupHelper>(countries, hotels, companies, nullptr))
{
    ui->setupUi(this);
    
    // СНАЧАЛА подключаем сигналы, чтобы они срабатывали при заполнении комбо-боксов
    connect(ui->countryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TourDialog::onCountryChanged);
    connect(ui->hotelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TourDialog::onHotelChanged);
    connect(ui->roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { calculateCost(); });
    connect(ui->transportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TourDialog::onTransportChanged);
    connect(ui->scheduleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { calculateCost(); });
    connect(ui->startDateEdit, &QDateEdit::dateChanged, this, [this]() {
        if (ui->startDateEdit->date() > ui->endDateEdit->date()) {
            ui->endDateEdit->setDate(ui->startDateEdit->date().addDays(1));
        }
        calculateCost();
    });
    connect(ui->endDateEdit, &QDateEdit::dateChanged, this, [this]() {
        calculateCost();
    });
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TourDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &TourDialog::reject);
    
    // Теперь заполняем комбо-боксы
    if (countries_) {
        for (const auto& country : countries_->getData()) {
            ui->countryCombo->addItem(country.getName());
        }
    }
    
    updateHotelsCombo();
    updateTransportCombo();
    updateSchedulesCombo();
    
    // Устанавливаем даты по умолчанию
    ui->startDateEdit->setDate(QDate::currentDate());
    ui->endDateEdit->setDate(QDate::currentDate().addDays(7));
    
    if (tour_) {
        ui->nameEdit->setText(tour_->getName());
        int index = ui->countryCombo->findText(tour_->getCountry());
        if (index >= 0) ui->countryCombo->setCurrentIndex(index);
        
        ui->startDateEdit->setDate(tour_->getStartDate());
        ui->endDateEdit->setDate(tour_->getEndDate());
        
        updateHotelsCombo();
        index = ui->hotelCombo->findText(tour_->getHotel().getName());
        if (index >= 0) {
            ui->hotelCombo->setCurrentIndex(index);
            // Явно вызываем updateRoomsCombo() после программной установки отеля
            updateRoomsCombo();
        }
        
        updateTransportCombo();
        index = ui->transportCombo->findText(tour_->getTransportCompany().getName());
        if (index >= 0) {
            ui->transportCombo->setCurrentIndex(index);
            updateSchedulesCombo();
        }
    }
    calculateCost();
}


TransportCompany* TourDialog::findSelectedTransportCompany() const {
    if (!companies_ || ui->transportCombo->currentIndex() < 0) {
        return nullptr;
    }
    
    QString selectedCountry = ui->countryCombo->currentText();
    QSet<QString> citiesInCountry = tourSetupHelper_->collectCitiesInCountry(selectedCountry);
    QString capital = tourSetupHelper_->findCountryCapital(selectedCountry);
    
    if (!capital.isEmpty()) {
        citiesInCountry.insert(capital);
    }
    
    int comboIndex = 0;
    for (int i = 0; i < companies_->size(); ++i) {
        TransportCompany* comp = companies_->get(i);
        if (!comp) {
            continue;
        }
        
        bool hasRelevantSchedule = citiesInCountry.isEmpty();
        if (!hasRelevantSchedule) {
            for (const auto& schedule : comp->getSchedules()) {
                QString arrivalCity = schedule.arrivalCity;
                if (citiesInCountry.contains(arrivalCity) || 
                    (!capital.isEmpty() && arrivalCity.contains(capital, Qt::CaseInsensitive))) {
                    hasRelevantSchedule = true;
                    break;
                }
            }
        }
        
        if (!hasRelevantSchedule) {
            continue;
        }
        
        if (comboIndex == ui->transportCombo->currentIndex()) {
            return comp;
        }
        comboIndex++;
    }
    return nullptr;
}

double TourDialog::calculateTransportCost() const {
    if (!companies_ || ui->transportCombo->currentIndex() < 0 || 
        ui->scheduleCombo->currentIndex() < 0) {
        return 0.0;
    }
    
    TransportCompany* company = findSelectedTransportCompany();
    if (!company) {
        return 0.0;
    }
    
    QVariant scheduleData = ui->scheduleCombo->itemData(ui->scheduleCombo->currentIndex());
    if (!scheduleData.isValid() || !scheduleData.canConvert<int>()) {
        return 0.0;
    }
    
    int scheduleIndex = scheduleData.toInt();
    if (scheduleIndex < 0 || scheduleIndex >= company->getScheduleCount()) {
        return 0.0;
    }
    
    TransportSchedule* schedule = company->getSchedule(scheduleIndex);
    return schedule ? schedule->price : 0.0;
}

double TourDialog::calculateHotelCost() const {
    if (!hotels_ || ui->hotelCombo->currentIndex() < 0 || 
        ui->roomCombo->currentIndex() < 0) {
        return 0.0;
    }
    
    QString selectedCountry = ui->countryCombo->currentText();
    int hotelIndex = 0;
    
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() != selectedCountry) {
            continue;
        }
        
        if (hotelIndex != ui->hotelCombo->currentIndex()) {
            hotelIndex++;
            continue;
        }
        
        QVariant roomData = ui->roomCombo->itemData(ui->roomCombo->currentIndex());
        if (!roomData.isValid() || !roomData.canConvert<int>()) {
            break;
        }
        
        int roomIndex = roomData.toInt();
        if (roomIndex < 0 || roomIndex >= hotel.getRoomCount()) {
            break;
        }
        
        const Room* room = hotel.getRoom(roomIndex);
        if (!room) {
            break;
        }
        
        Tour tempTour = getTour();
        int nights = tempTour.getDuration();
        if (nights > 0) {
            return room->getPricePerNight() * nights;
        }
        break;
    }
    
    return 0.0;
}

void TourDialog::calculateCost() {
    Tour tempTour = getTour();
    double totalCost = tempTour.calculateCost();
    double transportCost = calculateTransportCost();
    double hotelCost = calculateHotelCost();
    int nights = tempTour.getDuration();
    
    // Форматируем вывод стоимости с разбивкой
    QString costText = QString("<b style='font-size: 16pt; color: #0066cc;'>%1 руб</b>")
        .arg(totalCost, 0, 'f', 2);
    
    if (transportCost > 0 || hotelCost > 0) {
        costText += QString("<br><span style='font-size: 10pt; color: #666;'>");
        if (transportCost > 0) {
            costText += QString("Транспорт: %1 руб<br>")
                .arg(transportCost, 0, 'f', 2);
        }
        if (hotelCost > 0) {
            costText += QString("Отель (%1 ночей): %2 руб")
                .arg(nights).arg(hotelCost, 0, 'f', 2);
        }
        costText += "</span>";
    }
    
    ui->costValueLabel->setText(costText);
}


Hotel TourDialog::getSelectedHotel(const QString& country) const {
    if (!hotels_ || ui->hotelCombo->currentIndex() < 0) {
        return Hotel();
    }
    
    int hotelIndex = 0;
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() != country) {
            continue;
        }
        
        if (hotelIndex != ui->hotelCombo->currentIndex()) {
            hotelIndex++;
            continue;
        }
        
        Hotel hotelCopy = hotel;
        // Очищаем номера и добавляем только выбранный
        while (hotelCopy.getRoomCount() > 0) {
            hotelCopy.removeRoom(0);
        }
        
        if (ui->roomCombo->currentIndex() < 0) {
            return hotelCopy;
        }
        
        QVariant roomData = ui->roomCombo->itemData(ui->roomCombo->currentIndex());
        if (!roomData.isValid() || !roomData.canConvert<int>()) {
            return hotelCopy;
        }
        
        int roomIndex = roomData.toInt();
        if (roomIndex < 0 || roomIndex >= hotel.getRoomCount()) {
            return hotelCopy;
        }
        
        const Room* selectedRoom = hotel.getRoom(roomIndex);
        if (selectedRoom) {
            hotelCopy.addRoom(*selectedRoom);
        }
        return hotelCopy;
    }
    
    return Hotel();
}

void TourDialog::setupTourTransport(Tour& tour, const QString& country) const {
    if (!companies_ || ui->transportCombo->currentIndex() < 0) {
        return;
    }
    
    TransportCompany* company = findSelectedTransportCompany();
    if (!company) {
        return;
    }
    
    tour.setTransportCompany(*company);
    
    if (ui->scheduleCombo->currentIndex() < 0) {
        return;
    }
    
    QVariant scheduleData = ui->scheduleCombo->itemData(ui->scheduleCombo->currentIndex());
    if (!scheduleData.isValid() || !scheduleData.canConvert<int>()) {
        return;
    }
    
    int scheduleIndex = scheduleData.toInt();
    if (scheduleIndex < 0 || scheduleIndex >= company->getScheduleCount()) {
        return;
    }
    
    TransportSchedule* schedule = company->getSchedule(scheduleIndex);
    if (schedule) {
        tour.setTransportSchedule(*schedule);
    }
}

TourDialog::~TourDialog() = default;

Tour TourDialog::getTour() const {
    Tour tour;
    tour.setName(ui->nameEdit->text());
    tour.setCountry(ui->countryCombo->currentText());
    tour.setStartDate(ui->startDateEdit->date());
    tour.setEndDate(ui->endDateEdit->date());
    
    // Находим отель и выбранный номер
    QString selectedCountry = ui->countryCombo->currentText();
    Hotel hotel = getSelectedHotel(selectedCountry);
    if (!hotel.getName().isEmpty()) {
        tour.setHotel(hotel);
    }
    
    // Находим транспортную компанию и выбранный рейс
    setupTourTransport(tour, selectedCountry);
    
    return tour;
}

void TourDialog::accept() {
    if (ui->nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название тура");
        return;
    }
    if (ui->countryCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите страну");
        return;
    }
    if (ui->hotelCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите отель");
        return;
    }
    if (ui->roomCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите номер в отеле");
        return;
    }
    if (ui->transportCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите транспортную компанию");
        return;
    }
    if (ui->scheduleCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите рейс");
        return;
    }
    if (ui->startDateEdit->date() >= ui->endDateEdit->date()) {
        QMessageBox::warning(this, "Ошибка", "Дата окончания должна быть позже даты начала");
        return;
    }
    QDialog::accept();
}

void TourDialog::onCountryChanged() {
    updateTransportCombo();
    updateHotelsCombo();  // updateHotelsCombo() уже очищает и обновляет roomCombo
    // Обновляем список расписаний, если транспорт уже выбран
    if (ui->transportCombo->currentIndex() >= 0) {
        updateSchedulesCombo();
    }
    calculateCost();
}

void TourDialog::onHotelChanged() {
    // Всегда обновляем список номеров при изменении отеля
    // Этот метод вызывается при изменении индекса в комбо-боксе отелей
    updateRoomsCombo();
    calculateCost();
}

void TourDialog::onTransportChanged() {
    updateSchedulesCombo();
    calculateCost();
}

void TourDialog::updateHotelsCombo() {
    // Используем ту же простую логику, что и в BookTourDialog (которая работает)
    ui->hotelCombo->clear();
    ui->roomCombo->clear();
    
    if (!hotels_ || ui->countryCombo->currentIndex() < 0) return;
    
    QString selectedCountry = ui->countryCombo->currentText();
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() == selectedCountry) {
            ui->hotelCombo->addItem(hotel.getName());
        }
    }
    
    // После заполнения списка отелей, если список не пуст, выбираем первый отель
    // и обновляем список номеров (как это работает в BookTourDialog)
    if (ui->hotelCombo->count() > 0) {
        // Блокируем сигналы при программной установке, чтобы избежать двойного вызова
        ui->hotelCombo->blockSignals(true);
        ui->hotelCombo->setCurrentIndex(0);
        ui->hotelCombo->blockSignals(false);
        // Обновляем список номеров для первого отеля
        updateRoomsCombo();
    }
}

void TourDialog::updateRoomsCombo() {
    ui->roomCombo->clear();
    
    // Используем ту же простую логику, что и в BookTourDialog (которая работает)
    if (!hotels_ || ui->hotelCombo->currentIndex() < 0) return;
    
    QString selectedCountry = ui->countryCombo->currentText();
    int hotelIndex = 0;
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() == selectedCountry) {
            if (hotelIndex == ui->hotelCombo->currentIndex()) {
                for (int i = 0; i < hotel.getRoomCount(); ++i) {
                    const Room* room = hotel.getRoom(i);
                    if (room) {
                        QString roomInfo = QString("%1 (%2, %3 руб/ночь)")
                            .arg(room->getName())
                            .arg(Room::roomTypeToString(room->getRoomType()))
                            .arg(room->getPricePerNight(), 0, 'f', 2);
                        ui->roomCombo->addItem(roomInfo, i);
                    }
                }
                break;
            }
            hotelIndex++;
        }
    }
}

bool TourDialog::hasRelevantScheduleForCountry(const TransportCompany& company, 
                                                const QSet<QString>& citiesInCountry, 
                                                const QString& capital) const {
    if (citiesInCountry.isEmpty()) {
        return true;
    }
    
    for (const auto& schedule : company.getSchedules()) {
        QString arrivalCity = schedule.arrivalCity;
        if (citiesInCountry.contains(arrivalCity) || 
            (!capital.isEmpty() && arrivalCity.contains(capital, Qt::CaseInsensitive))) {
            return true;
        }
    }
    return false;
}

void TourDialog::updateTransportCombo() {
    ui->transportCombo->clear();
    
    if (!companies_) {
        return;
    }
    
    // Если страна не выбрана, показываем все компании
    if (!countries_ || ui->countryCombo->currentIndex() < 0) {
        for (const auto& company : companies_->getData()) {
            ui->transportCombo->addItem(company.getName());
        }
        return;
    }
    
    QString selectedCountry = ui->countryCombo->currentText();
    QString capital = tourSetupHelper_->findCountryCapital(selectedCountry);
    QSet<QString> citiesInCountry = tourSetupHelper_->collectCitiesInCountry(selectedCountry);
    
    if (!capital.isEmpty()) {
        citiesInCountry.insert(capital);
    }
    
    // Показываем только транспортные компании, которые имеют рейсы в выбранную страну
    for (const auto& company : companies_->getData()) {
        if (hasRelevantScheduleForCountry(company, citiesInCountry, capital)) {
            ui->transportCombo->addItem(company.getName());
        }
    }
}

void TourDialog::populateScheduleCombo(TransportCompany* company, 
                                       const QSet<QString>& citiesInCountry, 
                                       const QString& capital) const {
    if (!company) {
        return;
    }
    
    for (int i = 0; i < company->getScheduleCount(); ++i) {
        TransportSchedule* schedule = company->getSchedule(i);
        if (!schedule) {
            continue;
        }
        
        // Если страна выбрана, фильтруем рейсы по городам назначения
        bool shouldInclude = citiesInCountry.isEmpty();
        if (!shouldInclude) {
            QString arrivalCity = schedule->arrivalCity;
            shouldInclude = citiesInCountry.contains(arrivalCity) || 
                           (!capital.isEmpty() && arrivalCity.contains(capital, Qt::CaseInsensitive));
        }
        
        if (!shouldInclude) {
            continue;
        }
        
        QString scheduleInfo = QString("%1 → %2, %3, %4 руб")
            .arg(schedule->departureCity)
            .arg(schedule->arrivalCity)
            .arg(schedule->departureDate.toString("dd.MM.yyyy"))
            .arg(schedule->price, 0, 'f', 2);
        ui->scheduleCombo->addItem(scheduleInfo, i);
    }
}

void TourDialog::updateSchedulesCombo() {
    ui->scheduleCombo->clear();
    
    if (!companies_ || ui->transportCombo->currentIndex() < 0) {
        return;
    }
    
    QString selectedCountry = ui->countryCombo->currentText();
    QSet<QString> citiesInCountry;
    QString capital = "";
    
    if (countries_ && ui->countryCombo->currentIndex() >= 0) {
        capital = tourSetupHelper_->findCountryCapital(selectedCountry);
        citiesInCountry = tourSetupHelper_->collectCitiesInCountry(selectedCountry);
        
        if (!capital.isEmpty()) {
            citiesInCountry.insert(capital);
        }
    }
    
    // Находим выбранную транспортную компанию
    TransportCompany* company = findSelectedTransportCompany();
    populateScheduleCombo(company, citiesInCountry, capital);
}

