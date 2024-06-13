#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H
#include "filterwidget.h"
#include <QPointer>
#include <QSqlQueryModel>
#include <QObject>
#include <QWidget>

class SearchWidget : public FilterWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(FilterWidget* parent = nullptr);
signals:
    void submitedInfo(const QVector<Info>&);
};

#endif // SEARCHWIDGET_H
