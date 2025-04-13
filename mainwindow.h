#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_dbNameButton_clicked();

    void on_execQuery_clicked();

    void showDatabseNames();

    void on_dbOpenButton_clicked();

    void on_showColumnButton_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QSqlDatabase dbInit;
};
#endif // MAINWINDOW_H
