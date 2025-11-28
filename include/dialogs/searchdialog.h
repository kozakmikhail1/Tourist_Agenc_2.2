#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <memory>
#include "models/tour.h"
#include "containers/datacontainer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SearchDialog; }
QT_END_NAMESPACE

// Forward declaration для включения в .cpp
// Полное определение Ui::SearchDialog находится в ui_searchdialog.h

class SearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit SearchDialog(QWidget *parent = nullptr, 
                         DataContainer<Tour>* tours = nullptr);
    ~SearchDialog();

private slots:
    void search();
    void onResultSelected();

private:
    std::unique_ptr<Ui::SearchDialog> ui;
    DataContainer<Tour>* tours_;
    
    void updateResultsTable(const QVector<Tour>& results);
    bool matchesTourFilter(const Tour& tour, const QString& countryFilter, double maxCost,
                          int minDuration, int countryFilterState, int costFilterState,
                          int durationFilterState) const;
};

#endif // SEARCHDIALOG_H



