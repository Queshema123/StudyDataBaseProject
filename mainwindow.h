#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLayout>
#include <QTableView>
#include <QSqlTableModel>
#include <mutex>
#include "multisortfilterproxymodel.h"
#include "filterwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QTabWidget* windows;
    QSqlTableModel* model;
    FilterWidget* filter_wgt;
    // mutable std::mutex main_page_wgt_mutex;
    enum class Tabes {Main, Phone, Names};

    bool openDB(const QString& db_name);
    void prepareModel();
    void hideColumns(QTableView* view);
    void addTableView(const QString& object_name, QLayout* layout, const QVector<QString>& column_to_show = {});
    void createMenu();
    void createFilter();
    void createSearch();
    void createToolBar();
    void addWidgetsToMainTab(QWidget* wgt);
    void createTabes();
    void search();
    QWidget* createWidgetWithView(MultiSortFilterProxyModel* proxy_model);
    void addPages(int f_page_index, int lst_page_index, int f_row_index, int rows_in_page);
    void fillMainTab(const QString &table_name);
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void getPreparedModel();
signals:
    void preparedModel(const QSqlTableModel& model);
    void pagesCount(const QString& count);
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
