#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLayout>
#include <QTableView>
#include <QSqlTableModel>
#include <QSqlResult>
#include <QPushButton>
#include <QVector>
#include <QSqlQuery>
#include <QPointer>
#include <QtConcurrent>
#include <mutex>

#include "filterwidget.h"
#include "tab.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QTabWidget* windows;
    Tab* cur_tab;
    FilterWidget* filter_wgt;
    FilterWidget* search_wgt;
    int rows_in_page;
    QString db_name;
    QString cur_table;

    std::mutex tabes_mutex;
    QVector<QPointer<Tab>> tabes;

    QPair<int, int> getSearchedItemIndex(const QVector<Info>& info);
    bool isRightRow(const QSqlRecord &row, const QVector<Info>& info);
    QPushButton* addBtn(QLayout* layout, const QString &object_name, const QString &style_sheet, const QString &view);
    QWidget* addTab(const QString& obj_name, const QString& tab_name, const QString& table_name);

    void connectTabWithChangePageBtns(Tab* tab);
    void createChangePageWgts();
    void createMenu();
    void createFilter();
    void createSearch();
    void createToolBar();
    void search(const QVector<Info>& info);
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void selectSearchedIndex(int row, int column);
signals:
    void preparedModel(const QSqlQueryModel& model);
    void pagesCount(const QString& count);
    void isPageChange(int page_index);
    void isEnableSwitchingBtns(bool block);
private slots:
    void setPageIndex(int page_index);
    void enableSwitchingBtns(bool block);
};

#endif // MAINWINDOW_H
