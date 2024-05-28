#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filterwidget.h"
#include "multisortfilterproxymodel.h"

#include <QWidget>
#include <QLabel>
#include <QTableView>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QPushButton>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QStackedWidget>

#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlIndex>

#include <QtConcurrent>
#include <QFuture>

QSqlDatabase MainWindow::getDB(const QString& db_name)
{
    QString connect_name = QThread::currentThread()->objectName();
    if(QSqlDatabase::contains(connect_name))
    {
        return QSqlDatabase::database(connect_name);
    }
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE", connect_name);
    db.setDatabaseName(db_name);
    if(!db.isValid() || !db.open())
        qDebug() << "DB connection creation error!";
    return db;
}

QSqlQuery MainWindow::getCurrentPageData(int page_index, int rows_in_page)
{
    QSqlDatabase db = getDB(db_name);
    QString f_row = QString::number(page_index * rows_in_page +1);
    QString l_row = QString::number(f_row.toInt() + rows_in_page);
    QString query = "SELECT * FROM " + model->tableName() + " WHERE ID " + " BETWEEN " + f_row + " AND " + l_row + " LIMIT " + QString::number(rows_in_page);
    QSqlQuery sqlQ(query, db);
    return sqlQ;
}

QModelIndex MainWindow::getSearchedItemIndex(int column_index, const QString &finding_value)
{
    int i = 0;
    while(model->canFetchMore())
    {
        for(;i < model->rowCount(); ++i)
        {
            if(model->record(i).value(column_index).toString() == finding_value )
                return model->index(i, column_index);
        }
        model->fetchMore();
    }

    return QModelIndex();
}

void MainWindow::prepareModel()
{
    QSqlDatabase db = getDB(db_name);
    if(!db.open())
    {
        qDebug() << "Can't open DataBase connection";
        return;
    }
    model = new QSqlTableModel(this, db);
    model->setTable("USER");
    model->select();
    QSqlRecord r = model->record();
    for(int i = 0; i < r.count(); ++i)
    {
        model->setHeaderData(i, Qt::Horizontal, r.fieldName(i));
    }
}

void MainWindow::hideColumns(QTableView* view)
{
    int columns = model->record().count();
    for(int i = 0; i < columns; ++i)
    {
        view->hideColumn(i);
    }
}

void MainWindow::addTableView(const QString& object_name, QLayout* layout, const QVector<QString>& column_to_show)
{
    QTableView* view = new QTableView;
    view->setObjectName(object_name);
    view->setModel(model);
    layout->addWidget(view);

    if(column_to_show.size() > 0)
        hideColumns(view);

    for(const auto& column : column_to_show)
    {
        view->showColumn( model->record().indexOf(column) );
    }
}

QPushButton* MainWindow::addBtn(QLayout* layout, const QString &object_name, const QString &style_sheet)
{
    QPushButton* btn = new QPushButton(object_name);
    btn->setStyleSheet(style_sheet);
    layout->addWidget(btn);
    return btn;
}

void MainWindow::createMenu()
{
    QMenuBar* menu = this->menuBar();
    QMenu* administration_action = menu->addMenu("Administration");
    QAction* exit_action = menu->addAction("Exit");
    QObject::connect(exit_action,  SIGNAL(triggered(bool)), this, SLOT(close()));
}

void MainWindow::getPreparedModel()
{
    emit preparedModel(*model);
}

void MainWindow::selectSearchedIndex(int row, int column)
{
    QTableView* page_view = windows->currentWidget()->findChild<QTableView*>("MainPageView");
    page_view->selectionModel()->select(
        proxy_model->index(row, column),  QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
    );
}

void MainWindow::search()
{
    QString value_to_find{ QObject::sender()->parent()->findChild<QLineEdit*>()->text() };
    QString column_name{ QObject::sender()->parent()->findChild<QComboBox*>()->currentText() };
    int column_index = model->record().indexOf(column_name);
    QFuture<QModelIndex> future = QtConcurrent::run(getSearchedItemIndex, this, column_index, value_to_find);
    if(future.isValid())
    {
        qDebug() << future.result().row() % rows_in_page;
        QObject::connect(this, &MainWindow::isPageChange, this,
            [this, future]()
            {
                // Находим остаток от деления, поскольку номер строки найденного индекса соответствует индексу для модели, которая содержит все записи из бд
                // row() - номер строки в моделе со всеми записями, row() % rows_in_page - номер строки в моделе с 25 записями
                this->selectSearchedIndex(  future.result().row() % rows_in_page, future.result().column() );
            },
        Qt::QueuedConnection);
        changePage(static_cast<int>(future.result().row() / rows_in_page) );// Получаем номер страницы, на которой находится нужная строка
    }
}

void MainWindow::createFilter()
{
    QToolBar* toolbar = this->findChild<QToolBar*>();
    filter_wgt = new FilterWidget;
    filter_wgt->setObjectName("FilterWidget");
    QPushButton* filter_btn = new QPushButton("Filters");
    filter_btn->setObjectName("FilterBtn");
    toolbar->addWidget(filter_btn);

    proxy_model = new MultiSortFilterProxyModel(this);
    QObject::connect(filter_wgt, &FilterWidget::submitFilters, proxy_model, &MultiSortFilterProxyModel::addFilters);
    QTableView* view = this->findChild<QTableView*>("MainPageView");
    proxy_model->setSourceModel( page_model );
    view->setModel(proxy_model);

    QObject::connect(filter_wgt, &FilterWidget::isInAllPage, this, &MainWindow::filterSearchState);
    QObject::connect(filter_wgt, &FilterWidget::clearFiltersEffects, proxy_model, &MultiSortFilterProxyModel::clearFilters);
    QObject::connect(this,       SIGNAL(preparedModel(QSqlTableModel)), filter_wgt, SLOT(updateColumns(QSqlTableModel)));
    QObject::connect(filter_btn, SIGNAL(clicked(bool)),                 this,       SLOT(getPreparedModel()));
    QObject::connect(filter_btn, SIGNAL(clicked(bool)),                 filter_wgt, SLOT(show()));
}

void MainWindow::createSearch()
{
    QToolBar* toolbar = this->findChild<QToolBar*>();

    QPushButton* search_btn = new QPushButton("Search");
    search_btn->setObjectName("SearchBtn");
    toolbar->addWidget(search_btn);

    QWidget* search_input_data_wgt = new QWidget;
    search_input_data_wgt->setObjectName("SearchInputDataWidget");
    QHBoxLayout* search_layout = new QHBoxLayout;
    search_input_data_wgt->setLayout(search_layout);

    QComboBox* box = new QComboBox;
    for(int i = 0; i < model->columnCount(); ++i)
        box->addItem( model->record().fieldName(i) );

    search_layout->addWidget(box);
    QLineEdit* input_search_data = new QLineEdit;
    search_layout->addWidget(input_search_data);

    QPushButton* find_btn = new QPushButton("Find");
    search_btn->setObjectName("FindBtn");
    search_layout->addWidget(find_btn);

    QObject::connect(search_btn, &QPushButton::clicked, search_input_data_wgt, &QWidget::show);
    QObject::connect(find_btn,   &QPushButton::clicked, this, &MainWindow::search );
}

void MainWindow::createToolBar()
{
    QToolBar* toolbar = new QToolBar;
    toolbar->setMinimumHeight(50);
    this->addToolBar(toolbar);
    createFilter();
    createSearch();
}

void MainWindow::addWidgetsToMainTab(QWidget* wgt)
{
    QVBoxLayout* wgt_layout = qobject_cast<QVBoxLayout*>(wgt->layout());
    if(!wgt_layout)
    {
        qDebug() << "Can't add widgets to main tab";
        return;
    }
    QTableView* page = new QTableView;
    page->setObjectName("MainPageView");
    wgt_layout->addWidget(page);
    page_model = new QSqlQueryModel;
    page->setModel(page_model);

    QLineEdit* cur_page = new QLineEdit("0");
    QLabel* pages_cnt = new QLabel;

    pages_cnt->setObjectName("PagesCountLabel");
    cur_page->setObjectName("CurrentPageIndexLineEdit");
    cur_page->setMaximumWidth(50);
    pages_cnt->setMaximumWidth(50);

    QHBoxLayout* btns_layout = new QHBoxLayout;
    QPushButton* prev_btn = addBtn(btns_layout, "prev", "width: 50;");
    QPushButton* next_btn = addBtn(btns_layout, "next", "width: 50;");

    btns_layout->addWidget(cur_page);
    btns_layout->addWidget(pages_cnt);
    btns_layout->setAlignment(Qt::AlignLeft);

    wgt_layout->addLayout(btns_layout);

    QObject::connect(this, &MainWindow::pagesCount, pages_cnt, &QLabel::setText);
    QObject::connect(prev_btn, &QPushButton::clicked,       cur_page,   [=](){ changePage(cur_page->text().toInt() - 1); } );
    QObject::connect(next_btn, &QPushButton::clicked,       cur_page,   [=](){ changePage(cur_page->text().toInt() + 1); } );
    QObject::connect(cur_page, &QLineEdit::editingFinished, page,       [=](){ changePage(cur_page->text().toInt());     } );
}

void MainWindow::createTabes()
{
    QHBoxLayout* phones_layout = new QHBoxLayout;
    QHBoxLayout* names_layout  = new QHBoxLayout;
    QVBoxLayout* main_layout = new QVBoxLayout;
    QWidget* phones_wgt = new  QWidget;
    QWidget* names_wgt = new QWidget;
    QWidget* main_wgt = new QWidget;

    phones_wgt->setObjectName("PhonesPage");
    names_wgt->setObjectName("NamesPage");
    main_wgt->setObjectName("MainPage");

    phones_wgt->setLayout(phones_layout);
    names_wgt->setLayout(names_layout);
    main_wgt->setLayout(main_layout);

    addWidgetsToMainTab(main_wgt);

    windows->insertTab(static_cast<int>(Tabes::Names), names_wgt, "Names");
    windows->insertTab(static_cast<int>(Tabes::Phone), phones_wgt, "Phones");
    windows->insertTab(static_cast<int>(Tabes::Main), main_wgt, "Main");
}

void MainWindow::fillMainTab(const QString &table_name)
{
    QSqlQuery sqlQ("SELECT COUNT(*) FROM " + table_name, getDB(db_name));
    if(!sqlQ.next())
    {
        qDebug() << "Can't fill main page";
        return;
    }

    int rows = sqlQ.value(0).toInt();
    int pages = ( ( rows - 1) / rows_in_page ) + 1; // Деление с округлением вверх
    emit pagesCount( QString::number(pages) );
    page_model->setQuery(getCurrentPageData(0, rows_in_page));
}

void MainWindow::filterSearchState(bool in_all_page)
{
    if(in_all_page)
        proxy_model->setSourceModel(model);
    else
        proxy_model->setSourceModel(page_model);
}

void MainWindow::changePage(int index_page)
{
    QLineEdit* curr_page_line_edit = this->findChild<QLineEdit*>("CurrentPageIndexLineEdit");
    if(!curr_page_line_edit && curr_page_line_edit->text().toInt() == index_page)
        return;

    QLabel* page_cnt_lbl = this->findChild<QLabel*>("PagesCountLabel");
    int max_page = page_cnt_lbl->text().toInt();
    if(index_page >= 0 && index_page <= max_page)
    {
        QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, this, index_page, rows_in_page);
        if(future.isValid())
        {
            page_model->setQuery( future.takeResult() );
            curr_page_line_edit->setText(QString::number(index_page));
            emit isPageChange();
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), windows{new QTabWidget}, model{nullptr}, page_model{nullptr}, proxy_model{nullptr},
    filter_wgt{nullptr}, rows_in_page{20}, db_name{"C:\\UserDataBase\\study_db.db"}
{
    this->thread()->setObjectName("MainThread");
    QHBoxLayout* main_layout = new QHBoxLayout;
    main_layout->setObjectName("MainLayout");
    QWidget* main_wgt = new QWidget;
    main_wgt->setObjectName("MainWidget");
    main_wgt->setLayout(main_layout);
    main_layout->addWidget(windows);
    this->setCentralWidget(main_wgt);

    createTabes();
    prepareModel();

    addTableView("PhonesTableView", this->findChild<QWidget*>("PhonesPage")->layout(), {"PHONE_NUMBER"} );
    addTableView("NamesTableView",  this->findChild<QWidget*>("NamesPage")->layout(),  {"NAME"} );

    createMenu();
    createToolBar();

    fillMainTab("USER");

    windows->setCurrentIndex( static_cast<int>(Tabes::Main) );
}

MainWindow::~MainWindow() { }
