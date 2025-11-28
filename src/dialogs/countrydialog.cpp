#include "dialogs/countrydialog.h"
#include "ui_countrydialog.h"
#include <QMessageBox>

CountryDialog::CountryDialog(QWidget *parent, Country* country)
    : QDialog(parent)
    , ui(std::make_unique<Ui::CountryDialog>())
    , country_(country)
{
    ui->setupUi(this);
    
    if (country_) {
        ui->nameEdit->setText(country_->getName());
        ui->continentEdit->setText(country_->getContinent());
        ui->capitalEdit->setText(country_->getCapital());
        ui->currencyEdit->setText(country_->getCurrency());
    }
    
    // Подключаем кнопки диалога
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CountryDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &CountryDialog::reject);
}

CountryDialog::~CountryDialog() = default;

Country CountryDialog::getCountry() const {
    Country country;
    country.setName(ui->nameEdit->text());
    country.setContinent(ui->continentEdit->text());
    country.setCapital(ui->capitalEdit->text());
    country.setCurrency(ui->currencyEdit->text());
    return country;
}

void CountryDialog::accept() {
    if (ui->nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название страны");
        return;
    }
    QDialog::accept();
}

