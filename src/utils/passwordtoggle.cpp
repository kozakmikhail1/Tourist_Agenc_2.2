#include "utils/passwordtoggle.h"

#include <QAction>
#include <QIcon>
#include <QLineEdit>
#include <QStyle>

void attachPasswordVisibilityToggle(QLineEdit* edit)
{
    if (!edit) {
        return;
    }
    QStyle* st = edit->style();
    const QIcon iconHidden = QIcon::fromTheme(QStringLiteral("visibility-off"),
                                              st->standardIcon(QStyle::SP_FileDialogContentsView));
    const QIcon iconShown = QIcon::fromTheme(QStringLiteral("visibility"),
                                            st->standardIcon(QStyle::SP_FileDialogDetailedView));

    auto* action = new QAction(edit);
    action->setCheckable(true);
    action->setIcon(iconHidden);
    action->setToolTip(QStringLiteral("Показать пароль"));

    QObject::connect(action, &QAction::toggled, edit, [edit, action, iconHidden, iconShown](bool visible) {
        edit->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
        action->setIcon(visible ? iconShown : iconHidden);
        action->setToolTip(visible ? QStringLiteral("Скрыть пароль") : QStringLiteral("Показать пароль"));
    });

    edit->addAction(action, QLineEdit::TrailingPosition);
}

