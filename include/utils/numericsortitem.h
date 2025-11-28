#ifndef NUMERICSORTITEM_H
#define NUMERICSORTITEM_H

#include <QTableWidgetItem>
#include <QVariant>

// Кастомный класс для числовой сортировки в QTableWidget
// Хранит стоимость как число (double), а не как строку
class NumericSortItem : public QTableWidgetItem {
public:
    NumericSortItem(const QString& text, double numericValue)
        : QTableWidgetItem(text), numericValue_(numericValue) {
        // Сохраняем числовое значение для сортировки в DisplayRole и UserRole
        setData(Qt::DisplayRole, text);
        setData(Qt::UserRole, numericValue);
        setData(Qt::EditRole, numericValue);
    }
    
    // Override operator< для QTableWidgetItem (требуется базовым классом)
    // 
    // ВАЖНО: Этот оператор НЕ МОЖЕТ быть hidden friend, так как:
    // 1. QTableWidgetItem::operator< является виртуальной функцией
    // 2. Для переопределения виртуальной функции требуется, чтобы оператор был членом класса
    // 3. Hidden friend операторы не могут быть виртуальными функциями
    // 
    // Это исключение из правила C++ Core Guidelines C.168 (Make operator< a hidden friend),
    // так как правило применимо только к невиртуальным операторам сравнения.
    bool operator<(const QTableWidgetItem& other) const override {
        return lessThan(&other);
    }
    
    // Hidden friend для симметричного сравнения между двумя NumericSortItem
    // Этот оператор является hidden friend, так как не переопределяет виртуальную функцию
    friend bool operator<(const NumericSortItem& lhs, const NumericSortItem& rhs) {
        return lhs.numericValue_ < rhs.numericValue_;
    }
    
private:
    // Вспомогательная функция для сравнения
    bool lessThan(const QTableWidgetItem* other) const {
        if (!other) {
            return false;
        }
        
        // Всегда пытаемся сравнить по числовому значению
        // Сначала проверяем UserRole (должно быть у всех NumericSortItem)
        QVariant otherData = other->data(Qt::UserRole);
        if (otherData.isValid() && otherData.canConvert<double>()) {
            double otherValue = otherData.toDouble();
            return numericValue_ < otherValue;
        }
        
        // Если UserRole нет, пытаемся извлечь число из текста
        QString otherText = other->text();
        otherText = otherText.replace(" руб", "").replace(" ", "").trimmed();
        bool ok;
        double otherValue = otherText.toDouble(&ok);
        
        if (ok) {
            return numericValue_ < otherValue;
        }
        
        // Если ничего не получилось, возвращаем false (этот элемент меньше)
        // Это не должно происходить, если все элементы NumericSortItem
        return false;
    }
    
    // Переопределяем data() чтобы возвращать число для сортировки
    QVariant data(int role) const override {
        if (role == Qt::UserRole) {
            return numericValue_;
        }
        return QTableWidgetItem::data(role);
    }
    
private:
    double numericValue_;  // Стоимость хранится как число, а не строка
};

#endif // NUMERICSORTITEM_H

