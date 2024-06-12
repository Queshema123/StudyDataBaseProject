#include "filterwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>

#include <QSqlRecord>
#include <QSqlField>
#include <QSqlQueryModel>

#include <exception>
#include <algorithm>

const QMap<QString, FilterWidget::FieldType> FilterWidget::field_and_type =
{
    {"double", FilterWidget::FieldType::Number}, {"int", FilterWidget::FieldType::Number},
    {"QString", FilterWidget::FieldType::String}
};

QString FilterWidget::getStringOperationCondition(CompareOperations operation, const QString &value)
{
    switch (operation) {
    case CompareOperations::StartsWith:
        return getOperationSQLView(operation) + " '" + value + "%'";
    case CompareOperations::Contains:
        return getOperationSQLView(operation) + " '%" + value + "%'";
    case CompareOperations::EndsWith:
        return getOperationSQLView(operation) + " '%" + value + "' ";
    case CompareOperations::Empty:
        return getOperationSQLView(operation) + " '" + value + "' ";
    default:
        qDebug() << "Compare operation isn't a string opearation!";
        return "";
    }
}

QString FilterWidget::getOperationView(CompareOperations op)
{
    switch (op) {
    case CompareOperations::Contains:
        return "содержит";
    case CompareOperations::EndsWith:
        return "заканчивается на";
    case CompareOperations::Equal:
        return "=";
    case CompareOperations::Greater:
        return ">";
    case CompareOperations::GreaterOrEqual:
        return ">=";
    case CompareOperations::Less:
        return "<";
    case CompareOperations::LessOrEqual:
        return "<=";
    case CompareOperations::NotEqual:
        return "<>";
    case CompareOperations::StartsWith:
        return "начинается с";
    case CompareOperations::Empty:
        return "пусто";
    case CompareOperations::Unable:
        return "выключить";
    default:
        qDebug() << "Unknown operation in getOperationView";
        return "?";
    }
}

QString FilterWidget::getOperationSQLView(CompareOperations op)
{
    switch (op) {
    case CompareOperations::StartsWith:
    case CompareOperations::Contains:
    case CompareOperations::EndsWith:
        return "LIKE";
    case CompareOperations::Empty:
        return "=";
    default:
        return getOperationView(op);
    }
}

QString FilterWidget::getSQLCondition(CompareOperations op, const QString &value, const QString &field)
{
    switch (getTypeOfField(op)) {
    case FieldType::Number:
        return field + " " + getOperationSQLView(op) + " '" + value + "' ";
    case FieldType::String:
        return field + " " + getStringOperationCondition(op, value);
    default:
        qDebug() << "Unknown compare operation to SQL condition!";
        return "";
    }
}

FilterWidget::FieldType FilterWidget::getTypeOfField(CompareOperations operation)
{
    if ( std::find(std::begin(string_operations), std::end(string_operations) , operation) != std::end(string_operations) )
        return FieldType::String;
    else if ( std::find(std::begin(number_operations), std::end(number_operations), operation) !=  std::end(number_operations))
        return FieldType::Number;
    else
        return FieldType::Unknown;
}

FilterWidget::FieldType FilterWidget::getTypeOfField(const QString& field_type)
{
    if(field_and_type.contains(field_type))
        return field_and_type[field_type];
    return FieldType::Unknown;
}

void FilterWidget::addActionsBtns(QLayout* layout, const QString& object_name, const QString& view)
{
    QPushButton* btn = new QPushButton(view);
    btn->setObjectName(object_name);
    layout->addWidget(btn);
}

void FilterWidget::fillListOfColumns(QComboBox* box)
{
    for(auto i = index_column_name.begin(), end = index_column_name.end(); i != end; ++i)
    {
        box->addItem(i.value(), i.key());
    }
}

void FilterWidget::fillListOfOperations(QComboBox* box, FieldType type)
{
    const CompareOperations* operations;
    int end;
    switch (type) {
    case FieldType::Number:
        operations = number_operations;
        end = number_op_cnt;
        break;
    case FieldType::String:
        operations = string_operations;
        end = string_op_cnt;
        break;
    default:
        throw std::logic_error("wrong FieldType in filter_widget");
        break;
    }
    for(int i = 0; i < end; ++i)
    {
        box->addItem( getOperationView(operations[i]), static_cast<int>( operations[i] ) );
    }
}

void FilterWidget::fillFilterInfo()
{
    QList<QWidget*> wgts = this->findChild<QWidget*>("FiltersWidget")->findChildren<QWidget*>("FilterLineWidget");
    for(auto wgt : wgts)
    {
        CompareOperations operation = static_cast<CompareOperations>( wgt->findChild<QComboBox*>("OperationsComboBox")->currentData().toInt() );
        if(operation == CompareOperations::Unable)
            continue;
        int col_ind = wgt->findChild<QComboBox*>("ColumnsComboBox")->currentData().toInt();
        QString value = wgt->findChild<QLineEdit*>()->text();
        info.append( Info(col_ind, operation, value) );
    }
}

void FilterWidget::submitChooseFilters()
{
    info.clear();
    fillFilterInfo();
    emit submitFilters(info);
}

FilterWidget::FilterWidget(QWidget* parent) : QWidget{parent}
{
    QVBoxLayout* main_layout = new QVBoxLayout;
    this->setLayout(main_layout);
    QHBoxLayout* btns_layout = new QHBoxLayout;
    main_layout->addLayout(btns_layout);

    QVBoxLayout* filters_layout = new QVBoxLayout;
    filters_layout->setObjectName("FiltersLayout");
    QWidget* filters_wgt = new QWidget(this);
    filters_wgt->setObjectName("FiltersWidget");
    filters_wgt->setLayout(filters_layout);

    main_layout->addWidget(filters_wgt);

    //addActionsBtns(btns_layout, "addFilterBtn",    "Add filter"    );
    addActionsBtns(btns_layout, "submitFilterBtn", "Submit" );
    addActionsBtns(btns_layout, "clearFiltersBtn", "Clear" );
    // QObject::connect(this->findChild<QPushButton*>("addFilterBtn"),    &QPushButton::clicked, this, SLOT(addFilter()));
    QObject::connect(this->findChild<QPushButton*>("submitFilterBtn"), &QPushButton::clicked, this, &FilterWidget::submitChooseFilters);
    QObject::connect(this->findChild<QPushButton*>("clearFiltersBtn"), &QPushButton::clicked, this, &FilterWidget::clearFiltersEffects);
    QObject::connect(this, &FilterWidget::clearFiltersEffects, this, &FilterWidget::clearFilterWidgets);
}

void FilterWidget::addFilter(const QString &field, const QString &type)
{
    QHBoxLayout* filter_layout = new QHBoxLayout;
    QWidget* filter_wgt = new QWidget( this->findChild<QWidget*>("FiltersWidget") );
    filter_wgt->setLayout(filter_layout);
    filter_wgt->setObjectName("FilterLineWidget");
    this->findChild<QLayout*>("FiltersLayout")->addWidget(filter_wgt);

    QComboBox* col_list = new QComboBox;
    col_list->setObjectName("ColumnsComboBox");
    QComboBox* op_list = new QComboBox;
    op_list->setObjectName("OperationsComboBox");
    QLineEdit* input_line = new QLineEdit;

    // QPushButton* delete_btn = new QPushButton("Delete filter");
    filter_layout->addWidget(col_list, 0, Qt::AlignLeft);
    filter_layout->addWidget(op_list, 0, Qt::AlignLeft);
    filter_layout->addWidget(input_line, 0, Qt::AlignRight);
    // filter_layout->addWidget(delete_btn, 0, Qt::AlignRight);

    col_list->addItem(field, index_column_name.key(field));
    fillListOfOperations( op_list, getTypeOfField(type) );
    // QObject::connect(delete_btn, SIGNAL(clicked(bool)), this, SLOT(deleteFilter()));
}

void FilterWidget::deleteFilter()
{
    QWidget* filter_wgt = qobject_cast<QWidget*>( QObject::sender()->parent() );
    if(filter_wgt)
    {
        this->findChild<QLayout*>("FiltersLayout")->removeWidget(filter_wgt);
        filter_wgt->layout()->deleteLater();
        filter_wgt->deleteLater();
    }
}

void FilterWidget::deleteFilterWidgets()
{
    QList<QWidget*> wgts = this->findChild<QWidget*>("FiltersWidget")->findChildren<QWidget*>();
    for(auto wgt: wgts)
    {
        if(!wgt)
            continue;
        wgt->deleteLater();
    }
}

void FilterWidget::updateColumns(const QSqlQueryModel& model)
{
    info.clear();
    index_column_name.clear();
    deleteFilterWidgets();
    QSqlRecord record = model.record();
    for(int i = 0; i < record.count(); ++i)
    {
        index_column_name.insert( i, record.fieldName(i) );
        addFilter( record.fieldName(i), record.field(i).metaType().name() );
    }
}

void FilterWidget::clearFilterWidgets()
{
    QList<QLineEdit*> input_lines = this->findChild<QWidget*>("FiltersWidget")->findChildren<QLineEdit*>();
    for(auto input_line : input_lines)
    {
        input_line->clear();
    }
}
