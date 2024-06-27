#ifndef TAB_H
#define TAB_H

#include <QObject>
#include <QSqlQueryModel>
#include <QFuture>
#include <QHash>
#include <functional>
#include <QTableView>

#include "filterwidget.h"

struct PageInfo
{
    QPointer<QSqlQueryModel> model;
    QFuture<void> future;
    PageInfo() : model{ new QSqlQueryModel } { }
};

class Tab : public QObject
{
    Q_OBJECT

public:
    enum class PageName{ Left, Center, Right, All};
private:
    QVector<PageInfo> pages;
    const QWidget* tab_wgt;
    QString table_name;
    QString db_name;
    QTableView* view;
    int rows_in_page;
    QSqlQueryModel* cur_page_model;
    int max_page;
    int cur_page;
    bool isBlock = false;

    void swapPages(PageName first, PageName second ) { std::swap( pages[static_cast<int>(first)], pages[static_cast<int>(second)] ); }
    void fillModels(PageName page, int page_index);
    QString getFiltredQuery(const QVector<Info>& info);
    void setPagesCount();
public:
    explicit Tab(QObject *parent, const QWidget* wgt, const QString &table_name, const QString& db_name, int rows_in_page );
    const QSqlQueryModel* getCurrentModel() const { return cur_page_model; }
    const QTableView* getModeliew() const { return view; }
    int getMaxPage() const { return max_page; }
public slots:
    void changePage(int index_page, QObject* sender = nullptr);
    void addSender(QObject* sender, PageName state);
    void blockSignalsSlots(bool b);
    void setFilters(const QVector<Info>& info);
    void setDefaultPage();
    void emitPagesCount();
    void emitCurrentPage();
    void emitCurrentPageInfo();
    void selectSearchedIndex(int row, int column);
signals:
    void isLeftPagePrepared(bool);
    void isRightPagePrepared(bool);
    void isPageChange(int page);
    void pagesCount(QString count);
    void preparedModel(const QSqlQueryModel &model);
    void currentPage();
private:
    QHash<QObject*, PageName> states;
    static constexpr int pages_cnt = 4;
    const std::function<void(int)> pages_actions[pages_cnt] =
    {
        [this](int page){ std::swap( pages[1], pages[2] ); std::swap( pages[0],pages[1] ); fillModels(PageName::Left, page); },
        [this](int page){ fillModels(PageName::All, page); },
        [this](int page){ std::swap( pages[1], pages[0] ); std::swap(pages[2],pages[1]);  fillModels(PageName::Right, page);},
        [this](int page){ fillModels(PageName::All, page); },
    };
};

#endif // TAB_H
