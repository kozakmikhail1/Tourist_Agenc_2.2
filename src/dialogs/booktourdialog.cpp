#include "dialogs/booktourdialog.h"
#include "ui_booktourdialog.h"
#include "dialogs/booktourcostcalculator.h"
#include "dialogs/toursetuphelper.h"
#include <QMessageBox>
#include <QDate>
#include <QSet>

BookTourDialog::BookTourDialog(QWidget *parent,
                               DataContainer<Country>* countries,
                               DataContainer<Hotel>* hotels,
                               DataContainer<TransportCompany>* companies,
                               DataContainer<Tour>* tours)
    : QDialog(parent)
    , ui(std::make_unique<Ui::BookTourDialog>())
    , countries_(countries)
    , hotels_(hotels)
    , companies_(companies)
    , tours_(tours)
    , isEditMode_(false)
    , costCalculator_(nullptr)
    , tourSetupHelper_(std::make_unique<TourSetupHelper>(countries, hotels, companies, tours))
{
    ui->setupUi(this);
    
    // Инициализируем costCalculator после setupUi, так как нужны указатели на UI элементы
    BookTourUIElements uiElements = {
        ui->countryCombo, ui->transportCombo, ui->scheduleCombo,
        ui->hotelCombo, ui->roomCombo, ui->startDateEdit, ui->endDateEdit
    };
    costCalculator_ = std::make_unique<BookTourCostCalculator>(hotels, companies, uiElements);
    setWindowTitle("Бронирование тура");
    
    // Заполняем страны
    if (countries_) {
        for (const auto& country : countries_->getData()) {
            ui->countryCombo->addItem(country.getName());
        }
    }
    
    // Заполняем туры
    updateToursCombo();
    
    // Устанавливаем даты по умолчанию
    ui->startDateEdit->setDate(QDate::currentDate());
    ui->endDateEdit->setDate(QDate::currentDate().addDays(7));
    
    // Подключаем сигналы режимов
    connect(ui->selectTourRadio, &QRadioButton::toggled, this, &BookTourDialog::onModeChanged);
    connect(ui->createTourRadio, &QRadioButton::toggled, this, &BookTourDialog::onModeChanged);
    
    // Подключаем сигналы для создания тура
    connect(ui->countryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookTourDialog::onCountryChanged);
    connect(ui->transportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookTourDialog::onTransportChanged);
    connect(ui->scheduleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookTourDialog::calculateCost);
    connect(ui->hotelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookTourDialog::onHotelChanged);
    connect(ui->roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookTourDialog::calculateCost);
    connect(ui->startDateEdit, &QDateEdit::dateChanged, this, [this]() {
        if (ui->startDateEdit->date() > ui->endDateEdit->date()) {
            ui->endDateEdit->setDate(ui->startDateEdit->date().addDays(1));
        }
        onDatesChanged();
    });
    connect(ui->endDateEdit, &QDateEdit::dateChanged, this, &BookTourDialog::onDatesChanged);
    
    // Подключаем сигналы для выбора тура
    connect(ui->tourCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookTourDialog::calculateCost);
    
    // Подключаем кнопки диалога
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &BookTourDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &BookTourDialog::reject);
    
    // Инициализация
    updateTransportCombo();
    updateHotelsCombo();
    updateUIForMode();
    calculateCost();
}

BookTourDialog::~BookTourDialog() = default;

void BookTourDialog::onCountryChanged() {
    updateTransportCombo();
    updateHotelsCombo();
    // Обновляем список расписаний, если транспорт уже выбран
    if (ui->transportCombo->currentIndex() >= 0) {
        onTransportChanged();
    }
    calculateCost();
}

QString BookTourDialog::findCountryCapital(const QString& selectedCountry) const {
    if (!countries_) {
        return "";
    }
    
    for (const auto& country : countries_->getData()) {
        if (country.getName() == selectedCountry) {
            return country.getCapital();
        }
    }
    return "";
}

QSet<QString> BookTourDialog::collectCitiesInCountry(const QString& selectedCountry) const {
    QSet<QString> cities;
    
    if (!hotels_) {
        return cities;
    }
    
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() != selectedCountry) {
            continue;
        }
        
        QString address = hotel.getAddress();
        if (address.isEmpty()) {
            continue;
        }
        
        QString city = address.split(',').first().trimmed();
        if (!city.isEmpty()) {
            cities.insert(city);
        }
        cities.insert(address);
    }
    
    return cities;
}

TransportCompany* BookTourDialog::findSelectedTransportCompany() const {
    if (!companies_ || ui->transportCombo->currentIndex() < 0) {
        return nullptr;
    }
    
    int comboIndex = 0;
    for (int i = 0; i < companies_->size(); ++i) {
        TransportCompany* comp = companies_->get(i);
        if (!comp || comp->getScheduleCount() == 0) {
            continue;
        }
        
        if (comboIndex == ui->transportCombo->currentIndex()) {
            return comp;
        }
        comboIndex++;
    }
    return nullptr;
}

void BookTourDialog::populateScheduleCombo(TransportCompany* company, const QSet<QString>& citiesInCountry, const QString& capital) const {
    if (!company) {
        return;
    }
    
    for (int i = 0; i < company->getScheduleCount(); ++i) {
        TransportSchedule* schedule = company->getSchedule(i);
        if (!schedule) {
            continue;
        }
        
        // Если страна выбрана, фильтруем рейсы по городам назначения
        bool shouldInclude = true;
        if (!citiesInCountry.isEmpty()) {
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

void BookTourDialog::onTransportChanged() {
    ui->scheduleCombo->clear();
    
    if (!companies_ || ui->transportCombo->currentIndex() < 0) {
        calculateCost();
        return;
    }
    
    // Получаем список городов выбранной страны для фильтрации рейсов
    QSet<QString> citiesInCountry;
    QString capital = "";
    
    if (countries_ && ui->countryCombo->currentIndex() >= 0) {
        QString selectedCountry = ui->countryCombo->currentText();
        capital = findCountryCapital(selectedCountry);
        citiesInCountry = collectCitiesInCountry(selectedCountry);
        
        if (!capital.isEmpty()) {
            citiesInCountry.insert(capital);
        }
    }
    
    // Находим выбранную транспортную компанию и заполняем расписание
    TransportCompany* company = findSelectedTransportCompany();
    populateScheduleCombo(company, citiesInCountry, capital);
    
    calculateCost();
}

void BookTourDialog::onHotelChanged() {
    updateRoomsCombo();
    calculateCost();
}

void BookTourDialog::onDatesChanged() {
    calculateCost();
}

void BookTourDialog::updateToursCombo() {
    ui->tourCombo->clear();
    
    if (!tours_) return;
    
    for (const auto& tour : tours_->getData()) {
        QString tourInfo = QString("%1 (%2, %3 - %4, %5 руб)")
            .arg(tour.getName())
            .arg(tour.getCountry())
            .arg(tour.getStartDate().toString("dd.MM.yyyy"))
            .arg(tour.getEndDate().toString("dd.MM.yyyy"))
            .arg(tour.calculateCost(), 0, 'f', 2);
        ui->tourCombo->addItem(tourInfo);
    }
}

void BookTourDialog::onModeChanged() {
    updateUIForMode();
    calculateCost();
}

void BookTourDialog::updateUIForMode() {
    bool isCreateMode = ui->createTourRadio->isChecked();
    
    // Показываем/скрываем виджеты в зависимости от режима
    ui->createTourWidget->setVisible(isCreateMode);
    ui->selectTourWidget->setVisible(!isCreateMode);
}

void BookTourDialog::updateTransportCombo() {
    ui->transportCombo->clear();
    ui->scheduleCombo->clear();
    
    if (!companies_) return;
    
    // Если страна не выбрана, показываем все компании с рейсами
    if (!countries_ || ui->countryCombo->currentIndex() < 0) {
        for (const auto& company : companies_->getData()) {
            if (company.getScheduleCount() > 0) {
                ui->transportCombo->addItem(company.getName());
            }
        }
        return;
    }
    
    // Получаем выбранную страну и её столицу
    QString selectedCountry = ui->countryCombo->currentText();
    QString capital = findCountryCapital(selectedCountry);
    QSet<QString> citiesInCountry = collectCitiesInCountry(selectedCountry);
    
    // Добавляем столицу в список городов
    if (!capital.isEmpty()) {
        citiesInCountry.insert(capital);
    }
    
    // Показываем только транспортные компании, которые имеют рейсы в выбранную страну
    for (const auto& company : companies_->getData()) {
        if (company.getScheduleCount() > 0) {
            // Проверяем, есть ли у компании рейсы в города выбранной страны
            bool hasRelevantSchedule = false;
            for (const auto& schedule : company.getSchedules()) {
                // Проверяем, идет ли рейс в столицу или в город из списка отелей
                QString arrivalCity = schedule.arrivalCity;
                if (citiesInCountry.contains(arrivalCity) || 
                    (!capital.isEmpty() && arrivalCity.contains(capital, Qt::CaseInsensitive))) {
                    hasRelevantSchedule = true;
                    break;
                }
            }
            
            if (hasRelevantSchedule) {
                ui->transportCombo->addItem(company.getName());
            }
        }
    }
}

void BookTourDialog::updateHotelsCombo() {
    ui->hotelCombo->clear();
    ui->roomCombo->clear();
    
    if (!hotels_ || ui->countryCombo->currentIndex() < 0) return;
    
    QString selectedCountry = ui->countryCombo->currentText();
    for (const auto& hotel : hotels_->getData()) {
        if (hotel.getCountry() == selectedCountry) {
            QString hotelInfo = QString("%1 (%2 звезд)")
                .arg(hotel.getName())
                .arg(hotel.getStars());
            ui->hotelCombo->addItem(hotelInfo);
        }
    }
}

void BookTourDialog::updateRoomsCombo() {
    ui->roomCombo->clear();
    
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


void BookTourDialog::calculateCost() {
    if (ui->selectTourRadio->isChecked()) {
        calculateCostForSelectMode();
    } else {
        calculateCostForCreateMode();
    }
}

void BookTourDialog::calculateCostForSelectMode() {
    double totalCost = 0.0;
    
    if (tours_ && ui->tourCombo->currentIndex() >= 0) {
        Tour* tour = tours_->get(ui->tourCombo->currentIndex());
        if (tour) {
            totalCost = tour->calculateCost();
        }
    }
    
    QString costText = totalCost > 0 
        ? QString("<b style='font-size: 14pt; color: #2196F3;'>%1 руб</b>")
            .arg(totalCost, 0, 'f', 2)
        : "<span style='color: #999;'>0.00 руб</span>";
    
    ui->costValueLabelSelect->setText(costText);
}

void BookTourDialog::calculateCostForCreateMode() {
    double totalCost = costCalculator_->calculateTotalCost();
    double transportCost = costCalculator_->calculateTransportCost();
    double hotelCost = costCalculator_->calculateHotelCost();
    
    // Форматируем вывод стоимости
    QString costText = QString("<b style='font-size: 14pt; color: #2196F3;'>%1 руб</b>")
        .arg(totalCost, 0, 'f', 2);
    
    if (transportCost > 0 || hotelCost > 0) {
        costText += QString("<br><span style='font-size: 10pt; color: #666;'>");
        if (transportCost > 0) {
            costText += QString("Транспорт: %1 руб<br>")
                .arg(transportCost, 0, 'f', 2);
        }
        if (hotelCost > 0) {
            int nights = costCalculator_->calculateNights();
            costText += QString("Отель (%1 ночей, с учетом звезд): %2 руб<br>")
                .arg(nights).arg(hotelCost, 0, 'f', 2);
        }
        
        QString country = ui->countryCombo->currentText();
        double countryMultiplier = costCalculator_->getCountryMultiplier(country);
        if (countryMultiplier != 1.0) {
            costText += QString("Коэффициент страны: x%1")
                .arg(countryMultiplier, 0, 'f', 2);
        }
        costText += "</span>";
    }
    
    ui->costValueLabel->setText(costText);
}

bool BookTourDialog::validateClientData(const QString& name, const QString& phone, const QString& email) const {
    if (name.isEmpty()) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Введите ваше имя");
        return false;
    }
    if (phone.isEmpty()) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Введите ваш телефон");
        return false;
    }
    if (email.isEmpty()) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Введите ваш email");
        return false;
    }
    return true;
}

bool BookTourDialog::validateSelectMode() const {
    if (ui->tourCombo->currentIndex() < 0) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Выберите тур");
        return false;
    }
    return validateClientData(ui->clientNameEditSelect->text(), 
                              ui->clientPhoneEditSelect->text(), 
                              ui->clientEmailEditSelect->text());
}

bool BookTourDialog::validateCreateMode() const {
    if (ui->countryCombo->currentIndex() < 0) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Выберите страну");
        return false;
    }
    if (ui->transportCombo->currentIndex() < 0) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Выберите транспортную компанию");
        return false;
    }
    if (ui->scheduleCombo->currentIndex() < 0) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Выберите рейс");
        return false;
    }
    if (ui->hotelCombo->currentIndex() < 0) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Выберите отель");
        return false;
    }
    if (ui->roomCombo->currentIndex() < 0) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Выберите номер в отеле");
        return false;
    }
    if (ui->startDateEdit->date() >= ui->endDateEdit->date()) {
        QMessageBox::warning(const_cast<BookTourDialog*>(this), "Ошибка", "Дата окончания должна быть позже даты начала");
        return false;
    }
    return validateClientData(ui->clientNameEdit->text(), 
                              ui->clientPhoneEdit->text(), 
                              ui->clientEmailEdit->text());
}

void BookTourDialog::accept() {
    bool isValid = false;
    if (ui->selectTourRadio->isChecked()) {
        isValid = validateSelectMode();
    } else {
        isValid = validateCreateMode();
    }
    
    if (isValid) {
        QDialog::accept();
    }
}

void BookTourDialog::setEditMode(bool editMode) {
    isEditMode_ = editMode;
    if (editMode) {
        setWindowTitle("Редактирование заказа");
    } else {
        setWindowTitle("Бронирование тура");
    }
}

bool BookTourDialog::findExistingTour(const Tour& tour, int& tourIndex) {
    return tourSetupHelper_->findExistingTour(tour, tourIndex);
}

void BookTourDialog::setupSelectMode(int tourIndex, const QString& clientName, 
                                      const QString& clientPhone, const QString& clientEmail) {
    ui->selectTourRadio->setChecked(true);
    ui->createTourRadio->setChecked(false);
    updateUIForMode();
    ui->tourCombo->setCurrentIndex(tourIndex);
    ui->clientNameEditSelect->setText(clientName);
    ui->clientPhoneEditSelect->setText(clientPhone);
    ui->clientEmailEditSelect->setText(clientEmail);
}

void BookTourDialog::setupCreateMode(const Tour& tour, const QString& clientName,
                                     const QString& clientPhone, const QString& clientEmail) {
    ui->createTourRadio->setChecked(true);
    ui->selectTourRadio->setChecked(false);
    updateUIForMode();
    
    // Временно блокируем все сигналы
    ui->countryCombo->blockSignals(true);
    ui->transportCombo->blockSignals(true);
    ui->hotelCombo->blockSignals(true);
    ui->scheduleCombo->blockSignals(true);
    ui->roomCombo->blockSignals(true);
    
    // Устанавливаем страну
    int countryIndex = ui->countryCombo->findText(tour.getCountry());
    if (countryIndex >= 0) {
        ui->countryCombo->setCurrentIndex(countryIndex);
    }
    
    // Обновляем комбо-боксы после выбора страны
    updateTransportCombo();
    updateHotelsCombo();
    
    // Устанавливаем даты
    ui->startDateEdit->setDate(tour.getStartDate());
    ui->endDateEdit->setDate(tour.getEndDate());
    
    // Устанавливаем транспорт и отель
    setupTransportAndSchedule(tour);
    setupHotelAndRoom(tour);
    
    // Разблокируем все сигналы
    ui->countryCombo->blockSignals(false);
    ui->transportCombo->blockSignals(false);
    ui->hotelCombo->blockSignals(false);
    ui->scheduleCombo->blockSignals(false);
    ui->roomCombo->blockSignals(false);
    
    // Устанавливаем данные клиента
    ui->clientNameEdit->setText(clientName);
    ui->clientPhoneEdit->setText(clientPhone);
    ui->clientEmailEdit->setText(clientEmail);
}

void BookTourDialog::setupTransportAndSchedule(const Tour& tour) {
    tourSetupHelper_->setupTransportAndSchedule(tour, ui->countryCombo, 
                                                 ui->transportCombo, ui->scheduleCombo);
    ui->transportCombo->blockSignals(false);
    onTransportChanged();
    ui->transportCombo->blockSignals(true);
}

void BookTourDialog::setupHotelAndRoom(const Tour& tour) {
    tourSetupHelper_->setupHotelAndRoom(tour, ui->countryCombo, 
                                        ui->hotelCombo, ui->roomCombo);
    ui->hotelCombo->blockSignals(false);
    onHotelChanged();
    ui->hotelCombo->blockSignals(true);
}

void BookTourDialog::setOrder(const Order& order) {
    setEditMode(true);
    
    const Tour tour = order.getTour();
    QString clientName = order.getClientName();
    QString clientPhone = order.getClientPhone();
    QString clientEmail = order.getClientEmail();
    
    // Пытаемся найти тур в списке готовых туров
    int tourIndex = -1;
    if (findExistingTour(tour, tourIndex)) {
        setupSelectMode(tourIndex, clientName, clientPhone, clientEmail);
    } else {
        setupCreateMode(tour, clientName, clientPhone, clientEmail);
    }
    
    // Пересчитываем стоимость
    calculateCost();
}

Tour BookTourDialog::getTourFromSelectMode() const {
    Tour tour;
    if (tours_ && ui->tourCombo->currentIndex() >= 0) {
        Tour* selectedTour = tours_->get(ui->tourCombo->currentIndex());
        if (selectedTour) {
            tour = *selectedTour;
        }
    }
    return tour;
}

Hotel BookTourDialog::getSelectedHotel(const QString& country) const {
    return tourSetupHelper_->getSelectedHotel(country, ui->countryCombo, 
                                             ui->hotelCombo, ui->roomCombo);
}

void BookTourDialog::setupTourTransport(Tour& tour) const {
    tourSetupHelper_->setupTourTransport(tour, ui->countryCombo, 
                                         ui->transportCombo, ui->scheduleCombo);
}

Tour BookTourDialog::getTourFromCreateMode() const {
    QString country = ui->countryCombo->currentText();
    QDate startDate = ui->startDateEdit->date();
    QDate endDate = ui->endDateEdit->date();
    
    QString tourName = QString("Тур в %1").arg(country);
    Tour tour(tourName, country, startDate, endDate);
    
    // Устанавливаем отель
    Hotel hotel = getSelectedHotel(country);
    if (!hotel.getName().isEmpty()) {
        tour.setHotel(hotel);
    }
    
    // Устанавливаем транспорт
    setupTourTransport(tour);
    
    return tour;
}

Order BookTourDialog::getOrder() const {
    Tour tour;
    QString clientName;
    QString clientPhone;
    QString clientEmail;
    
    if (ui->selectTourRadio->isChecked()) {
        tour = getTourFromSelectMode();
        clientName = ui->clientNameEditSelect->text();
        clientPhone = ui->clientPhoneEditSelect->text();
        clientEmail = ui->clientEmailEditSelect->text();
    } else {
        tour = getTourFromCreateMode();
        clientName = ui->clientNameEdit->text();
        clientPhone = ui->clientPhoneEdit->text();
        clientEmail = ui->clientEmailEdit->text();
    }
    
    return Order(tour, clientName, clientPhone, clientEmail);
}

