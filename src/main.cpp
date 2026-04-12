#include "dialogs/logindialog.h"
#include "agency/agencydata.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TouristAgency"));
    QApplication::setOrganizationName(QStringLiteral("CourseWork"));

    agency::AgencyData& data = agency::AgencyData::instance();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        agency::AgencyData& d = agency::AgencyData::instance();
        if (!d.saveToFiles()) {
            QMessageBox::warning(nullptr, QStringLiteral("Сохранение при выходе"),
                                 QStringLiteral("Не удалось записать файлы data:\n%1")
                                     .arg(d.lastLoadMessages().join(QLatin1Char('\n'))));
        }
    });

    data.loadFromFiles();
    const QStringList loadMsgs = data.lastLoadMessages();
    if (!loadMsgs.isEmpty()) {
        QMessageBox::information(nullptr, QStringLiteral("Загрузка данных"),
                                 loadMsgs.join(QLatin1Char('\n')));
    }

    auto* login = new LoginDialog();
    login->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(login, &QDialog::finished, [](int code) {
        if (code == QDialog::Rejected) {
            QApplication::quit();
        }
    });
    login->show();

    return app.exec();
}

