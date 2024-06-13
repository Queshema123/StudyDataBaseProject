#include "tab.h"
#include "db_functions.h"
#include <QtConcurrent>
#include <QTableView>
#include <QSqlRecord>
#include <QVBoxLayout>
#include <QSqlError>
#include "filterwidget.h"

Tab::Tab(QObject *parent, const QWidget* wgt, const QString &table_name, const QString& db_name, int rows_in_page)
    : QObject{parent}, pages(3), tab_wgt{wgt}, table_name(table_name), db_name{db_name}, view{nullptr}, rows_in_page{rows_in_page}, cur_page_model{nullptr}, max_page{1},
    cur_page{1}
{
    view = wgt->findChild<QTableView*>();
    QFuture<void> future = QtConcurrent::run( [this]()
    {
        int center = static_cast<int>(PageName::Center);
        pages[center].model->setQuery( getCurrentPageData(1, this->rows_in_page, "SELECT * FROM " + this->table_name, this->db_name, this->table_name) );
        pages[center].future.waitForFinished();
        cur_page_model = pages[center].model.get();
        fillModels(PageName::Right, 2);
    }
    );
    future.waitForFinished();
    view->setModel( cur_page_model );
    setPagesCount();
}

void Tab::fillModels(PageName page, int index)
{
    auto fillModel = [this](int thread_idx, int page_index)
    {
        pages[thread_idx].model->setQuery( getCurrentPageData(page_index, rows_in_page, cur_page_model->query().lastQuery(), db_name, table_name) );
    };
    int thread_idx = static_cast<int>(page);
    switch (page) {
    case PageName::Left:
        pages[thread_idx].future = QtConcurrent::run(fillModel, thread_idx, index-1);
        break;
    case PageName::Right:
        pages[thread_idx].future = QtConcurrent::run(fillModel, thread_idx, index+1);
        break;
    default:
        thread_idx = static_cast<int>(PageName::Center);

        pages[thread_idx-1].future = QtConcurrent::run(fillModel, thread_idx-1, index-1);
        pages[thread_idx].future   = QtConcurrent::run(fillModel, thread_idx, index  );
        pages[thread_idx+1].future = QtConcurrent::run(fillModel, thread_idx+1, index+1);
        break;
    }
}

QString Tab::getFiltredQuery(const QVector<Info>& info)
{
    QString query{"SELECT * FROM " + table_name + " WHERE 1 = 1 "};
    for(int i = 0; i < info.size(); ++i)
    {
        if(info[i].operation == FilterWidget::CompareOperations::Unable)
            continue;
        query += "AND " + FilterWidget::getSQLCondition(info[i].operation, info[i].value.toString(), cur_page_model->record().fieldName(info[i].column_id)) + " ";
    }
    return query;
}

void Tab::setPagesCount()
{
    auto count = [this](){
        QString query = cur_page_model->query().lastQuery();
        query.replace("*", "COUNT(*)");
        QSqlQuery sqlQ(query, getDB(db_name));
        sqlQ.next();
        int rows = sqlQ.value(0).toInt();
        int pages = ( ( rows - 1) / rows_in_page ) + 1;
        max_page = pages;
        emitPagesCount();
    };
    QFuture<void> future = QtConcurrent::run(count);
}

void Tab::setFilters(const QVector<Info>& info)
{
    if(isBlock)
        return;
    QString query = getFiltredQuery(info);
    QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, 1, rows_in_page, query, db_name, table_name);
    future.waitForFinished();
    pages[1].model->setQuery( future.takeResult() );
    cur_page = 1;
    cur_page_model = pages[1].model.get();
    fillModels(PageName::All, 1);
    setPagesCount();
    emitCurrentPageInfo();
    emitCurrentPage();
}

void Tab::setDefaultPage()
{
    if(isBlock)
        return;
    QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, 1, rows_in_page, QString("SELECT * FROM " + table_name), db_name, table_name);
    future.waitForFinished();
    pages[1].model->setQuery( future.takeResult() );
    fillModels(PageName::All, 1);
    cur_page = 1;
    cur_page_model = pages[1].model.get();

    setPagesCount();
    emitCurrentPage();
    emitCurrentPageInfo();
}

void Tab::changePage(int index_page, QObject* sender)
{
    if(isBlock)
        return;
    cur_page = index_page;
    PageName page;
    if(sender)
        page = states[sender];
    else
        page = PageName::All;

    pages_actions[ static_cast<int>(page) ](index_page);

    int center = static_cast<int>(PageName::Center);
    pages[center].future.waitForFinished();

    cur_page_model = pages[center].model.get();
    view->setModel( cur_page_model );

    emitCurrentPage();
}

void Tab::addSender(QObject* sender, PageName state) { states.insert(sender, state); }

void Tab::blockSignalsSlots(bool b)
{
    isBlock = b;
    this->blockSignals(b);
}

void Tab::selectSearchedIndex(int row, int column)
{
    pages[1].future.waitForFinished();
    view->selectionModel()->select(
        cur_page_model->index(row, column),  QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
    );
}

void Tab::emitPagesCount() { emit pagesCount(QString::number(max_page) ) ; }
void Tab::emitCurrentPage() { emit isPageChange(cur_page); }
void Tab::emitCurrentPageInfo() { emit preparedModel(*cur_page_model); }
