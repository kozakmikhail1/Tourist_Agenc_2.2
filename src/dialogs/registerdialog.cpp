#include "dialogs/registerdialog.h"
#include "ui_registerdialog.h"
#include "agency/agencydata.h"
#include "agency/validation.h"
#include "utils/passwordtoggle.h"

#include <QMessageBox>

RegisterDialog::RegisterDialog(QWidget* parent)
    : QDialog(parent)
    , ui(std::make_unique<Ui::RegisterDialog>())
{
    ui->setupUi(this);
    attachPasswordVisibilityToggle(ui->passwordEdit);
    attachPasswordVisibilityToggle(ui->password2Edit);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &RegisterDialog::onAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

RegisterDialog::~RegisterDialog() = default;

void RegisterDialog::onAccepted()
{
    const QString login = ui->loginEdit->text().trimmed();
    const QString p1 = ui->passwordEdit->text();
    const QString p2 = ui->password2Edit->text();

    const agency::ValidationResult lv = agency::validateLogin(login);
    if (!lv.ok) {
        QMessageBox::warning(this, QStringLiteral("Регистрация"), lv.errorMessage);
        return;
    }
    const agency::ValidationResult pv = agency::validatePassword(p1);
    if (!pv.ok) {
        QMessageBox::warning(this, QStringLiteral("Регистрация"), pv.errorMessage);
        return;
    }
    if (p1 != p2) {
        QMessageBox::warning(this, QStringLiteral("Регистрация"), QStringLiteral("Пароли не совпадают."));
        return;
    }

    agency::AgencyData& data = agency::AgencyData::instance();
    if (!data.registerUser(login, p1, false)) {
        QMessageBox::warning(this, QStringLiteral("Регистрация"),
                             QStringLiteral("Этот логин уже занят."));
        return;
    }
    if (!data.saveToFiles()) {
        QMessageBox::warning(this, QStringLiteral("Регистрация"),
                             data.lastLoadMessages().join(QLatin1Char('\n')));
        return;
    }
    accept();
}

