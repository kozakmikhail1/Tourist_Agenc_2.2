# Исправление ошибки: Automatic Analysis конфликт

## Проблема

Ошибка: `ERROR You are running CI analysis while Automatic Analysis is enabled. Please consider disabling one or the other.`

Это означает, что в настройках проекта SonarCloud включен автоматический анализ, и одновременно запускается CI анализ через GitHub Actions.

## Решение: Отключить Automatic Analysis в SonarCloud

### Шаг 1: Откройте настройки проекта в SonarCloud

1. Перейдите на [SonarCloud.io](https://sonarcloud.io/)
2. Войдите в свой аккаунт
3. Откройте ваш проект `Tourist_Agenc_2.2`

### Шаг 2: Отключите Automatic Analysis

1. В проекте перейдите в **Project Settings** (Настройки проекта)
   - Нажмите на иконку шестеренки ⚙️ в правом верхнем углу
   - Или перейдите по пути: **Administration** → **General Settings**

2. Найдите раздел **"Analysis Method"** или **"Automatic Analysis"**

3. Отключите автоматический анализ:
   - Найдите опцию **"Automatic Analysis"** или **"Auto-scan"**
   - Переключите её в состояние **OFF** или **Disabled**
   - Или выберите **"CI/CD"** как метод анализа

4. Сохраните изменения (нажмите **"Save"** или **"Update"**)

### Альтернативный путь (если не нашли в General Settings)

1. Перейдите в **Administration** → **Analysis Method**
2. Выберите **"CI/CD"** вместо **"Automatic Analysis"**
3. Сохраните изменения

## После отключения

После отключения автоматического анализа:

1. CI анализ через GitHub Actions будет работать нормально
2. Анализ будет запускаться только при:
   - Push в ветки `main`, `master`, `develop`
   - Создании Pull Request
   - Ручном запуске через GitHub Actions

## Проверка

После отключения автоматического анализа:

1. Сделайте новый commit и push в репозиторий
2. Или запустите workflow вручную через GitHub Actions
3. Проверьте, что ошибка больше не появляется

## Дополнительная информация

- **Automatic Analysis** - SonarCloud автоматически анализирует код при каждом push
- **CI/CD Analysis** - анализ запускается через CI/CD пайплайны (GitHub Actions, GitLab CI, и т.д.)

Для проектов с CI/CD рекомендуется использовать **CI/CD Analysis**, так как:
- Больше контроля над процессом анализа
- Можно настроить дополнительные шаги (компиляция, тесты, покрытие кода)
- Лучшая интеграция с процессом разработки

