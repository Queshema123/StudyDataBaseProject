#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filterwidget.h"
#include "db_functions.h"

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
#include <QCheckBox>
#include <QStatusBar>

#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlIndex>

#include <QtConcurrent>
#include <QFuture>
#include <thread>
#include <memory>
#include <algorithm>

void MainWindow::enableSwitchingBtns(bool block)
{
    this->findChild<QPushButton*>("prevPageBtn")->setEnabled(block);
    this->findChild<QPushButton*>("nextPageBtn")->setEnabled(block);
    this->findChild<QLineEdit*>("CurrentPageIndexLineEdit")->setEnabled(block);
}

void MainWindow::setPageIndex(int page_index)
{
    this->findChild<QLineEdit*>("CurrentPageIndexLineEdit")->setText(QString::number(page_index));
}

bool MainWindow::isRightRow(const QSqlRecord &row, const QVector<Info>& info)
{
    for(int i = 0; i < info.size(); ++i)
    {
        if( row.value(info[i].column_id).toString() != info[i].value )
            return false;
    }
    return true;
}

QPair<int, int> MainWindow::getSearchedItemIndex(const QVector<Info>& info)
{// Не может знать о других записях по скольку хранит только 20, добавить поток, который вернет модель со всеми строками (без LIMIT OFFSET)
    QString sqlQ = cur_tab->getCurrentModel()->query().lastQuery();
    sqlQ = sqlQ.mid(0, sqlQ.indexOf("LIMIT"));

    auto get_query = [sqlQ, this]() ->QSqlQuery
    {
        return QSqlQuery(sqlQ, getDB(db_name));
    };
    QFuture<QSqlQuery> future = QtConcurrent::run( get_query );
    future.waitForFinished();
    QSqlQuery full_record =  future.takeResult();
    int row = 0;
    while(full_record.next())
    {
        if( isRightRow(full_record.record(), info) )
            return QPair<int, int>( row, 0 );
        ++row;
    }
    return QPair<int, int>(-1, -1);
}

QPushButton* MainWindow::addBtn(QLayout* layout, const QString &object_name, const QString &style_sheet, const QString &view)
{
    QPushButton* btn = new QPushButton(view);
    btn->setObjectName(object_name);
    btn->setStyleSheet(style_sheet);
    layout->addWidget(btn);
    return btn;
}

void MainWindow::createMenu()
{
    QMenuBar* menu = this->menuBar();
    QMenu* application = menu->addMenu("Application");
    QAction* user_table = new QAction("USER");
    QAction* departament_table = new QAction("DEPARTAMENTS");
    application->addAction(user_table);
    application->addAction(departament_table);

    QMenu* administration_action = menu->addMenu("Administration");
    QAction* exit_action = menu->addAction("Exit");
    QObject::connect(exit_action,  &QAction::triggered, this, &MainWindow::close);
    QObject::connect(exit_action,  &QAction::triggered, filter_wgt, &FilterWidget::deleteLater);
    QObject::connect(exit_action,  &QAction::triggered, search_wgt, &FilterWidget::deleteLater);
    QObject::connect(user_table, &QAction::triggered, this, [this](bool)
    {
        std::lock_guard<std::mutex> lock(tabes_mutex);
        windows->setCurrentWidget( addTab("UserTab", "User", "USER") );
        tabes.last()->changePage(1);
        cur_tab = tabes.last().get();
        emit cur_tab->preparedModel( *cur_tab->getCurrentModel() );
    }
    );
    QObject::connect(departament_table, &QAction::triggered, this, [this](bool)
    {
        std::lock_guard<std::mutex> lock(tabes_mutex);
        windows->setCurrentWidget( addTab("DepartamentTab", "Departament", "DEPARTAMENTS") );
        tabes.last()->changePage(1);
        cur_tab = tabes.last().get();
        emit cur_tab->preparedModel( *cur_tab->getCurrentModel() );
    }
    );
}

void MainWindow::selectSearchedIndex(int row, int column)
{
    cur_tab->getModeliew()->selectionModel()->select(
        cur_tab->getCurrentModel()->index(row, column),  QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
    );
}

void MainWindow::search(const QVector<Info>& info)
{
    QFuture<QPair<int, int>> future = QtConcurrent::run(getSearchedItemIndex, this, info);
    future.waitForFinished();
    // Находим остаток от деления, поскольку номер строки найденного индекса соответствует индексу для модели, которая содержит все записи из бд
    // row() - номер строки в моделе со всеми записями, row() % rows_in_page - номер строки в моделе с 25 записями
    this->selectSearchedIndex(  future.result().first % rows_in_page, future.result().second );
    int page_index = (future.result().first > rows_in_page) ? std::ceil( (double)future.result().first / rows_in_page) : 1; // int на int делится поэтому пришлось кастить к double для округления
    cur_tab->changePage(page_index);
}

void MainWindow::createFilter()
{
    QToolBar* toolbar = this->findChild<QToolBar*>();
    filter_wgt = new FilterWidget;
    filter_wgt->setObjectName("FilterWidget");
    QPushButton* filter_btn = new QPushButton("Filters");
    filter_btn->setObjectName("FilterBtn");
    toolbar->addWidget(filter_btn);

    QObject::connect(this,       &MainWindow::preparedModel,         filter_wgt, &FilterWidget::updateColumns);
    QObject::connect(filter_btn, &QPushButton::clicked,              filter_wgt, &FilterWidget::show);
}

void MainWindow::createSearch()
{
    QToolBar* toolbar = this->findChild<QToolBar*>();

    QPushButton* search_btn = new QPushButton("Search");
    search_btn->setObjectName("SearchBtn");
    toolbar->addWidget(search_btn);

    search_wgt = new FilterWidget;
    search_wgt->setObjectName("SearchWidget");

    QObject::connect(search_btn, &QPushButton::clicked,        search_wgt, &QWidget::show);
    QObject::connect(search_wgt, &FilterWidget::submitFilters, this, &MainWindow::search );
    QObject::connect(this, &MainWindow::preparedModel, search_wgt, &FilterWidget::updateColumns);
}

void MainWindow::createChangePageWgts()
{
    QToolBar* toolbar = this->findChild<QToolBar*>("MainToolBar");
    QLineEdit* cur_page = new QLineEdit("1");
    QLabel* pages_cnt = new QLabel;

    pages_cnt->setObjectName("PagesCountLabel");
    cur_page->setObjectName("CurrentPageIndexLineEdit");
    cur_page->setMaximumWidth(50);
    pages_cnt->setMaximumWidth(25);

    QHBoxLayout* btns_layout = new QHBoxLayout;
    addBtn(btns_layout, "prevPageBtn", "width: 50;", "prev")->setMaximumWidth(50);
    btns_layout->addWidget(cur_page);
    btns_layout->addWidget(pages_cnt);
    addBtn(btns_layout, "nextPageBtn", "width: 50;", "next")->setMaximumWidth(50);

    btns_layout->setAlignment(Qt::AlignLeft);

    QWidget* btns_wgt = new QWidget;
    btns_wgt->setLayout(btns_layout);

    toolbar->addWidget( btns_wgt );

    QObject::connect(this, &MainWindow::isEnableSwitchingBtns, this, &MainWindow::enableSwitchingBtns);
    QObject::connect(this, &MainWindow::pagesCount, pages_cnt, &QLabel::setText);
    QObject::connect(this, &MainWindow::isPageChange, this, &MainWindow::setPageIndex);
    QObject::connect(this, &MainWindow::pagesCount, cur_page, [cur_page, this](const QString &max_page)
        {
            cur_page->setValidator(nullptr);
            cur_page->setText("1");
            cur_page->setValidator( new QIntValidator(1, max_page.toInt(), this) );
        }
    );
}

void MainWindow::createToolBar()
{
    QToolBar* toolbar = new QToolBar;
    toolbar->setMinimumHeight(50);
    toolbar->setObjectName("MainToolBar");
    this->addToolBar(toolbar);
    createFilter();
    createSearch();
    createChangePageWgts();
}

void MainWindow::connectTabWithChangePageBtns(Tab* tab)
{
    QLineEdit* cur_page_index_line = this->findChild<QLineEdit*>("CurrentPageIndexLineEdit");
    QPushButton* prev_btn = this->findChild<QPushButton*>("prevPageBtn");
    QPushButton* next_btn = this->findChild<QPushButton*>("nextPageBtn");

    auto valid = [cur_page_index_line](int index) -> bool
    {
        int pos = 0;
        QString index_page = QString::number(index);
        if(cur_page_index_line->validator()->validate(index_page, pos) != QValidator::Acceptable)
            return false;
        return true;
    };

    tab->addSender(prev_btn, Tab::PageName::Left);
    tab->addSender(next_btn, Tab::PageName::Right);
    tab->addSender(cur_page_index_line, Tab::PageName::All);

    QObject::connect(prev_btn, &QPushButton::clicked, tab,  [this, tab, valid, cur_page_index_line]()
        {
            int page_index = cur_page_index_line->text().toInt() - 1;
            // Валидация номера страницы
            if( !valid(page_index))
                return;
            emit isEnableSwitchingBtns(false); // блокируются кнопки
            cur_page_index_line->setText( QString::number(page_index) );
            tab->changePage( page_index, QObject::sender() );
            emit isEnableSwitchingBtns(true); // разблокируем
        },
        Qt::QueuedConnection
        );
    QObject::connect(next_btn, &QPushButton::clicked,       this,   [this, tab, valid, cur_page_index_line]()
        {
            int page_index = cur_page_index_line->text().toInt() + 1;
            // Валидация номера страницы
            if( !valid(page_index))
                return;
            emit isEnableSwitchingBtns(false); // блокируются кнопки
            cur_page_index_line->setText( QString::number(page_index) );
            tab->changePage( page_index, QObject::sender() );
            emit isEnableSwitchingBtns(true); // разблокируем
        },
        Qt::QueuedConnection
        );
    QObject::connect(cur_page_index_line, &QLineEdit::returnPressed, this, [this, tab, cur_page_index_line]()
        {
            emit isEnableSwitchingBtns(false); // блокируются кнопки
            tab->changePage( cur_page_index_line->text().toInt(), QObject::sender() );
            emit isEnableSwitchingBtns(true); // разблокируем
        },
        Qt::QueuedConnection
        );
}

QWidget* MainWindow::addTab(const QString& obj_name, const QString& tab_name, const QString& table_name)
{
    QVBoxLayout* main_layout = new QVBoxLayout;

    QWidget* main_wgt = new QWidget;
    main_wgt->setObjectName(obj_name);
    main_wgt->setLayout(main_layout);

    QTableView* page = new QTableView;
    page->setObjectName("MainView");
    main_layout->addWidget(page);

    QStatusBar* status_bar = new QStatusBar;
    status_bar->setObjectName("PageStatusBar");
    main_layout->addWidget( status_bar );

    int index = tabes.length();
    windows->insertTab(index, main_wgt, tab_name);
    tabes.append( new Tab(nullptr, main_wgt, table_name, db_name, rows_in_page) );

    QObject::connect(filter_wgt, &FilterWidget::submitFilters,       tabes[index].get(), &Tab::setFilters);
    QObject::connect(filter_wgt, &FilterWidget::clearFiltersEffects, tabes[index].get(), &Tab::setDefaultPage);
    QObject::connect(tabes[index].get(), &Tab::pagesCount,    this, &MainWindow::pagesCount);
    QObject::connect(tabes[index].get(), &Tab::isPageChange,  this, &MainWindow::isPageChange);
    QObject::connect(tabes[index].get(), &Tab::preparedModel, filter_wgt, &FilterWidget::updateColumns);
    QObject::connect(tabes[index].get(), &Tab::preparedModel, search_wgt, &FilterWidget::updateColumns);
    QObject::connect(windows, &QTabWidget::currentChanged,    tabes[index].get(), [this, index, main_wgt](int idx)
        {
            tabes[index]->blockFilter( index != idx );
            cur_tab = tabes[idx].get();
            cur_tab->getPagesCount();
            cur_tab->getCurrentPage();
            emit tabes[idx]->preparedModel( *tabes[idx]->getCurrentModel() );
        }
    );
    QObject::connect(filter_wgt, &FilterWidget::submitFilters,       tabes[index].get(), [status_bar, this]() { status_bar->showMessage("Фильтр включен"); } );
    QObject::connect(filter_wgt, &FilterWidget::clearFiltersEffects, tabes[index].get(), [status_bar, this]() { status_bar->showMessage("Фильтр выключен"); } );

    connectTabWithChangePageBtns(tabes[index].get());

    return main_wgt;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), windows{new QTabWidget}, cur_tab{ nullptr },
    filter_wgt{nullptr}, search_wgt{nullptr}, rows_in_page{20}, db_name{"C:\\UserDataBase\\study_db.db"}, cur_table{"USER"}
{
    QHBoxLayout* main_layout = new QHBoxLayout;
    main_layout->setObjectName("MainLayout");
    QWidget* main_wgt = new QWidget;
    main_wgt->setObjectName("MainWidget");
    main_wgt->setLayout(main_layout);
    main_layout->addWidget(windows);
    this->setCentralWidget(main_wgt);

    createToolBar();
    createMenu();
}

MainWindow::~MainWindow() { }
