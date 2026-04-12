#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class LoginDialog;
}
QT_END_NAMESPACE

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog() override;

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    std::unique_ptr<Ui::LoginDialog> ui;
};

#endif
