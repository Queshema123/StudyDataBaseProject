#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLayout>
#include <QTableView>
#include <QSqlTableModel>
#include <QSqlResult>
#include <QPushButton>
#include "filterwidget.h"
#include <memory>
#include <thread>
#include <vector>

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QTabWidget* windows;
    QSqlQueryModel* page_model;
    FilterWidget* filter_wgt;
    int rows_in_page;
    QString db_name;
    QString cur_table;
    enum class PageName{ Left, Center, Right, All};
    std::vector<std::unique_ptr<std::thread>> pages_threads;
    enum class Tabes {Main, Phone, Names};

    QSqlDatabase getDB(const QString& db_name);
    QSqlQuery getCurrentPageData(int page_index, int rows_in_page, const QString &query = "SELECT * FROM USER");
    QPair<int, int> getSearchedItemIndex(const QList<int> &columns_indexex, const QList<QString> &finding_values);
    QString getFiltredQuery(const QMap<int, FilterWidget::CompareOperations> &operations = {}, const QMap<int, QString> &values = {});
    bool isRightRow(const QSqlRecord &row, const QList<int> &columns_indexes, const QList<QString> &finding_values);
    QPushButton* addBtn(QLayout* layout, const QString &object_name, const QString &style_sheet, const QString &view);

    void prepareModel();
    void hideColumns(QTableView* view);
    void addTableView(const QString& object_name, QLayout* layout, const QVector<QString>& column_to_show = {});
    void addLineToSearch(QLayout* layout, const QString& field_name_to_search);
    void fillSearchedInfo(QList<int> &columns_indexes, QList<QString> &values_to_find, QObject* parent_wgt);
    void createMenu();
    void createFilter();
    void createSearch();
    void createToolBar();
    void addWidgetsToMainTab(QWidget* wgt);
    void createTabes();
    void search();
    void changePage(int index_page);
    void connectPagesThreads();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void getPreparedModel();
    void selectSearchedIndex(int row, int column);
    void setFilters(const QMap<int, FilterWidget::CompareOperations> &operations, const QMap<int, QString> &values);
    void setDefaultPage();
    void setPagesCount();
signals:
    void preparedModel(const QSqlQueryModel& model);
    void pagesCount(const QString& count);
    void isPageChange(int page_index);
    void isEnableSwitchingBtns(bool block);
private slots:
    void setPageIndex(int page_index);
    void enableSwitchingBtns(bool block);
    void changeTable(const QString &table_name);
};

#endif // MAINWINDOW_H
