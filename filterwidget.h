#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>
#include <QMap>
#include <QComboBox>
#include <QSqlQueryModel>

class FilterWidget : public QWidget
{
    Q_OBJECT
public:
    enum class CompareOperations{ Equal, NotEqual, Greater, Less, GreaterOrEqual, LessOrEqual, StartsWith, EndsWith, Contains };
    enum class FieldType{Number, String, Unknown};
    FilterWidget(QWidget* parent = nullptr);
public slots:
    void addFilter(const QString &field, const QString &type);
    void deleteFilter();
    void updateColumns(const QSqlQueryModel& model);
    QString getStringOperationCondition(CompareOperations operation, const QString &value);
    QString getOperationView(CompareOperations op);
    QString getOperationSQLView(CompareOperations op);
    QString getSQLCondition(CompareOperations op, const QString &value, const QString &field);
    FieldType getTypeOfField(const QString& field_type);
    FieldType getTypeOfField(CompareOperations operation);
signals:
    void submitFilters(const QMap<int, CompareOperations> &operations, const QMap<int, QString> &values);
    void clearFiltersEffects();
private slots:
    void submitChooseFilters();
    void deleteFilterWidgets();
    void clearFilterWidgets();
private:

    static constexpr int string_op_cnt = 3;
    static constexpr int number_op_cnt = 6;
    static constexpr int column_type_cnt = 2;

    static constexpr CompareOperations string_operations[string_op_cnt] =
    {
            FilterWidget::CompareOperations::Contains, FilterWidget::CompareOperations::EndsWith, FilterWidget::CompareOperations::StartsWith
    };
    static constexpr CompareOperations number_operations[number_op_cnt] =
    {
            FilterWidget::CompareOperations::Equal,   FilterWidget::CompareOperations::NotEqual,
            FilterWidget::CompareOperations::Greater, FilterWidget::CompareOperations::GreaterOrEqual,
            FilterWidget::CompareOperations::Less,    FilterWidget::CompareOperations::LessOrEqual,
    };
    static constexpr FieldType column_types[column_type_cnt] =
    {
        FilterWidget::FieldType::Number, FilterWidget::FieldType::String
    };
    static const QMap<QString, FieldType> field_and_type;

    QMap<int, QString> index_column_name;
    QMap<int, CompareOperations> index_operation;
    QMap<int, QString> index_value;
    void addActionsBtns(QLayout* layout, const QString& object_name, const QString& view);
    void fillListOfColumns(QComboBox* box);
    void fillListOfOperations(QComboBox* box, FieldType type);
    void fillFilterInfo();
};

#endif // FILTERWIDGET_H
