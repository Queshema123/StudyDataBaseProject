#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>
#include <QMap>
#include <QComboBox>
#include <QSqlQueryModel>
#include <QVector>

struct Info;

class FilterWidget : public QWidget
{
    Q_OBJECT
public:
    enum class CompareOperations{ Equal, NotEqual, Greater, Less, GreaterOrEqual, LessOrEqual, StartsWith, EndsWith, Contains, Empty, Unable };
    enum class FieldType{Number, String, Unknown};
    FilterWidget(QWidget* parent = nullptr);
public slots:
    void addFilter(const QString &field, const QString &type);
    void deleteFilter();
    void updateColumns(const QSqlQueryModel& model);
    static QString getStringOperationCondition(CompareOperations operation, const QString &value);
    static QString getOperationView(CompareOperations op);
    static QString getOperationSQLView(CompareOperations op);
    static QString getSQLCondition(CompareOperations op, const QString &value, const QString &field);
    static FieldType getTypeOfField(const QString& field_type);
    static FieldType getTypeOfField(CompareOperations operation);
signals:
    void submitFilters(const QVector<Info>& info);
    void clearFiltersEffects();
private slots:
    void submitChooseFilters();
    void deleteFilterWidgets();
    void clearFilterWidgets();
private:

    static constexpr int string_op_cnt = 5;
    static constexpr int number_op_cnt = 8;
    static constexpr int column_type_cnt = 2;

    static constexpr CompareOperations string_operations[string_op_cnt] =
    {
            FilterWidget::CompareOperations::Contains, FilterWidget::CompareOperations::EndsWith, FilterWidget::CompareOperations::StartsWith,
            FilterWidget::CompareOperations::Empty, FilterWidget::CompareOperations::Unable
    };
    static constexpr CompareOperations number_operations[number_op_cnt] =
    {
            FilterWidget::CompareOperations::Equal,   FilterWidget::CompareOperations::NotEqual,
            FilterWidget::CompareOperations::Greater, FilterWidget::CompareOperations::GreaterOrEqual,
            FilterWidget::CompareOperations::Less,    FilterWidget::CompareOperations::LessOrEqual,
            FilterWidget::CompareOperations::Empty, FilterWidget::CompareOperations::Unable
    };
    static constexpr FieldType column_types[column_type_cnt] =
    {
        FilterWidget::FieldType::Number, FilterWidget::FieldType::String
    };
    static const QMap<QString, FieldType> field_and_type;

    QMap<int, QString> index_column_name;
    QVector<Info> info;
    void addActionsBtns(QLayout* layout, const QString& object_name, const QString& view);
    void fillListOfColumns(QComboBox* box);
    void fillListOfOperations(QComboBox* box, FieldType type);
    void fillFilterInfo();
};

struct Info
{
    int column_id;
    FilterWidget::CompareOperations operation;
    QString value;
    Info(int id, FilterWidget::CompareOperations op, const QString& val) : column_id{id}, operation{op}, value{val} {}
};

#endif // FILTERWIDGET_H
