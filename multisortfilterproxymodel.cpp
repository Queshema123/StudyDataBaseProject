#include "multisortfilterproxymodel.h"
#include <QVariant>

MultiSortFilterProxyModel::MultiSortFilterProxyModel(QObject* parent, const QMap<int, QVariant> &filters, const QMap<int, CompareOperation> &operations,
    const QVector<int> &showed_rows) :
    QSortFilterProxyModel{parent}, column_and_value_to_filter{filters}, column_and_operation_to_filter{operations}, rows_to_show{showed_rows} { }

void MultiSortFilterProxyModel::setRowsToShow(const QVector<int> &rows)
{
    rows_to_show = rows;
    invalidateFilter();
}

void MultiSortFilterProxyModel::setRowsToShow(int start_index, int end_index)
{
    rows_to_show.clear();
    for(; start_index <= end_index; ++start_index)
    {
        rows_to_show.push_back(start_index);
    }
    invalidateFilter();
}

bool MultiSortFilterProxyModel::comparesValuesOperation(CompareOperation operation, const QVariant& l_val, const QVariant& r_val) const
{
    switch (operation) {
    case CompareOperation::Equal:
        return QVariant::compare(l_val, r_val) == 0;

    case CompareOperation::NotEqual:
        return QVariant::compare(l_val, r_val) != 0;

    case CompareOperation::Bigger:
        return QVariant::compare(l_val, r_val) > 0;

    case CompareOperation::Less:
        return QVariant::compare(l_val, r_val) < 0;

    case CompareOperation::BiggerOrEqual:
        return QVariant::compare(l_val, r_val) >= 0;

    case CompareOperation::LessOrEqual:
        return QVariant::compare(l_val, r_val) <= 0;
    default:
        return false;
    }
}

bool MultiSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if(rows_to_show.size() > 0 && rows_to_show.indexOf(sourceRow) == -1)
        return false;

    for(auto i = column_and_value_to_filter.begin(); i != column_and_value_to_filter.end(); ++i)
    {
        QModelIndex index = sourceModel()->index(sourceRow, i.key(), sourceParent);
        QVariant row_data = sourceModel()->data(index);
        QVariant compared_data = column_and_value_to_filter.value(i.key());
        CompareOperation operation = column_and_operation_to_filter.value(i.key());
        if( !comparesValuesOperation(operation, row_data, compared_data) )
            return false;
    }
    return true;
}

QString MultiSortFilterProxyModel::getViewOfOperation(CompareOperation op)
{
    switch (op) {
    case CompareOperation::Equal:
        return "=";

    case CompareOperation::NotEqual:
        return "<>";

    case CompareOperation::Bigger:
        return ">";

    case CompareOperation::Less:
        return "<";

    case CompareOperation::BiggerOrEqual:
        return ">=";

    case CompareOperation::LessOrEqual:
        return "<=";
    default:
        return "";
    }
}

void MultiSortFilterProxyModel::addFilter(int column, const QVariant& val, CompareOperation op)
{
    column_and_value_to_filter.insert(column, val);
    column_and_operation_to_filter.insert(column, op);
    invalidateFilter();
}

void MultiSortFilterProxyModel::addFilters(const QMap<int, QVariant> &values, const QMap<int, MultiSortFilterProxyModel::CompareOperation> &operations)
{
    column_and_value_to_filter = values;
    column_and_operation_to_filter = operations;
    invalidateFilter();
}

void MultiSortFilterProxyModel::deleteFilter(int column)
{
    column_and_value_to_filter.erase( column_and_value_to_filter.find(column) );
    column_and_operation_to_filter.erase( column_and_operation_to_filter.find(column) );
    invalidateFilter();
}

void MultiSortFilterProxyModel::clearFilters()
{
    column_and_value_to_filter.clear();
    column_and_operation_to_filter.clear();
    this->setFilterRegularExpression("");
}
