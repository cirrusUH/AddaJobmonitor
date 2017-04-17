#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

#include "jal.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void displayMessage(QString s);
    void displayErrorMessage(QString s);

private slots:
    void on_listView_runningjobs_clicked(const QModelIndex &index);
    void on_actionAbout_triggered();
    void on_pushButton_refresh_clicked();
    void on_pushButton_release_clicked();
    void on_pushButton_hold_clicked();
    void on_pushButton_kill_clicked();
    void on_pushButton_saveplot_clicked();
    void on_pushButton_showlog_clicked();

    void on_comboBox_accounts_currentIndexChanged(const QString &arg1);

    void closeEvent(QCloseEvent*e);



private:
    Ui::MainWindow *ui;
    void logmessage(QString s);
    void refreshjobs();

    QString savejobidtext;
    QString currentMachine;
    QSharedPointer<JAL> jal;
    JAL*jalstdptr;
};

#endif // MAINWINDOW_H
