#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class RegisterDialog;
}
QT_END_NAMESPACE

class RegisterDialog : public QDialog {
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget* parent = nullptr);
    ~RegisterDialog() override;

private slots:
    void onAccepted();

private:
    std::unique_ptr<Ui::RegisterDialog> ui;
};

#endif
