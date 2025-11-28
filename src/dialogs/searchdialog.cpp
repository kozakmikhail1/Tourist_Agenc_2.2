#include "dialogs/searchdialog.h"
#include "ui_searchdialog.h"
#include "utils/numericsortitem.h"
#include <algorithm>
#include <QTableWidgetItem>
#include <iterator>

SearchDialog::SearchDialog(QWidget *parent, DataContainer<Tour>* tours)
    : QDialog(parent)
    , ui(std::make_unique<Ui::SearchDialog>())
    , tours_(tours)
{
    ui->setupUi(this);
    
    // Устанавливаем начальное состояние всех фильтров в "Не применять" (нейтральное)
    ui->countryFilterCombo->setCurrentIndex(0);
    ui->costFilterCombo->setCurrentIndex(0);
    ui->durationFilterCombo->setCurrentIndex(0);
    
    connect(ui->searchButton, &QPushButton::clicked, this, &SearchDialog::search);
    connect(ui->resultsTable, &QTableWidget::itemDoubleClicked, 
            this, &SearchDialog::onResultSelected);
    
    // Подключаем кнопку закрытия
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SearchDialog::~SearchDialog() = default;

void SearchDialog::search() {
    if (!tours_) return;
    
    QString countryFilter = ui->countryEdit->text().trimmed();
    double maxCost = ui->maxCostSpin->value();
    int minDuration = ui->minDurationSpin->value();
    
    // Получаем состояние каждого фильтра (0 - не применять, 1 - фильтровать, 2 - исключить)
    int countryFilterState = ui->countryFilterCombo->currentIndex();
    int costFilterState = ui->costFilterCombo->currentIndex();
    int durationFilterState = ui->durationFilterCombo->currentIndex();
    
    QVector<Tour> results;
    
    // Используем STL алгоритмы для поиска
    std::copy_if(tours_->getData().begin(), tours_->getData().end(),
                 std::back_inserter(results),
                 [this, countryFilter, maxCost, minDuration, countryFilterState, costFilterState, durationFilterState](const Tour& tour) {
        return matchesTourFilter(tour, countryFilter, maxCost, minDuration,
                                countryFilterState, costFilterState, durationFilterState);
    });
    
    // Сортировка по стоимости (используя STL) - по возрастанию
    std::sort(results.begin(), results.end(),
              [](const Tour& a, const Tour& b) {
        return a.calculateCost() < b.calculateCost();
    });
    
    updateResultsTable(results);
}

bool SearchDialog::matchesTourFilter(const Tour& tour, const QString& countryFilter, double maxCost,
                                     int minDuration, int countryFilterState, int costFilterState,
                                     int durationFilterState) const {
    // Фильтр по стране
    if (countryFilterState == 1) { // Фильтровать
        if (countryFilter.isEmpty() || 
            !tour.getCountry().contains(countryFilter, Qt::CaseInsensitive)) {
            return false;
        }
    } else if (countryFilterState == 2) { // Исключить
        if (!countryFilter.isEmpty() && 
            tour.getCountry().contains(countryFilter, Qt::CaseInsensitive)) {
            return false;
        }
    }
    // Если countryFilterState == 0 (не применять), пропускаем этот фильтр
    
    // Фильтр по стоимости
    if (costFilterState == 1) { // Фильтровать
        if (tour.calculateCost() > maxCost) {
            return false;
        }
    } else if (costFilterState == 2) { // Исключить
        if (tour.calculateCost() <= maxCost) {
            return false;
        }
    }
    // Если costFilterState == 0 (не применять), пропускаем этот фильтр
    
    // Фильтр по продолжительности
    if (durationFilterState == 1) { // Фильтровать
        if (tour.getDuration() < minDuration) {
            return false;
        }
    } else if (durationFilterState == 2) { // Исключить
        if (tour.getDuration() >= minDuration) {
            return false;
        }
    }
    // Если durationFilterState == 0 (не применять), пропускаем этот фильтр
    
    return true;
}

void SearchDialog::updateResultsTable(const QVector<Tour>& results) {
    // Временно отключаем сортировку
    ui->resultsTable->setSortingEnabled(false);
    
    ui->resultsTable->setRowCount(results.size());
    
    int row = 0;
    for (const auto& tour : results) {
        ui->resultsTable->setItem(row, 0, new QTableWidgetItem(tour.getName()));
        ui->resultsTable->setItem(row, 1, new QTableWidgetItem(tour.getCountry()));
        ui->resultsTable->setItem(row, 2, 
            new QTableWidgetItem(tour.getStartDate().toString("yyyy-MM-dd")));
        ui->resultsTable->setItem(row, 3, 
            new QTableWidgetItem(tour.getEndDate().toString("yyyy-MM-dd")));
        
        // Для правильной числовой сортировки используем NumericSortItem
        double cost = tour.calculateCost();
        NumericSortItem* costItem = new NumericSortItem(QString::number(cost, 'f', 2) + " руб", cost);
        costItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->resultsTable->setItem(row, 4, costItem);
        ++row;
    }
    
    // Включаем сортировку и сортируем по стоимости по умолчанию
    ui->resultsTable->setSortingEnabled(true);
    ui->resultsTable->sortItems(4, Qt::AscendingOrder); // Сортировка по столбцу стоимости
}

void SearchDialog::onResultSelected() {
    // Можно добавить функционал для детального просмотра тура
}

