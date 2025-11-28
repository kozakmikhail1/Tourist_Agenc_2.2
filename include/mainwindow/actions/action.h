#ifndef ACTION_H
#define ACTION_H

#include <QObject>
#include <QWidget>

// Базовый класс для всех действий (Command pattern)
class Action : public QObject {
    Q_OBJECT

public:
    explicit Action(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~Action() = default;
    
    // Выполнить действие
    virtual void execute() = 0;
    
    // Отменить действие (опционально)
    // Пустая реализация по умолчанию, так как не все действия поддерживают отмену
    virtual void undo() {
        // Метод намеренно оставлен пустым, так как базовый класс Action
        // предоставляет опциональную функциональность отмены действий.
        // Производные классы могут переопределить этот метод, если требуется поддержка отмены.
    }
    
    // Получить описание действия
    virtual QString description() const = 0;

signals:
    void executed();  // Сигнал о выполнении действия
};

#endif // ACTION_H

