#include "searchwidget.h"
#include "db_functions.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QObject>
#include <QtConcurrent>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

#include <limits>
#include <algorithm>

int index_of_next_min(const QList<int>& list, int min_index)
{
    int new_min( std::numeric_limits<int>::max() );
    int new_min_index{-1};
    for(int i = 0; i < list.size(); ++i)
    {
        if(i > min_index && new_min > list.at(i) )
        {
            new_min = list.at(i);
            new_min_index = i;
        }
    }
    return new_min_index;
}

void SearchWidget::setNextElement(int index_if_sort)
{
    if(isSorted)
        current_row_index = index_if_sort;
    else
        current_row = index_of_next_min(searched_rows, current_row_index);

    current_row = searched_rows[current_row_index];
    emit searchedRow( current_row );
    emit changeElement(current_row_index+1);
}

void SearchWidget::addSwitchesWgts()
{
    QHBoxLayout* switch_wgts_layout = new QHBoxLayout;
    switch_wgts_layout->setAlignment(Qt::AlignCenter);

    QWidget* switch_wgt = new QWidget(this);
    switch_wgt->setLayout(switch_wgts_layout);
    this->layout()->addWidget(switch_wgt);

    QLabel* current_element_number = new QLabel("1");
    current_element_number->setMaximumWidth(50);
    QLabel* max_elements_cnt = new QLabel("1");
    max_elements_cnt->setMaximumWidth(50);
    QLabel* separate = new QLabel("/");
    separate->setMaximumWidth(5);
    QPushButton* prev_btn = new QPushButton("previous");
    QPushButton* next_btn = new QPushButton("next");

    switch_wgts_layout->addWidget(prev_btn);
    switch_wgts_layout->addWidget(current_element_number);
    switch_wgts_layout->addWidget(separate);
    switch_wgts_layout->addWidget(max_elements_cnt);
    switch_wgts_layout->addWidget(next_btn);

    QObject::connect(this, &SearchWidget::searchedRowsCount, max_elements_cnt,   [max_elements_cnt](int elem) { max_elements_cnt->setNum(elem); } );
    QObject::connect(this, &SearchWidget::changeElement, current_element_number, [current_element_number](int elem) { current_element_number->setNum(elem); } );

    auto get_next_index = [this](int increment, int bound)
    {
        return (current_row_index+increment < bound) ? current_row_index+increment : bound-1;
    };
    QObject::connect(prev_btn, &QPushButton::clicked, this, [this, get_next_index](){ setNextElement( get_next_index(-1, 1) ); } );
    QObject::connect(next_btn, &QPushButton::clicked, this, [this, get_next_index](){ setNextElement( get_next_index(1, searched_rows.size()) ); } );
}

SearchWidget::SearchWidget(FilterWidget* parent): FilterWidget{parent}, f_thread_idx{0}, current_row{std::numeric_limits<int>::max()}, current_row_index{-1},
    isFind{false}, isSorted{false}
{
    this->setWindowTitle("Search");
    QObject::connect(this, &SearchWidget::clearFiltersEffects, this, &SearchWidget::clearFilterWidgets);
    addSwitchesWgts();
}

SearchTaskThread::SearchTaskThread(SearchWidget* wgt, const QVector<Info>& info, const QString& query, const QString& db_name, int fpage, int lpage) : wgt{wgt},
    db_name{db_name}, query{query}, info{&info}, fpage{fpage}, lpage{lpage}
{
    int fidx{ static_cast<int>(query.indexOf("LIMIT")) + 5}, lidx{ static_cast<int>(query.indexOf("OFFSET")) };
    int rows{ query.mid(fidx, lidx - fidx).simplified().toInt() };

    this->cur_row = (fpage-1)*rows + 1;
    this->query = query.mid(0, query.indexOf("LIMIT")) + "LIMIT " + QString::number( (lpage-fpage+1)*rows ) + " OFFSET " + QString::number( (fpage-1)*rows );
}

bool SearchWidget::isRightRow(const QSqlRecord &row, const QVector<Info>& info)
{
    for(int i = 0; i < info.size(); ++i)
    {
        if(info[i].operation == FilterWidget::CompareOperations::Unable)
            continue;
        if( !FilterWidget::compareValues(info[i].operation, row.value(info[i].column_id), info[i].value) )
            return false;
    }
    return true;
}

void SearchWidget::selectFirstRow(int row)
{
    int idx = search_pool.indexOf( QThread::currentThread() );
    if(row > current_row && !search_pool[f_thread_idx]) // Если поток, в котором было найденно наименьшее значение отработал
    {
        isFind = true;
        emit searchedRow(current_row);
        return;
    }
    else if (row > current_row)
        return;

    current_row = row;
    lst_thread_idx = (lst_thread_idx < idx) ? idx : lst_thread_idx;

    while(f_thread_idx < lst_thread_idx && !search_pool[f_thread_idx]) // Проверяем работают ли потоки предыдущих страниц
        ++f_thread_idx; // Если поток данной страницы отработал, то проверяем следующий

    if(f_thread_idx == lst_thread_idx) // Если все предыдущие потоки закончили выполнение и в данном потоке найденно значение
    {
        isFind = true;
        emit searchedRow(current_row);
    }
}

void SearchWidget::appendRow(int row)
{
    searched_rows.append(row);

    if(!isFind)
        selectFirstRow(row);

    emit searchedRowsCount(searched_rows.size());
}

void SearchTaskThread::run()
{
    QSqlQuery sqlQ(query, getDB(db_name));
    while(sqlQ.next())
    {
        if( wgt->isRightRow( sqlQ.record(), *info))
            emit findedRow(cur_row);
        ++cur_row;
    }
}

void SearchWidget::searchTaskEnd()
{
    int i = 0;
    while(i < search_pool.size() && !search_pool[i])
        ++i;

    if( i != search_pool.size())
        return;

    std::sort( searched_rows.begin(), searched_rows.end() );
    isSorted = true;
    current_row_index = searched_rows.indexOf(current_row);
    emit changeElement( current_row_index+1 );
}

void SearchWidget::fillPool(const QVector<Info>& info, const QString& query, const QString& db_name, int max_page)
{
    int thread_cnt{ QThread::idealThreadCount() };
    int page_to_thread{ max_page / thread_cnt };
    lst_thread_idx = thread_cnt - 1;
    search_pool.clear();
    for(int i = 1; i < thread_cnt; ++i)
    {
        SearchTaskThread* task = new SearchTaskThread(this, info, query, db_name, (i-1)*page_to_thread + 1, i*page_to_thread);
        QObject::connect(task, &SearchTaskThread::findedRow, this, &SearchWidget::appendRow);

        QObject::connect(task, &SearchTaskThread::finished, this, [this, i]()
        {
            QObject::sender()->deleteLater();
            search_pool[i-1] = nullptr;
            searchTaskEnd();
        }
        );

        search_pool.insert(i-1, task);
        task->start();
    }
}

void SearchWidget::search(const QVector<Info>& info, const QString& query, const QString& db_name, int max_page)
{
    clearCache();
    fillPool(info, query, db_name, max_page);
}

void SearchWidget::clearCache()
{
    searched_rows.clear();
    f_thread_idx = 0;
    current_row = std::numeric_limits<int>::max();
    isFind = false;
    isSorted = false;
    current_row_index = -1;
}
