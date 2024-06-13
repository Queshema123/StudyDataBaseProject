#include "searchwidget.h"
#include <QVBoxLayout>
#include <QObject>

SearchWidget::SearchWidget(FilterWidget* parent): FilterWidget{parent}
{
    this->setWindowTitle("Search");
}

