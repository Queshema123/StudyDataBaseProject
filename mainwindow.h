#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLayout>
#include <QTableView>
#include <QSqlTableModel>
#include <QSqlResult>
#include <QPushButton>
#include <QModelIndex>
#include "multisortfilterproxymodel.h"
#include "filterwidget.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT
    QTabWidget* windows;
    QSqlQueryModel* page_model;
    MultiSortFilterProxyModel* proxy_model;
    FilterWidget* filter_wgt;
    int rows_in_page;
    QString db_name;
    enum class Tabes {Main, Phone, Names};

    QSqlDatabase getDB(const QString& db_name);
    QSqlQuery getCurrentPageData(int page_index, int rows_in_page);
    QModelIndex getSearchedItemIndex(const QList<int> &columns_indexex, const QList<QString> &finding_values);
    bool isRightRow(const QSqlRecord &row, const QList<int> &columns_indexes, const QList<QString> &finding_values);

    void prepareModel();
    void hideColumns(QTableView* view);
    void addTableView(const QString& object_name, QLayout* layout, const QVector<QString>& column_to_show = {});
    QPushButton* addBtn(QLayout* layout, const QString &object_name, const QString &style_sheet);
    void addLineToSearch(QLayout* layout, const QString& field_name_to_search);
    void fillSearchedInfo(QList<int> &columns_indexes, QList<QString> &values_to_find, QObject* parent_wgt);
    void createMenu();
    void createFilter();
    void createSearch();
    void createToolBar();
    void addWidgetsToMainTab(QWidget* wgt);
    void createTabes();
    void search();
    void fillMainTab(const QString &table_name);
    void changePage(int index_page);
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void getPreparedModel();
    void selectSearchedIndex(int row, int column);
signals:
    void preparedModel(const QSqlQueryModel& model);
    void pagesCount(const QString& count);
    void createdFilter(MultiSortFilterProxyModel* filter);
    void currentPageData(const QSqlResult* result);
    void isPageChange();
};

#endif // MAINWINDOW_H
