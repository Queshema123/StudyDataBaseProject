#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H
#include "filterwidget.h"
#include <QPointer>
#include <QSqlQueryModel>
#include <QObject>
#include <QWidget>
#include <QList>
#include <QThread>
#include <limits>
#include <functional>

int index_of_next_min(const QList<int>& list, int min);

class SearchWidget : public FilterWidget
{
    Q_OBJECT

    QList<int> searched_rows;

    QList<QThread*> search_pool;

    int f_thread_idx;
    int lst_thread_idx;
    int current_row;
    int current_row_index;
    bool isFind;
    bool isSorted;

    void clearCache();
    void fillPool(const QVector<Info>& info, const QString& query, const QString& db_name, int max_page);
    void addSwitchesWgts();
    void setNextElement(int index_if_sort = 0);
    void selectFirstRow(int row);
    void searchTaskEnd();
public:
    explicit SearchWidget(FilterWidget* parent = nullptr);
    bool isRightRow(const QSqlRecord &row, const QVector<Info>& info);
    void search(const QVector<Info>& info, const QString& query, const QString& db_name, int max_page);
signals:
    void submitedInfo(const QVector<Info>&);
    void searchedRow(int row);
    void searchedRowsCount(int);
    void changeElement(int);
public slots:
    void appendRow(int row);
};

class SearchTaskThread : public QThread
{
    Q_OBJECT

    SearchWidget* wgt;
    QString db_name;
    QString query;
    const QVector<Info>* info;
    int fpage, lpage, cur_row;
    void run() override;
public:
    SearchTaskThread(SearchWidget* wgt, const QVector<Info>& info, const QString& query, const QString& db_name, int fpage, int lpage);
signals:
    void findedRow(int);
};

#endif // SEARCHWIDGET_H
