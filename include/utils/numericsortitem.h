#ifndef NUMERICSORTITEM_H
#define NUMERICSORTITEM_H

#include <QTableWidgetItem>
#include <QVariant>

class NumericSortItem : public QTableWidgetItem {
public:
    NumericSortItem(const QString& text, double numericValue)
        : QTableWidgetItem(text), numericValue_(numericValue) {
        setData(Qt::DisplayRole, text);
        setData(Qt::UserRole, numericValue);
        setData(Qt::EditRole, numericValue);
    }
    
    bool operator<(const QTableWidgetItem& other) const override {
        return lessThan(&other);
    }
    
    friend bool operator<(const NumericSortItem& lhs, const NumericSortItem& rhs) {
        return lhs.numericValue_ < rhs.numericValue_;
    }
    
private:
    bool lessThan(const QTableWidgetItem* other) const {
        if (!other) {
            return false;
        }
        
        QVariant otherData = other->data(Qt::UserRole);
        if (otherData.isValid() && otherData.canConvert<double>()) {
            double otherValue = otherData.toDouble();
            return numericValue_ < otherValue;
        }
        
        QString otherText = other->text();
        otherText = otherText.replace(" руб", "").replace(" ", "").trimmed();
        bool ok;
        double otherValue = otherText.toDouble(&ok);
        
        if (ok) {
            return numericValue_ < otherValue;
        }
        
        return false;
    }
    
    QVariant data(int role) const override {
        if (role == Qt::UserRole) {
            return numericValue_;
        }
        return QTableWidgetItem::data(role);
    }
    
private:
    double numericValue_;
};

#endif

