#include "mainwindow.h"

#include <QApplication>

// Добавить поле рождения, добавить роли пользователю, добавить числовое поле
// Добавить в меню вкладку приложение и поместить в него phone, name ..., выход, администрирование
// ToolBar на будущее
// На информацию с колонки устанавливается фильтр, и в рамках фильтра есть поиск
// Например: Фильтр на Фамилию и год рождения, и по этому результату есть поиск по имени(например)

// При использовании QFilterSortProxyModel длительность выполнения увеличивается с кол-ом записей в бд что влияет на производительность
// Но используя другой подход через создание копии текущей модели мы дублируем одни и те же данные, что также может привести к последствиям
// В каких случаях выбирать 1-е или 2-е

// Сменить StackedWidget на QTabWidget(Вкладки)
// Добавить возможность выбора операции сравнения в фильтр

// Добавить лимит на отображение кол-ва строк в странице, например, 20

// Отдельное поток, считающий кол-во страниц
// Отдельно 2 потока для прогрузки страниц, т.е человек нажал на 10 страницу, 9 и 11 прогружают два отдельных потока
// Добавить сообщение об окончании работы потока
// Блокировать кнопку переключения на другую страницу в случае если она обрабатывается потоком

// Исправить операции в фильтре, которые зависят от типа поля ( Начинается, заканчивается, содержит) +
// Сделать 3 потока, в каждом из них QSQLQUERY модель ( сделать через std::thread ) ( Обновлять модель новым запросом при применении фильтра +) +
// При движении вправо удаляем левую модель, при движении влево правую и смещаем данные (+-)
// Поиск по нескольким полям(Список всех полей) +
// Количество страниц в отдельном потоке +
// В фильтре отображаются все поля сразу, убрать добавление/удаление +

// Добавить возможность кастомизации FilterWidget для использования его объекта в качестве поиска +
// Добавить status_bar +
// Переместить кнопки переключения страниц в toolbar +
// Убрать очистку фильтра при его закрытии +
// Добавить закрытие FilterWidget'a при закрытии главного окна +
// Добавление вкладки(Tab) при выборе другой таблицы
// Добавить операцию пусто в FilterWidget +
// Добавить операцию выключить для того, чтобы данное поле не учитывалось при обработке +
// Убрать checkbox включить в поиск/фильтр +
// Добавить структуру, хранящую данные отправленные FilterWidget'ом +
// Протестировать работают ли сигналы на заблокированные слоты при множестве вкладок +
// Добавить запоминание фильтром введенных параметров, чтобы при смене вкладок отображалось сохраненное +

// Починить поиск, некорректно работает при повторном нажатии кнопки, также иногда не находит значения +
// Найти эффективный алгоритм поиска
// Добавить кэш для поиска, чтобы можно было нажимать на кнопку и переключаться на следующие  найденные значения +

// Блокировать кнопки пока страница не сформировалась( Убрать блокировку во время переключения ) +
// Изменить реализацию поиска, добавить разбиение страниц для разных потоков, например, 1 поток 1-е 100 стр и тд, Затем после первого найденного значения проверить не нашли ли потоки слева +
// Использовать QThreaPool, QThread +
// Сделать FilterWidget модальным окном +

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
