#include "db_functions.h"
#include <QSqlIndex>

QSqlDatabase getDB(const QString& db_name)
{
    static std::atomic<int> id = 1;
    QString connect_name = QThread::currentThread()->objectName() + QString::number(++id);
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

QSqlQuery getCurrentPageData(int page_index, int rows_in_page, const QString &query, const QString& db_name, const QString& table_name)
{
    QSqlDatabase db = getDB(db_name);
    QString f_row = QString::number(rows_in_page * (page_index - 1) );
    QString new_query = query;
    if(!query.contains("LIMIT"))
        new_query += " ORDER BY " + db.primaryIndex( table_name ).fieldName(0) + " ASC LIMIT " + QString::number(rows_in_page) + " OFFSET " + f_row;
    else
    {
        new_query = query.mid( 0, query.indexOf("LIMIT") ) + " LIMIT " + QString::number(rows_in_page) + " OFFSET " + f_row;
    }
    QSqlQuery sqlQ(new_query, db);
    return sqlQ;
}
