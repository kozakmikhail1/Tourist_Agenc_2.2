#include "dialogs/logindialog.h"
#include "ui_logindialog.h"
#include "dialogs/registerdialog.h"
#include "userwindow.h"
#include "adminwindow.h"
#include "agency/agencydata.h"
#include "agency/validation.h"
#include "utils/passwordtoggle.h"

#include <QApplication>
#include <QStyle>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , ui(std::make_unique<Ui::LoginDialog>())
{
    ui->setupUi(this);
    ui->loginButton->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));
    ui->registerButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->quitButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));

    connect(ui->loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(ui->registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(ui->quitButton, &QPushButton::clicked, this, &QDialog::reject);

    attachPasswordVisibilityToggle(ui->passwordEdit);
}

LoginDialog::~LoginDialog() = default;

void LoginDialog::onLoginClicked()
{
    agency::AgencyData& data = agency::AgencyData::instance();
    const QString login = ui->loginEdit->text().trimmed();
    const QString pass = ui->passwordEdit->text();

    const agency::ValidationResult lv = agency::validateLogin(login);
    if (!lv.ok) {
        ui->statusLabel->setText(lv.errorMessage);
        return;
    }
    const agency::ValidationResult pv = agency::validatePasswordLoginAttempt(pass);
    if (!pv.ok) {
        ui->statusLabel->setText(pv.errorMessage);
        return;
    }

    // Проверяем пару логин/пароль уже после базовой валидации полей.
    if (!data.tryLogin(login, pass)) {
        ui->statusLabel->setText(QStringLiteral("Неверный логин или пароль."));
        return;
    }

    ui->statusLabel->setText(QStringLiteral("Успешный вход."));
    hide();

    agency::User* u = data.currentUser();
    if (!u) {
        return;
    }
    // После успешного входа открываем окно по роли текущего пользователя.
    if (u->isAdmin) {
        auto* w = new AdminWindow();
        w->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(w, &QObject::destroyed, qApp, [this]() {
            agency::AgencyData::instance().setCurrentUser(nullptr);
            ui->loginEdit->clear();
            ui->passwordEdit->clear();
            ui->statusLabel->clear();
            show();
        });
        w->show();
    } else {
        auto* w = new UserWindow();
        w->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(w, &QObject::destroyed, qApp, [this]() {
            agency::AgencyData::instance().setCurrentUser(nullptr);
            ui->loginEdit->clear();
            ui->passwordEdit->clear();
            ui->statusLabel->clear();
            show();
        });
        w->show();
    }
}

void LoginDialog::onRegisterClicked()
{
    // После закрытия окна регистрации лишь показываем статус в текущем диалоге.
    RegisterDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }
    ui->statusLabel->setText(QStringLiteral("Регистрация выполнена. Войдите."));
}


