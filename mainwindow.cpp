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
        if(info[i].operation == FilterWidget::CompareOperations::Unable)
            continue;
        if( !FilterWidget::compareValues(info[i].operation, row.value(info[i].column_id), info[i].value) )
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
    QObject::connect(exit_action,  &QAction::triggered, search_wgt, &SearchWidget::deleteLater);
    QObject::connect(user_table, &QAction::triggered, this, [this](bool)
    {
        windows->setCurrentWidget( addTab("UserTab", "User", "USER") );
        cur_tab = tabes.last().get();
        cur_tab->emitCurrentPage();
        cur_tab->emitPagesCount();
        cur_tab->emitCurrentPageInfo();
    }
    );
    QObject::connect(departament_table, &QAction::triggered, this, [this](bool)
    {
        windows->setCurrentWidget( addTab("DepartamentTab", "Departament", "DEPARTAMENTS") );
        cur_tab = tabes.last().get();
        cur_tab->emitCurrentPage();
        cur_tab->emitPagesCount();
        cur_tab->emitCurrentPageInfo();
    }
    );
}

void MainWindow::search(const QVector<Info>& info)
{
    QFuture<QPair<int, int>> future = QtConcurrent::run(getSearchedItemIndex, this, info);
    future.waitForFinished();
    // Находим остаток от деления, поскольку номер строки найденного индекса соответствует индексу для модели, которая содержит все записи из бд
    // row() - номер строки в моделе со всеми записями, row() % rows_in_page - номер строки в моделе с 25 записями
    int page_index = (future.result().first > rows_in_page) ? std::ceil( (double)future.result().first / rows_in_page) : 1; // int на int делится поэтому пришлось кастить к double для округления
    cur_tab->changePage(page_index);
    qDebug() << page_index << future.result();
    cur_tab->selectSearchedIndex(future.result().first % rows_in_page, future.result().second);
}

void MainWindow::createFilter()
{
    QToolBar* toolbar = this->findChild<QToolBar*>();
    filter_wgt = new FilterWidget;
    filter_wgt->setObjectName("FilterWidget");
    QPushButton* filter_btn = new QPushButton("Filters");
    filter_btn->setObjectName("FilterBtn");
    toolbar->addWidget(filter_btn);

    QObject::connect(filter_btn, &QPushButton::clicked,              filter_wgt, &FilterWidget::show);
}

void MainWindow::createSearch()
{
    QToolBar* toolbar = this->findChild<QToolBar*>();

    QPushButton* search_btn = new QPushButton("Search");
    search_btn->setObjectName("SearchBtn");
    toolbar->addWidget(search_btn);

    search_wgt = new SearchWidget;

    QObject::connect(search_btn, &QPushButton::clicked,       search_wgt, &SearchWidget::show);
    QObject::connect(search_wgt, &SearchWidget::submitFilters, this, &MainWindow::search );
}

void MainWindow::createChangePageWgts()
{
    QToolBar* toolbar = this->findChild<QToolBar*>("MainToolBar");
    QLineEdit* cur_page = new QLineEdit("1");
    QLabel* pages_cnt = new QLabel("1");

    pages_cnt->setObjectName("PagesCountLabel");
    cur_page->setObjectName("CurrentPageIndexLineEdit");
    cur_page->setMaximumWidth(50);
    pages_cnt->setMaximumWidth(25);

    QHBoxLayout* btns_layout = new QHBoxLayout;
    addBtn(btns_layout, "prevPageBtn", " ", "prev")->setMaximumWidth(50);
    btns_layout->addWidget(cur_page);
    btns_layout->addWidget(pages_cnt);
    addBtn(btns_layout, "nextPageBtn", " ", "next")->setMaximumWidth(50);

    btns_layout->setAlignment(Qt::AlignLeft);

    QWidget* btns_wgt = new QWidget;
    btns_wgt->setLayout(btns_layout);

    toolbar->addWidget( btns_wgt );

    QObject::connect(this, &MainWindow::isEnableSwitchingBtns, this, &MainWindow::enableSwitchingBtns);
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

    QObject::connect(prev_btn, &QPushButton::clicked, tab,  [this, tab, valid, cur_page_index_line, prev_btn]()
        {
            int page_index = cur_page_index_line->text().toInt() - 1;
            // Валидация номера страницы
            if( !valid(page_index))
                return;
            emit isEnableSwitchingBtns(false); // блокируются кнопки
            tab->changePage( page_index, prev_btn );
            emit isEnableSwitchingBtns(true); // разблокируем
        },
        Qt::QueuedConnection
        );
    QObject::connect(next_btn, &QPushButton::clicked,       tab,   [this, tab, valid, cur_page_index_line, next_btn]()
        {
            int page_index = cur_page_index_line->text().toInt() + 1;
            // Валидация номера страницы
            if( !valid(page_index))
                return;
            emit isEnableSwitchingBtns(false); // блокируются кнопки
            tab->changePage( page_index, next_btn );
            emit isEnableSwitchingBtns(true); // разблокируем
        },
        Qt::QueuedConnection
        );
    QObject::connect(cur_page_index_line, &QLineEdit::returnPressed, tab, [this, tab, cur_page_index_line]()
        {
            emit isEnableSwitchingBtns(false); // блокируются кнопки
            tab->changePage( cur_page_index_line->text().toInt(), cur_page_index_line );
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
    QPointer<Tab> tab = new Tab(nullptr, main_wgt, table_name, db_name, rows_in_page);

    QObject::connect(filter_wgt, &FilterWidget::submitFilters,       tab, &Tab::setFilters);
    QObject::connect(filter_wgt, &FilterWidget::clearFiltersEffects, tab, &Tab::setDefaultPage);
    QObject::connect(tab, &Tab::preparedModel, filter_wgt, &FilterWidget::updateColumns);
    QObject::connect(tab, &Tab::preparedModel, search_wgt, &SearchWidget::updateColumns);
    QObject::connect(windows, &QTabWidget::currentChanged,    tab, [this, index, main_wgt](int idx)
        {
            if(idx == index)
            {
                cur_tab = tabes[idx].get();
                cur_tab->blockSignalsSlots(false);
                cur_tab->emitPagesCount();
                cur_tab->emitCurrentPage();
                cur_tab->emitCurrentPageInfo();
            }
            else
                tabes[index]->blockSignalsSlots( true ); // Блокируем слоты для работы с фильтром в случае, если это не та страница
        }
    );
    QObject::connect(filter_wgt, &FilterWidget::submitFilters,       tab, [status_bar, this]() { status_bar->showMessage("Фильтр включен"); } );
    QObject::connect(filter_wgt, &FilterWidget::clearFiltersEffects, tab, [status_bar, this]() { status_bar->showMessage("Фильтр выключен"); } );
    QObject::connect(tab, &Tab::isPageChange, this, &MainWindow::setPageIndex);

    auto cur_page = this->findChild<QLineEdit*>("CurrentPageIndexLineEdit");
    auto max_page_lbl = this->findChild<QLabel*>("PagesCountLabel");

    QObject::connect( tab, &Tab::pagesCount, [this, cur_page, max_page_lbl](QString max_page)
    {
        cur_page->setValidator(nullptr);
        cur_page->setValidator( new QIntValidator(1, max_page.toInt() ) );
        max_page_lbl->setText(max_page);
    }
    );
    connectTabWithChangePageBtns(tab.get());
    tabes.append( std::move(tab) );
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
