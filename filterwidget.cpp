#include "filterwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QVariant>
#include <QPointer>

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

const std::array<std::function<bool(const QVariant&, const QVariant&)>, static_cast<int>(FilterWidget::CompareOperations::MaxOp)> FilterWidget::compares_func =
{
    [](const QVariant& left, const QVariant& right){ return QVariant::compare(left, right) == 0; },
    [](const QVariant& left, const QVariant& right){ return QVariant::compare(left, right) != 0; },
    [](const QVariant& left, const QVariant& right){ return QVariant::compare(left, right) > 0; },
    [](const QVariant& left, const QVariant& right){ return QVariant::compare(left, right) < 0; },
    [](const QVariant& left, const QVariant& right){ return QVariant::compare(left, right) >= 0; },
    [](const QVariant& left, const QVariant& right){ return QVariant::compare(left, right) <= 0; },

    [](const QVariant& left, const QVariant& right){ return left.toString().indexOf(right.toString()) == 0; },
    [](const QVariant& left, const QVariant& right){
        QString l = left.toString();
        QString r = right.toString();
        return l.mid(l.indexOf(r)).length() == r.length();
    },
    [](const QVariant& left, const QVariant& right){ return left.toString().indexOf(right.toString()) >= 0; },

    [](const QVariant& left, const QVariant& right){ return QVariant::compare(left, right) == 0; },
    [](const QVariant& left, const QVariant& right){ return false; },
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
    QList<Info>::iterator cur_inf = info.begin();
    QList<QWidget*> wgts = this->findChildren<QWidget*>("FilterLineWidget");
    for(const auto& wgt : wgts)
    {
        CompareOperations operation = static_cast<CompareOperations>(wgt->findChild<QComboBox*>("OperationsComboBox")->currentData().toInt() );
        int col_ind = wgt->findChild<QComboBox*>("ColumnsComboBox")->currentData().toInt();
        QString value = wgt->findChild<QLineEdit*>()->text();

        cur_inf->column_id = col_ind;
        cur_inf->operation = operation;
        cur_inf->field_name = index_column_name[col_ind];
        cur_inf->value = value;
        ++cur_inf;
    }
}

void FilterWidget::submitChooseFilters()
{
    fillFilterInfo();
    if(cached_info_for_tabs[current_tab] != info)
    {
        cached_info_for_tabs[current_tab] = info;
    }
    emit submitFilters(cached_info_for_tabs[current_tab]);
}

FilterWidget::FilterWidget(QDialog* parent) : QDialog{parent}
{
    this->setModal(true);
    this->setWindowTitle("Filter");
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

    addActionsBtns(btns_layout, "submitFilterBtn", "Submit" );
    addActionsBtns(btns_layout, "clearFiltersBtn", "Clear" );
    QObject::connect(this->findChild<QPushButton*>("submitFilterBtn"), &QPushButton::clicked, this, &FilterWidget::submitChooseFilters);
    QObject::connect(this->findChild<QPushButton*>("clearFiltersBtn"), &QPushButton::clicked, this, &FilterWidget::clearFiltersEffects);
    QObject::connect(this, &FilterWidget::clearFiltersEffects, this, &FilterWidget::clearFilterWidgets);
}

void FilterWidget::addFilter(const QString &field, const QString &type, const Info* inf)
{
    auto ptr = std::make_unique<const Info>();
    if(!inf)
        inf = ptr.get();
    else
        ptr.release(); // Освобождаем память

    QHBoxLayout* filter_layout = new QHBoxLayout;
    QWidget* filter_wgt = new QWidget( this );
    filter_wgt->setLayout(filter_layout);
    filter_wgt->setObjectName("FilterLineWidget");
    this->findChild<QLayout*>("FiltersLayout")->addWidget(filter_wgt);

    QComboBox* col_list = new QComboBox(filter_wgt);
    col_list->setObjectName("ColumnsComboBox");
    QComboBox* op_list = new QComboBox(filter_wgt);
    op_list->setObjectName("OperationsComboBox");
    QLineEdit* input_line = new QLineEdit(filter_wgt);
    input_line->setObjectName("InputValueLineEdit");
    input_line->setText( inf->value.toString() );

    filter_layout->addWidget(col_list, 0, Qt::AlignLeft);
    filter_layout->addWidget(op_list, 0, Qt::AlignLeft);
    filter_layout->addWidget(input_line, 0, Qt::AlignRight);

    col_list->addItem(field, index_column_name.key(field));
    fillListOfOperations( op_list, getTypeOfField(type) );
    op_list->setCurrentText( getOperationView(inf->operation) );
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
    QList<QWidget*> wgts = this->findChildren<QWidget*>("FilterLineWidget");
    for(auto wgt: wgts)
    {
        if(!wgt)
            continue;
        this->findChild<QLayout*>("FiltersLayout")->removeWidget(wgt);
        wgt->layout()->deleteLater();
        wgt->deleteLater();
    }
}

void FilterWidget::updateColumns(const QSqlQueryModel& model)
{
    current_tab = QObject::sender();

    if(!cached_info_for_tabs.contains(current_tab))
        cached_info_for_tabs.insert(current_tab, QList<Info>(model.record().count()) );

    info = cached_info_for_tabs[current_tab];
    index_column_name.clear();
    deleteFilterWidgets();

    QSqlRecord record = model.record();
    for(int i = 0; i < record.count(); ++i)
    {
        index_column_name.insert( i, record.fieldName(i) );
        info[i].field_name = record.fieldName(i);
        info[i].column_id = i;
        addFilter( info[i].field_name, record.field( info[i].field_name ).metaType().name(), &info[i] );
    }
}

void FilterWidget::clearFilterWidgets()
{
    QList<QLineEdit*> input_lines = this->findChild<QWidget*>("FiltersWidget")->findChildren<QLineEdit*>();
    for(auto input_line : input_lines)
    {
        input_line->clear();
    }
    QList<QComboBox*> operations_boxes = this->findChildren<QComboBox*>("OperationsComboBox");
    for(auto box : operations_boxes)
    {
        box->setCurrentText( getOperationView(CompareOperations::Unable) );
    }
}

bool FilterWidget::compareValues(CompareOperations op, const QVariant &left, const QVariant &right)
{
    return compares_func[static_cast<int>(op)](left, right);
}

bool Info::operator==(const Info& other) const
{
    return column_id == other.column_id && field_name == other.field_name && operation == other.operation && value == other.value;
}

bool operator ==(const QList<Info>& left, const QList<Info>& right)
{
    if(left.size() != right.size())
        return false;
    for(int i = 0; i < left.size(); ++i)
    {
        if(left[i] != right[i])
            return false;
    }
    return true;
}

bool operator !=(const QList<Info>& left, const QList<Info>& right ) { return !(left == right); }
