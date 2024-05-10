#ifndef MULTISORTFILTERPROXYMODEL_H
#define MULTISORTFILTERPROXYMODEL_H
#include <QSortFilterProxyModel>
#include <QMap>
#include <QVector>

class MultiSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
public:

    enum class CompareOperation{ Equal, Bigger, Less, LessOrEqual, BiggerOrEqual, NotEqual, MaxOp };
    explicit MultiSortFilterProxyModel(QObject* parent = nullptr, const QMap<int, QVariant>& filters = {}, const QMap<int, CompareOperation> &operations = {},
                                       const QVector<int> &showed_rows = {});

    bool comparesValuesOperation(CompareOperation operation, const QVariant& l_val, const QVariant& r_val) const;

    static QString getViewOfOperation(CompareOperation op);
public slots:
    void addFilter(int column, const QVariant& val, CompareOperation op);
    void deleteFilter(int column);
    void clearFilters();
    void setRowsToShow(const QVector<int> &rows);
    void setRowsToShow(int start_index, int end_index);
    void addFilters(const QMap<int, QVariant> &values, const QMap<int, MultiSortFilterProxyModel::CompareOperation> &operations);
private:
    QMap<int, QVariant> column_and_value_to_filter;
    QMap<int, CompareOperation> column_and_operation_to_filter;
    QVector<int> rows_to_show;
};

#endif // MULTISORTFILTERPROXYMODEL_H
