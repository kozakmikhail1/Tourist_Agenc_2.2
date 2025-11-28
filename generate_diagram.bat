@echo off
echo ========================================
echo Генерация диаграммы классов в высоком качестве
echo ========================================
echo.

REM Проверка наличия Java
where java >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ОШИБКА] Java не найдена!
    echo Пожалуйста, установите Java: https://www.java.com/
    pause
    exit /b 1
)

REM Проверка наличия plantuml.jar
if not exist "plantuml.jar" (
    echo [ОШИБКА] Файл plantuml.jar не найден!
    echo.
    echo Пожалуйста, скачайте plantuml.jar:
    echo 1. Перейдите на https://plantuml.com/download
    echo 2. Скачайте plantuml.jar
    echo 3. Поместите его в папку с проектом
    echo.
    pause
    exit /b 1
)

REM Проверка наличия файла диаграммы
if not exist "class_diagram.puml" (
    echo [ОШИБКА] Файл class_diagram.puml не найден!
    pause
    exit /b 1
)

echo Генерация SVG (векторный формат, лучшее качество)...
java -jar plantuml.jar -tsvg -DPLANTUML_LIMIT_SIZE=16384 -charset UTF-8 class_diagram.puml
if %ERRORLEVEL% EQU 0 (
    echo [OK] class_diagram.svg создан!
) else (
    echo [ОШИБКА] Не удалось создать SVG
)

echo.
echo Генерация PNG с высоким разрешением (300 DPI для печати А2)...
java -jar plantuml.jar -tpng -DPLANTUML_LIMIT_SIZE=16384 -SDPI=300 -charset UTF-8 class_diagram.puml
if %ERRORLEVEL% EQU 0 (
    echo [OK] class_diagram.png создан с разрешением 300 DPI!
) else (
    echo [ОШИБКА] Не удалось создать PNG
)

echo.
echo Генерация PNG с очень высоким разрешением (600 DPI для максимального качества)...
java -jar plantuml.jar -tpng -DPLANTUML_LIMIT_SIZE=16384 -SDPI=600 -charset UTF-8 class_diagram.puml
if %ERRORLEVEL% EQU 0 (
    echo [OK] class_diagram.png создан с разрешением 600 DPI!
    echo [INFO] Файл будет очень большим, но качество максимальное
) else (
    echo [ОШИБКА] Не удалось создать PNG 600 DPI
)

echo.
echo Генерация PDF (высокое качество)...
java -jar plantuml.jar -tpdf -DPLANTUML_LIMIT_SIZE=16384 -charset UTF-8 class_diagram.puml
if %ERRORLEVEL% EQU 0 (
    echo [OK] class_diagram.pdf создан!
) else (
    echo [ОШИБКА] Не удалось создать PDF
)

echo.
echo ========================================
echo Готово! Файлы созданы в текущей папке.
echo ========================================
echo.
echo Рекомендация: Используйте SVG для печати на А2
echo (лучшее качество, масштабируется без потери)
echo.
pause

