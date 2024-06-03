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
#include <QSqlTableModel>

void FilterWidget::addActionsBtns(QLayout* layout, const QString& object_name, const QString& view)
{
    QPushButton* btn = new QPushButton(view);
    btn->setObjectName(object_name);
    layout->addWidget(btn);
}

void FilterWidget::fillListOfColumns(QComboBox* box)
{
    for(auto i = columns_names_indexes.begin(), end = columns_names_indexes.end(); i != end; ++i)
    {
        box->addItem(i.key(), i.value());
    }
}

void FilterWidget::fillListOfOperations(QComboBox* box)
{
    int end = static_cast<int>(MultiSortFilterProxyModel::CompareOperation::MaxOp);
    for(auto i = static_cast<int>(MultiSortFilterProxyModel::CompareOperation::Equal); i < end; ++i)
    {
        box->addItem( MultiSortFilterProxyModel::getViewOfOperation( static_cast<MultiSortFilterProxyModel::CompareOperation>(i) ), i);
    }
}

QVariant FilterWidget::getDataToColumn(const QString& data, int column)
{
    QVariant v(data);
    if(v.convert( column_type[column] ))
        return v;
    return data;
}

QMap<int, QVariant> FilterWidget::getColumnsAndValuesFilters()
{
    QMap<int, QVariant> filters;
    const QObjectList filters_wgts = this->findChild<QWidget*>("FiltersWidget")->children();
    for(auto filter_wgt : filters_wgts)
    {
        filter_wgt = qobject_cast<QWidget*>(filter_wgt);
        if(filter_wgt)
        {
            int column = filter_wgt->findChild<QComboBox*>("ColumnsComboBox")->currentData().toInt();
            QVariant data = getDataToColumn(filter_wgt->findChild<QLineEdit*>()->text(), column);
            filters.insert(column, data);
        }
    }
    return filters;
}

QMap<int, MultiSortFilterProxyModel::CompareOperation> FilterWidget::getColumnsAndOperationsFilters()
{
    typedef MultiSortFilterProxyModel::CompareOperation operation;
    QMap<int, operation> operations;

    const QObjectList filters_wgts = this->findChild<QWidget*>("FiltersWidget")->children();
    for(auto filter_wgt : filters_wgts)
    {
        filter_wgt = qobject_cast<QWidget*>(filter_wgt);
        if(filter_wgt)
        {
            int column = filter_wgt->findChild<QComboBox*>("ColumnsComboBox")->currentData().toInt();
            operation op = static_cast<operation>( filter_wgt->findChild<QComboBox*>("OperationsComboBox")->currentData().toInt() );
            operations.insert(column , op);
        }
    }

    return operations;
}

void FilterWidget::submitChooseFilters()
{
    auto filters = getColumnsAndValuesFilters();
    auto operations = getColumnsAndOperationsFilters();
    emit submitFilters( filters, operations );
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
    addActionsBtns(btns_layout, "submitFilterBtn", "Submit filter" );
    addActionsBtns(btns_layout, "clearFiltersBtn", "Clear filters" );
    // QObject::connect(this->findChild<QPushButton*>("addFilterBtn"),    SIGNAL(clicked(bool)), this, SLOT(addFilter()));
    QObject::connect(this->findChild<QPushButton*>("submitFilterBtn"), SIGNAL(clicked(bool)), this, SLOT(submitChooseFilters()));
    QObject::connect(this->findChild<QPushButton*>("clearFiltersBtn"), SIGNAL(clicked(bool)), this, SIGNAL(clearFiltersEffects()));
}

void FilterWidget::addFilter()
{
    QHBoxLayout* filter_layout = new QHBoxLayout;
    QWidget* filter_wgt = new QWidget( this->findChild<QWidget*>("FiltersWidget") );
    filter_wgt->setLayout(filter_layout);
    this->findChild<QLayout*>("FiltersLayout")->addWidget(filter_wgt);

    QComboBox* col_list = new QComboBox;
    col_list->setObjectName("ColumnsComboBox");
    QComboBox* op_list = new QComboBox;
    op_list->setObjectName("OperationsComboBox");
    QLineEdit* input_line = new QLineEdit;
    QPushButton* delete_btn = new QPushButton("Delete filter");
    filter_layout->addWidget(col_list);
    filter_layout->addWidget(op_list);
    filter_layout->addWidget(input_line);
    filter_layout->addWidget(delete_btn);

    fillListOfColumns( col_list );
    fillListOfOperations( op_list );

    QObject::connect(delete_btn, SIGNAL(clicked(bool)), this, SLOT(deleteFilter()));
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

void FilterWidget::updateColumns(const QSqlQueryModel& model)
{
    columns_names_indexes.clear();
    column_type.clear();
    QSqlRecord record = model.record();
    for(int i = 0; i < record.count(); ++i)
    {
        columns_names_indexes.insert( record.fieldName(i), i );
        column_type.insert( i, record.field(i).metaType() );
    }
}
