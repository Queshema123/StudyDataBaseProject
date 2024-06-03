#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include "multisortfilterproxymodel.h"
#include <QWidget>
#include <QMap>
#include <QSqlTableModel>
#include <QComboBox>

class FilterWidget : public QWidget
{
    Q_OBJECT

    QMap<QString, int> columns_names_indexes;
    QMap<int, QMetaType> column_type;

    void addActionsBtns(QLayout* layout, const QString& object_name, const QString& view);
    void fillListOfColumns(QComboBox* box);
    void fillListOfOperations(QComboBox* box);
    QVariant getDataToColumn(const QString& data, int column);
    QMap<int, QVariant> getColumnsAndValuesFilters();
    QMap<int, MultiSortFilterProxyModel::CompareOperation> getColumnsAndOperationsFilters();
public:
    FilterWidget(QWidget* parent = nullptr);
public slots:
    void addFilter();
    void deleteFilter();
    void updateColumns(const QSqlQueryModel& model);
signals:
    void submitFilters(const QMap<int, QVariant> &values, const QMap<int, MultiSortFilterProxyModel::CompareOperation> &operations);
    void clearFiltersEffects();
private slots:
    void submitChooseFilters();
};

#endif // FILTERWIDGET_H
