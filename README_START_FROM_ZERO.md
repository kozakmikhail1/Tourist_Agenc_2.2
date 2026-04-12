# TouristAgency 2.2 - запуск с нуля (Windows)

Этот файл для человека, у которого на компьютере ничего не установлено.
Ниже пошагово: что скачать, что установить и как запустить проект.

## 1) Что нужно установить

1. `Git for Windows`  
Нужен, чтобы скачать проект из репозитория.

2. `Qt` через `Qt Online Installer`  
При установке отметьте:
- `Qt 6.x` (рекомендуется 6.11+)
- комплект: `Desktop Qt 6.x MinGW 64-bit`
- `Qt Creator`
- в разделе `Developer and Designer Tools`: `MinGW` (компилятор)
- опционально: `Ninja`, `CMake` (если не ставите отдельно)

3. `CMake` (если не выбрали в Qt Installer)  
Добавьте в `PATH` во время установки.

## 2) Скачать проект

Откройте `PowerShell` и выполните:

```powershell
git clone <URL_ВАШЕГО_РЕПОЗИТОРИЯ>
cd TouristAgenc_2.2
```

Если проект уже скачан - просто перейдите в папку проекта.

## 3) Сборка и запуск через Qt Creator (рекомендуется)

1. Откройте `Qt Creator`.
2. `File -> Open File or Project...`
3. Выберите `CMakeLists.txt` в корне проекта.
4. В `Configure Project` выберите Kit:
- `Desktop Qt 6.x MinGW 64-bit`
5. Нажмите `Configure Project`.
6. Нажмите `Build` (молоток), затем `Run` (зеленая стрелка).

После первой сборки рядом с `.exe` автоматически создастся папка `data` с шаблонными файлами.

## 4) Сборка и запуск через консоль (альтернатива)

Из корня проекта:

```powershell
cmake -S . -B build
cmake --build build
.\build\TouristAgency.exe
```

## 5) Первый вход

- Админ: `admin` / `admin`

Если вход не работает, удалите папку `data` рядом с `.exe` и пересоберите проект.

## 6) Если что-то не запускается

1. Проверьте, что выбран правильный Kit: `Desktop Qt 6.x MinGW 64-bit`.
2. Убедитесь, что проект открыт именно через `CMakeLists.txt`.
3. Если ошибка линковки `Permission denied` на `TouristAgency.exe` - закройте запущенное приложение и пересоберите.
4. Если в интерфейсе старые данные - закройте приложение, удалите `data` рядом с `.exe`, соберите снова.

## 7) Где лежит исполняемый файл

Обычно:
- `build\TouristAgency.exe` (при сборке через консоль)
- или в build-папке Qt Creator профиля (`build\Desktop_Qt_...`)

## 8) Как генерировать тестовые данные

В проекте есть генератор: `tools\generate_sample_data.cpp`.
Он перезаписывает:
- `data\accounts.txt`
- `data\tours.txt`
- `data\bookings.txt`

Важно: это сбрасывает текущие локальные изменения в данных.

### 8.1 Сборка и запуск генератора

Из корня проекта:

```powershell
g++ -std=c++17 -O2 tools\generate_sample_data.cpp -o tools\generate_sample_data.exe
.\tools\generate_sample_data.exe
```

Если команда `g++` не найдена:
- запустите команду из терминала Qt Creator (там обычно уже есть MinGW в `PATH`),
- или укажите полный путь к `g++.exe` из вашей установки Qt/MinGW.

### 8.2 Как подхватить новые данные в приложении

Приложение читает `data` рядом с `.exe`, поэтому после генерации:

1. Закройте приложение.
2. Скопируйте обновленные файлы из корневой `data` в `data` рядом с вашим `TouristAgency.exe`.
3. Запустите приложение заново.

Пример (если `.exe` в `build`):

```powershell
Copy-Item data\accounts.txt build\data\accounts.txt -Force
Copy-Item data\tours.txt build\data\tours.txt -Force
Copy-Item data\bookings.txt build\data\bookings.txt -Force
```

