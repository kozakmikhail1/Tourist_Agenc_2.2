#include "agency/validation.h"

#include <QDate>
#include <QRegularExpression>

namespace agency {

namespace {

const QRegularExpression kLoginPattern(QStringLiteral("^[A-Za-zА-Яа-яЁё0-9_]{3,32}$"));

} // namespace

ValidationResult validateLogin(const QString& login)
{
    const QString t = login.trimmed();
    if (t.isEmpty()) {
        return ValidationResult::failure(QStringLiteral("Введите логин."));
    }
    if (t.contains(QLatin1Char(';'))) {
        return ValidationResult::failure(QStringLiteral("Логин не может содержать символ «;»."));
    }
    if (t.contains(QLatin1Char(' '))) {
        return ValidationResult::failure(QStringLiteral("Логин не должен содержать пробелы."));
    }
    if (!kLoginPattern.match(t).hasMatch()) {
        return ValidationResult::failure(
            QStringLiteral("Логин: 3–32 символа, буквы (латиница/кириллица), цифры и «_»."));
    }
    return ValidationResult::success();
}

ValidationResult validatePasswordLoginAttempt(const QString& password)
{
    if (password.isEmpty()) {
        return ValidationResult::failure(QStringLiteral("Введите пароль."));
    }
    if (password.size() > 128) {
        return ValidationResult::failure(QStringLiteral("Пароль слишком длинный (макс. 128 символов)."));
    }
    if (password.contains(QLatin1Char(';'))) {
        return ValidationResult::failure(QStringLiteral("Пароль не может содержать символ «;»."));
    }
    return ValidationResult::success();
}

ValidationResult validatePassword(const QString& password)
{
    if (password.size() < 4) {
        return ValidationResult::failure(QStringLiteral("Пароль не короче 4 символов."));
    }
    if (password.size() > 128) {
        return ValidationResult::failure(QStringLiteral("Пароль слишком длинный (макс. 128 символов)."));
    }
    if (password.contains(QLatin1Char(';'))) {
        return ValidationResult::failure(QStringLiteral("Пароль не может содержать символ «;»."));
    }
    return ValidationResult::success();
}

ValidationResult validateTourFields(const QString& country,
                                    int price,
                                    int days,
                                    const QString& hotel,
                                    const QString& hotelAddress,
                                    int hotelStars,
                                    const QString& dateStart,
                                    const QString& dateEnd)
{
    const QString c = country.trimmed();
    const QString h = hotel.trimmed();
    const QString addr = hotelAddress.trimmed();
    const QString ds = dateStart.trimmed();
    const QString de = dateEnd.trimmed();
    if (c.isEmpty()) {
        return ValidationResult::failure(QStringLiteral("Укажите страну."));
    }
    if (h.isEmpty()) {
        return ValidationResult::failure(QStringLiteral("Укажите отель."));
    }
    if (addr.isEmpty()) {
        return ValidationResult::failure(QStringLiteral("Укажите адрес отеля."));
    }
    if (c.contains(QLatin1Char(';')) || h.contains(QLatin1Char(';')) || addr.contains(QLatin1Char(';'))) {
        return ValidationResult::failure(QStringLiteral("Поля не могут содержать символ «;»."));
    }
    if (c.size() > 200 || h.size() > 200 || addr.size() > 300) {
        return ValidationResult::failure(
            QStringLiteral("Страна и отель — до 200 симв.; адрес — до 300 символов."));
    }
    if (price < 1 || price > 50000000) {
        return ValidationResult::failure(QStringLiteral("Цена должна быть в диапазоне 1 … 50 000 000."));
    }
    if (days < 1 || days > 3650) {
        return ValidationResult::failure(QStringLiteral("Число дней: 1 … 3650."));
    }
    if (hotelStars < 1 || hotelStars > 5) {
        return ValidationResult::failure(QStringLiteral("Звёздность отеля: от 1 до 5."));
    }
    const QDate d1 = QDate::fromString(ds, Qt::ISODate);
    const QDate d2 = QDate::fromString(de, Qt::ISODate);
    if (!d1.isValid() || !d2.isValid()) {
        return ValidationResult::failure(
            QStringLiteral("Даты проведения: формат ГГГГ-ММ-ДД (например 2026-07-01)."));
    }
    if (d2 < d1) {
        return ValidationResult::failure(QStringLiteral("Дата окончания тура не раньше даты начала."));
    }
    return ValidationResult::success();
}

ValidationResult parseOptionalNonNegativeInt(const QString& text, int& outValue, const QString& fieldName)
{
    const QString t = text.trimmed();
    if (t.isEmpty()) {
        outValue = 0;
        return ValidationResult::success();
    }
    bool ok = false;
    const int v = t.toInt(&ok);
    if (!ok) {
        return ValidationResult::failure(
            QStringLiteral("Поле «%1»: ожидается целое неотрицательное число.").arg(fieldName));
    }
    if (v < 0) {
        return ValidationResult::failure(QStringLiteral("Поле «%1» не может быть отрицательным.").arg(fieldName));
    }
    outValue = v;
    return ValidationResult::success();
}

ValidationResult parseOptionalIntRange(const QString& minText,
                                       const QString& maxText,
                                       int& minOut,
                                       int& maxOut,
                                       const QString& fieldMinName,
                                       const QString& fieldMaxName)
{
    const ValidationResult r1 = parseOptionalNonNegativeInt(minText, minOut, fieldMinName);
    if (!r1.ok) {
        return r1;
    }
    const ValidationResult r2 = parseOptionalNonNegativeInt(maxText, maxOut, fieldMaxName);
    if (!r2.ok) {
        return r2;
    }
    if (minOut > 0 && maxOut > 0 && minOut > maxOut) {
        return ValidationResult::failure(
            QStringLiteral("«%1» не может быть больше «%2».").arg(fieldMinName, fieldMaxName));
    }
    return ValidationResult::success();
}

} // namespace agency

