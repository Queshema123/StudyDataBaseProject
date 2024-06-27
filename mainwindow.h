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
#include "searchwidget.h"
#include "tab.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QTabWidget* windows;
    Tab* cur_tab;
    FilterWidget* filter_wgt;
    SearchWidget* search_wgt;
    int rows_in_page;
    QString db_name;
    QString cur_table;

    QVector<QPointer<Tab>> tabes;

    QPushButton* addBtn(QLayout* layout, const QString &object_name, const QString &style_sheet, const QString &view);
    QWidget* addTab(const QString& obj_name, const QString& tab_name, const QString& table_name);

    void connectTabWithChangePageBtns(Tab* tab);
    void createChangePageWgts();
    void createMenu();
    void createFilter();
    void createSearch();
    void createToolBar();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void selectRow(int row);
private slots:
    void setPageIndex(int page_index);
};

#endif // MAINWINDOW_H
