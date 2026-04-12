#ifndef ACTION_H
#define ACTION_H

#include <QObject>
#include <QWidget>

class Action : public QObject {
    Q_OBJECT

public:
    explicit Action(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~Action() = default;
    
    virtual void execute() = 0;
    
    virtual void undo() {
    }
    
    virtual QString description() const = 0;

signals:
    void executed();
};

#endif

