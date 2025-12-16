#ifndef COUNTRYDIALOG_H
#define COUNTRYDIALOG_H

#include <QDialog>
#include <memory>
#include "models/country.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CountryDialog; }
QT_END_NAMESPACE

class CountryDialog : public QDialog {
    Q_OBJECT

public:
    explicit CountryDialog(QWidget *parent = nullptr, Country* country = nullptr);
    ~CountryDialog();
    
    Country getCountry() const;

private slots:
    void accept() override;

private:
    std::unique_ptr<Ui::CountryDialog> ui;
    Country* country_;
};

#endif



