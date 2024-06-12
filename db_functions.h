#ifndef DB_FUNCTIONS_H
#define DB_FUNCTIONS_H
#include <QSqlDatabase>
#include <QThread>
#include <QSqlQuery>

QSqlDatabase getDB(const QString& db_name);
QSqlQuery getCurrentPageData(int page_index, int rows_in_page, const QString &query, const QString& db_name, const QString& table_name);

#endif // DB_FUNCTIONS_H
