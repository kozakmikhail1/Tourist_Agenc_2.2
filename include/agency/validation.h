#ifndef AGENCY_VALIDATION_H
#define AGENCY_VALIDATION_H

#include <QString>

namespace agency {

struct ValidationResult {
    bool ok = true;
    QString errorMessage;

    static ValidationResult success() { return {true, {}}; }
    static ValidationResult failure(const QString& msg) { return {false, msg}; }
};

/** Логин: 3–32 символа, буквы/цифры/«_», без «;» и пробелов. */
ValidationResult validateLogin(const QString& login);

/** Пароль: 4–128 символов, без символа-разделителя «;». */
ValidationResult validatePassword(const QString& password);

/** Ввод пароля при входе: не пустой, без «;», не длиннее 128 (для учётных записей до регистрации по правилам). */
ValidationResult validatePasswordLoginAttempt(const QString& password);

/** Поля тура: страна/отель/адрес, звёзды 1–5, даты yyyy-MM-dd, конец ≥ начала. */
ValidationResult validateTourFields(const QString& country,
                                    int price,
                                    int days,
                                    const QString& hotel,
                                    const QString& hotelAddress,
                                    int hotelStars,
                                    const QString& dateStart,
                                    const QString& dateEnd);

/** Необязательное поле в фильтре: пусто или целое ≥ 0. */
ValidationResult parseOptionalNonNegativeInt(const QString& text, int& outValue, const QString& fieldName);

/** Два необязательных поля диапазона; 0 = «без ограничения». min≤max, если оба заданы. */
ValidationResult parseOptionalIntRange(const QString& minText,
                                     const QString& maxText,
                                     int& minOut,
                                     int& maxOut,
                                     const QString& fieldMinName,
                                     const QString& fieldMaxName);

} // namespace agency

#endif
