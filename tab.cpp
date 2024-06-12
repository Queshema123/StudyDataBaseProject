#include "tab.h"
#include "db_functions.h"
#include <QtConcurrent>
#include <QTableView>
#include <QSqlRecord>
#include <QVBoxLayout>
#include "filterwidget.h"

Tab::Tab(QObject *parent, const QWidget* wgt, const QString &table_name, const QString& db_name, int rows_in_page)
    : QObject{parent}, pages(3), table_name(table_name), db_name{db_name}, tab_wgt{wgt}, view{nullptr}, rows_in_page{rows_in_page}, cur_page_model{nullptr}
{
    view = wgt->findChild<QTableView*>();
    QFuture<void> future = QtConcurrent::run(
        [this]()
        {
            int center = static_cast<int>(PageName::Center);
            pages[center].model->setQuery( getCurrentPageData(1, this->rows_in_page, "SELECT * FROM " + this->table_name, this->db_name, this->table_name) );
            fillModels(PageName::Right, 2);
            cur_page_model = pages[center].model.get();
            setPagesCount();
        }
    );
    future.waitForFinished();
}

Tab::Tab(Tab&& other) : pages{ std::move(other.pages) }, table_name{ std::move(other.table_name) }, db_name{ std::move(other.db_name) },
    rows_in_page{other.rows_in_page}
{
    this->tab_wgt = other.tab_wgt;
    this->view = other.view;
    this->cur_page_model = this->pages[1].model;
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
        query += "AND " + FilterWidget::getSQLCondition(info[i].operation, info[i].value, cur_page_model->record().fieldName(info[i].column_id)) + " ";
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
        getPagesCount();
    };

    std::thread pages_cnt_thread(count);
    pages_cnt_thread.detach();
}

void Tab::setFilters(const QVector<Info>& info)
{
    if(isblockFilter)
        return;
    QString query = getFiltredQuery(info);
    QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, 1, rows_in_page, query, db_name, table_name);
    future.waitForFinished();
    cur_page_model->setQuery( future.takeResult() );
    emit isPageChange(1);
    setPagesCount();
}

void Tab::setDefaultPage()
{
    if(isblockFilter)
        return;
    QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, 1, rows_in_page, QString("SELECT * FROM " + table_name), db_name, table_name);
    future.waitForFinished();
    cur_page_model->setQuery( future.takeResult() );
    setPagesCount();
    emit isPageChange(1);
}

void Tab::changePage(int index_page, QObject* sender)
{
    cur_page = index_page;
    PageName page;
    if(sender)
        page = states[sender];
    else
        page = PageName::All;
    pages_actions[ static_cast<int>(page) ]();

    int center = static_cast<int>(PageName::Center);
    pages[center].future.waitForFinished();

    cur_page_model = pages[center].model;
    view->setModel( cur_page_model);

    fillModels(page, index_page);
}

void Tab::getPagesCount()
{
    emit pagesCount( QString::number(max_page));
}

void Tab::getCurrentPage()
{
    emit isPageChange(cur_page);
}
