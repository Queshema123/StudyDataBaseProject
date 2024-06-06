#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filterwidget.h"

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

#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>
#include <QSqlIndex>

#include <QtConcurrent>
#include <QFuture>
#include <thread>
#include <memory>

void MainWindow::changeTable(const QString &table_name)
{
    this->cur_table = table_name;
    std::thread t(setDefaultPage, this);
    t.join();
}

void MainWindow::enableSwitchingBtns(bool block)
{
    this->findChild<QPushButton*>("prevPageBtn")->setEnabled(block);
    this->findChild<QPushButton*>("nextPageBtn")->setEnabled(block);
    this->findChild<QLineEdit*>("CurrentPageIndexLineEdit")->setEnabled(block);
}

void MainWindow::setPagesCount()
{
    auto count = [this](){
        QString query = page_model->query().lastQuery();
        query.replace("*", "COUNT(*)");
        QSqlQuery sqlQ(query, getDB(db_name));
        sqlQ.next();
        int rows = sqlQ.value(0).toInt();
        int pages = ( ( rows - 1) / rows_in_page ) + 1;
        emit pagesCount( QString::number(pages) );
    };

    std::thread pages_cnt_thread(count);
    pages_cnt_thread.detach();
}

void MainWindow::setPageIndex(int page_index)
{
    this->findChild<QLineEdit*>("CurrentPageIndexLineEdit")->setText(QString::number(page_index));
}

QSqlDatabase MainWindow::getDB(const QString& db_name)
{
    QString connect_name = QThread::currentThread()->objectName();
    if(QSqlDatabase::contains(connect_name))
    {
        return QSqlDatabase::database(connect_name);
    }
    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_name);
    if(!db.isValid() || !db.open())
        qDebug() << "DB connection creation error!";
    return db;
}

QSqlQuery MainWindow::getCurrentPageData(int page_index, int rows_in_page, const QString &query)
{
    QSqlDatabase db = getDB(db_name);
    QString f_row = QString::number(rows_in_page * (page_index - 1) );
    QString new_query = query;
    if(!query.contains("LIMIT"))
        new_query += + " ORDER BY " + db.primaryIndex( cur_table ).fieldName(0) + " ASC LIMIT " + QString::number(rows_in_page) + " OFFSET " + f_row;
    else
    {
        new_query = query.mid( 0, query.indexOf("LIMIT") ) + " LIMIT " + QString::number(rows_in_page) + " OFFSET " + f_row;
    }
    QSqlQuery sqlQ(new_query, db);
    return sqlQ;
}

bool MainWindow::isRightRow(const QSqlRecord &row, const QList<int> &columns_indexes, const QList<QString> &finding_values)
{
    for(int i = 0; i < columns_indexes.size(); ++i)
    {
        if( row.value(columns_indexes[i]).toString() != finding_values[i] )
            return false;
    }
    return true;
}

QPair<int, int> MainWindow::getSearchedItemIndex(const QList<int> &columns_indexes, const QList<QString> &finding_values)
{// Не может знать о других записях по скольку хранит только 20, добавить поток, который вернет модель со всеми строками (без LIMIT OFFSET)
    QString sqlQ = page_model->query().lastQuery();
    sqlQ = sqlQ.mid(0, sqlQ.indexOf("LIMIT"));

    auto get_query = [sqlQ, this]() ->QSqlQuery
    {
        return QSqlQuery(sqlQ, this->getDB(db_name));
    };
    QFuture<QSqlQuery> future = QtConcurrent::run( get_query );
    if(future.isValid())
    {
        QSqlQuery full_record =  future.takeResult();
        int row = 0;
        while(full_record.next())
        {
            if( isRightRow(full_record.record(), columns_indexes, finding_values) )
                return QPair<int, int>( row, 0 );
            ++row;
        }
    }
    return QPair<int, int>();
}

void MainWindow::prepareModel()
{
    page_model = new QSqlQueryModel;
    page_model->setQuery(getCurrentPageData(1, rows_in_page));
}

void MainWindow::hideColumns(QTableView* view)
{
    int columns = page_model->record().count();
    for(int i = 0; i < columns; ++i)
    {
        view->hideColumn(i);
    }
}

void MainWindow::addTableView(const QString& object_name, QLayout* layout, const QVector<QString>& column_to_show)
{
    QTableView* view = new QTableView;
    view->setObjectName(object_name);
    view->setModel(page_model);
    layout->addWidget(view);

    if(column_to_show.size() > 0)
        hideColumns(view);

    for(const auto& column : column_to_show)
    {
        view->showColumn( page_model->record().indexOf(column) );
    }
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
    QObject::connect(user_table, &QAction::triggered, this, [this](bool) { changeTable("USER"); } );
    QObject::connect(departament_table, &QAction::triggered, this, [this](bool) { changeTable("DEPARTAMENTS"); } );
}

void MainWindow::getPreparedModel()
{
    emit preparedModel(*page_model);
}

void MainWindow::selectSearchedIndex(int row, int column)
{
    QTableView* page_view = windows->currentWidget()->findChild<QTableView*>("MainPageView");
    page_view->selectionModel()->select(
        page_model->index(row, column),  QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
    );
}

void MainWindow::fillSearchedInfo(QList<int> &columns_indexes, QList<QString> &values_to_find, QObject* parent_wgt)
{
    // ParentWgt -> QWidget -> QLabel & QLineEdit
    const QList<QWidget*> wgts = qobject_cast<QWidget*>( parent_wgt )->findChildren<QWidget*>();
    for(auto wgt : wgts)
    {
        if(QString("QWidget") != wgt->metaObject()->className())
            continue;
        if(!wgt->findChild<QCheckBox*>()->isChecked())
            continue;
        values_to_find.append( wgt->findChild<QLineEdit*>()->text() );
        columns_indexes.append( page_model->record().indexOf( wgt->findChild<QLabel*>()->objectName() ) );
    }
}

void MainWindow::search()
{
    QList<QString> values_to_find;
    QList<int> columns_indexes;
    fillSearchedInfo(columns_indexes, values_to_find, QObject::sender()->parent() );
    if(values_to_find.size() == 0)
        return;

    QFuture<QPair<int, int>> future = QtConcurrent::run(getSearchedItemIndex, this, columns_indexes, values_to_find);
    if(future.isValid())
    {
        QObject::connect(this, &MainWindow::isPageChange, this,
            [this, future](int)
            {
                // Находим остаток от деления, поскольку номер строки найденного индекса соответствует индексу для модели, которая содержит все записи из бд
                // row() - номер строки в моделе со всеми записями, row() % rows_in_page - номер строки в моделе с 25 записями
                this->selectSearchedIndex(  future.result().first % rows_in_page, future.result().second );
            },
        Qt::QueuedConnection);
        int page_index = (future.result().first > rows_in_page) ? std::ceil( (double)future.result().first / rows_in_page) : 1; // int на int делится поэтому пришлось кастить к double для округления
        changePage( page_index );
    }
}

QString MainWindow::getFiltredQuery(const QMap<int, FilterWidget::CompareOperations> &operations, const QMap<int, QString> &values)
{
    QString query{"SELECT * FROM " + cur_table + " "};
    if(operations.size() == 0)
        return query;
    query += "WHERE ";
    for( auto[key, value] : operations.asKeyValueRange() )
    {
        query += filter_wgt->getSQLCondition(value, values[key], page_model->record().fieldName(key)) + " AND ";
    }
    query.chop(5); // Удаляем лишний AND в конце
    return query;
}

void MainWindow::setFilters(const QMap<int, FilterWidget::CompareOperations> &operations, const QMap<int, QString> &values)
{
    QString query = getFiltredQuery(operations, values);
    QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, this, 1, rows_in_page, query);
    if(future.isValid())
    {
        page_model->setQuery( future.takeResult() );
        changePage(1);
        setPagesCount();
    }
}

void MainWindow::setDefaultPage()
{
    QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, this, 1, rows_in_page, QString("SELECT * FROM " + cur_table));
    if(future.isValid())
    {
        page_model->setQuery( future.takeResult() );
        changePage(1);
        setPagesCount();
        emit isPageChange(1);
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
    QTableView* view = this->findChild<QTableView*>("MainPageView");
    view->setModel(page_model);

    QObject::connect(filter_wgt, &FilterWidget::submitFilters,       this, &MainWindow::setFilters  );
    QObject::connect(filter_wgt, &FilterWidget::clearFiltersEffects, this, &MainWindow::setDefaultPage);
    QObject::connect(this,       &MainWindow::preparedModel, filter_wgt, &FilterWidget::updateColumns);
    QObject::connect(filter_btn, SIGNAL(clicked(bool)),                 this,       SLOT(getPreparedModel()));
    QObject::connect(filter_btn, &QPushButton::clicked,                 filter_wgt, &FilterWidget::show);
    QObject::connect(this, &QMainWindow::destroyed, filter_wgt, &FilterWidget::deleteLater);
}

void MainWindow::addLineToSearch(QLayout* layout, const QString& field_name_to_search)
{
    QWidget* input_data_line_wgt = new QWidget;
    QHBoxLayout* input_data_layout = new QHBoxLayout;
    input_data_line_wgt->setLayout(input_data_layout);

    QLabel* column_name_lbl = new QLabel(field_name_to_search);
    column_name_lbl->setObjectName(field_name_to_search);
    input_data_layout->addWidget(column_name_lbl, 0, Qt::AlignLeft);

    QLineEdit* input_search_data = new QLineEdit;
    input_data_layout->addWidget(input_search_data, 0, Qt::AlignCenter);

    QCheckBox* box = new QCheckBox;
    input_data_layout->addWidget(box, 0, Qt::AlignRight);
    QLabel* lbl = new QLabel("Включить в поиск");
    input_data_layout->addWidget(lbl);

    QObject::connect(input_search_data, &QLineEdit::editingFinished, box, [box, input_search_data](){ box->setChecked(input_search_data->text().length() > 0); } );

    layout->addWidget(input_data_line_wgt);
}

void MainWindow::createSearch()
{
    QToolBar* toolbar = this->findChild<QToolBar*>();

    QPushButton* search_btn = new QPushButton("Search");
    search_btn->setObjectName("SearchBtn");
    toolbar->addWidget(search_btn);

    QWidget* search_input_data_wgt = new QWidget;
    search_input_data_wgt->setObjectName("SearchInputDataWidget");
    QVBoxLayout* search_data_layout = new QVBoxLayout;
    search_input_data_wgt->setLayout(search_data_layout);

    QPushButton* find_btn = new QPushButton("Find");
    search_btn->setObjectName("FindBtn");
    search_data_layout->addWidget(find_btn);

    for(int i = 0; i < page_model->columnCount(); ++i)
    {
        addLineToSearch(search_data_layout, page_model->record().fieldName(i));
    }

    QObject::connect(search_btn, &QPushButton::clicked, search_input_data_wgt, &QWidget::show);
    QObject::connect(find_btn,   &QPushButton::clicked, this, &MainWindow::search );
    QObject::connect(this,       &QMainWindow::destroyed, search_input_data_wgt, &QWidget::deleteLater);
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
    page->setModel(page_model);

    QLineEdit* cur_page = new QLineEdit("1");
    QLabel* pages_cnt = new QLabel;

    pages_cnt->setObjectName("PagesCountLabel");
    cur_page->setObjectName("CurrentPageIndexLineEdit");
    cur_page->setMaximumWidth(50);
    pages_cnt->setMaximumWidth(50);

    QHBoxLayout* btns_layout = new QHBoxLayout;
    addBtn(btns_layout, "prevPageBtn", "width: 50;", "prev");
    addBtn(btns_layout, "nextPageBtn", "width: 50;", "next");

    btns_layout->addWidget(cur_page);
    btns_layout->addWidget(pages_cnt);
    btns_layout->setAlignment(Qt::AlignLeft);

    wgt_layout->addLayout(btns_layout);

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

void MainWindow::changePage(int index_page)
{
    QFuture<QSqlQuery> future = QtConcurrent::run(getCurrentPageData, this, index_page, rows_in_page, page_model->query().lastQuery());
    if(future.isValid())
    {
        page_model->setQuery( future.takeResult() );
        emit isPageChange( index_page );
    }
}

void MainWindow::connectPagesThreads()
{
    QLineEdit* cur_page_index = this->findChild<QLineEdit*>("CurrentPageIndexLineEdit");
    QPushButton* prev_btn = this->findChild<QPushButton*>("prevPageBtn");
    QPushButton* next_btn = this->findChild<QPushButton*>("nextPageBtn");
    QObject::connect(this, &MainWindow::isPageChange, this, [this](){ emit isEnableSwitchingBtns(true);} );

    auto change_page_func = [this, cur_page_index]()
    {
        emit isEnableSwitchingBtns(false); // Здесь блокируются кнопки
        int page_idx = cur_page_index->text().toInt();
        changePage(page_idx);
    };

    QObject::connect(prev_btn, &QPushButton::clicked,       this,   [this, change_page_func, cur_page_index]()
        {
            int left = static_cast<int>(PageName::Left);
            QString page_index =  QString::number( cur_page_index->text().toInt() - 1 ) ;
            int pos = 0;
            if(cur_page_index->validator()->validate(page_index, pos) != QValidator::Acceptable)
                return;
            cur_page_index->setText( page_index );
            pages_threads[left] = std::make_unique<std::thread>( change_page_func);
            pages_threads[left].release()->detach();
        },
        Qt::QueuedConnection
    );
    QObject::connect(next_btn, &QPushButton::clicked,       this,   [this, change_page_func, cur_page_index]()
        {
            int right = static_cast<int>(PageName::Right);
            QString page_index =  QString::number( cur_page_index->text().toInt() + 1 ) ;
            int pos = 0;
            if(cur_page_index->validator()->validate(page_index, pos) != QValidator::Acceptable)
                return;
            cur_page_index->setText( page_index );

            pages_threads[right] = std::make_unique<std::thread>( change_page_func);
            pages_threads[right].release()->detach();
        },
        Qt::QueuedConnection
    );
    QObject::connect(cur_page_index, &QLineEdit::returnPressed, this, [this, change_page_func, cur_page_index]()
        {
            int center = static_cast<int>(PageName::Center);
            pages_threads[center] = std::make_unique<std::thread>( change_page_func);
            pages_threads[center].release()->detach();
        },
        Qt::QueuedConnection
    );
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), windows{new QTabWidget}, page_model{nullptr},
    filter_wgt{nullptr}, rows_in_page{20}, db_name{"C:\\UserDataBase\\study_db.db"}, cur_table{"USER"}, pages_threads{static_cast<int>(PageName::All)}
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

    setPagesCount();
    connectPagesThreads();

    windows->setCurrentIndex( static_cast<int>(Tabes::Main) );
}

MainWindow::~MainWindow() { }
