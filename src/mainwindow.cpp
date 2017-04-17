#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <algorithm>
#include <sstream>

#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QComboBox>
#include <QDebug>
#include <QStringListModel>
#include <QSettings>
#include <QFileDialog>
#include <QScrollBar>

#include "helpabout.h"
#include "qssh.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , jalstdptr(nullptr)
{
    ui->setupUi(this);

    QSettings settings("ADDA-team","ADDAjobmanager");
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());
    ui->splitter_horizontal->restoreState(settings.value("splitterSizes_horizontal").toByteArray());
    ui->splitter_vertical->restoreState(settings.value("splitterSizes_vertical").toByteArray());

    ui->textBrowser_console->append("Initialized...");

#ifdef Q_OS_WIN
    // ui->pushButton_release->setEnabled(false);
    // ui->pushButton_hold->setEnabled(false);
#endif

    repaint();

    // windows is different...
    QDir::setCurrent( QCoreApplication::applicationDirPath() );
    // qDebug() << QCoreApplication::applicationDirPath();

    QFile addamachinesfile(QCoreApplication::applicationDirPath()+"/addamachines.txt");
    // qDebug() << QDir::currentPath() << " "<< addamachinesfile.fileName();

    // populate accounts dropdown
    QStringList accountsStringList;

    if (!addamachinesfile.open(QIODevice::ReadOnly))
    {
        QMessageBox::information(0, "Error opening file", "Error opening file " + addamachinesfile.fileName()
                                 +": "+addamachinesfile.errorString() +"\n"

                                 + "Please provide a file addamachines.txt containing account information\n"
                                 + "in the same directory as this binary (see README.md)." );
    }
    else
    {
        while(!addamachinesfile.atEnd())
        {
            accountsStringList.append(addamachinesfile.readLine());
        }
        addamachinesfile.close();
    }

    QComboBox *combobox_accounts = findChild<QComboBox*>("comboBox_accounts");
    Q_ASSERT(combobox_accounts);

    combobox_accounts->insertItems(0, accountsStringList);

    if( accountsStringList.length() > 0)
    {
        currentMachine = combobox_accounts->currentText();
        refreshjobs();
    }
    else
    {
        QMessageBox::information(0, "Empty addamachines.txt file", "No entries found in addamachines.txt, nothing will work.");
    }

#ifdef Q_OS_WIN
    ui->listView_runningjobs->setFont(QFont("Courier",9));
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::displayMessage(QString s)
{
    ui->textBrowser_console->append(s);
    ui->textBrowser_console->repaint();
    qApp->processEvents();

}

void MainWindow::displayErrorMessage(QString s)
{
    ui->textBrowser_console->append("<font color=\"red\">" + s + "</font>" );
    ui->textBrowser_console->repaint();
    qApp->processEvents();
}

void MainWindow::on_listView_runningjobs_clicked(const QModelIndex &index)
{
    static bool locked;
    if(locked)
    {
        return;
    }

    locked=true;

    // qDebug() << "MainWindow::on_listView_runningjobs_clicked(const QModelIndex &index=" << index.row();
    int i = index.row();

    if( jal->getIsRunningJob(i) == false)
    {
        return;
    }

    QCustomPlot*customPlot = findChild<QCustomPlot*>("qcustomplot_convergence");

    customPlot->clearGraphs();
    customPlot->clearItems();

    QCPItemText *textLabel = new QCPItemText(customPlot);
    textLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter);
    // textLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel->position->setType(QCPItemPosition::ptViewportRatio);
    textLabel->position->setCoords(0.5, 0.5);
    textLabel->setText("Retrieving convergence data...");
    textLabel->setFont(QFont(font().family(), 16)); // make font a bit larger

    customPlot->replot();
    customPlot->repaint();

    logmessage("Retrieving convergence data for jobid " + jal->getJobID(index.row()) +" which has working directory:\n cd  " + jal->getJobDirectory(index.row()) );
    Convergence conv = jal->getConvergence( i );
    customPlot->removeItem(textLabel);

    QString jobid = jal->getJobID( i);
    if(customPlot->plotLayout()->rowCount() == 1)
    {
        customPlot->plotLayout()->insertRow(0);
        customPlot->plotLayout()->addElement(0, 0, new QCPTextElement(customPlot, "Convergence of job " +  jobid));
    }
    else
    {
        if( customPlot->plotLayout()->hasElement(0,0) )
        {
            QCPTextElement*titletext= dynamic_cast<QCPTextElement* >(customPlot->plotLayout()->element(0,0));
            titletext->setText( "Convergence of job " + jobid );
        }
    }

    savejobidtext = jobid;

    if(conv.YIter.count()>0)
    {
        customPlot->addGraph();
        customPlot->graph(0)->setData(conv.YIter, conv.Y);
        customPlot->graph(0)->setPen(QPen(Qt::red));
        customPlot->graph(0)->setName("Y Field");
    }

    if(conv.XIter.count()>0)
    {
        customPlot->addGraph();
        customPlot->graph(1)->setData(conv.XIter, conv.X);
        customPlot->graph(1)->setPen(QPen(Qt::blue));
        customPlot->graph(1)->setName("X Field");
    }

    customPlot->xAxis->setLabel("Iterations");
    customPlot->yAxis->setLabel("Residual Error");

    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    customPlot->yAxis->setTicker(logTicker);
    customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);

    int IterRangeX = 0;
    double Xmin=100;
    double Xmax=0;
    if( !conv.XIter.isEmpty())
    {
        IterRangeX = 1000* ((double(conv.XIter.last())/1000)+0.5);
        Xmin = *std::min_element(conv.X.constBegin(), conv.X.constEnd());
        Xmax = *std::max_element(conv.X.constBegin(), conv.X.constEnd());
        qDebug()<< " Xiter=" <<conv.XIter.last();
    }

    long runtime_sec = jal->getRuntime(i);


    int IterRangeY = 0;
    double Ymin=100;
    double Ymax=0;
    double timePerIter=0;
    if( !conv.YIter.isEmpty())
    {
        IterRangeY = 1000* ((double(conv.YIter.last())/1000)+0.5);
        Ymin = *std::min_element(conv.Y.constBegin(), conv.Y.constEnd());
        Ymax = *std::max_element(conv.Y.constBegin(), conv.Y.constEnd());
        qDebug()<< " Yiter=" <<conv.YIter.last();

        timePerIter = double(runtime_sec)  / double(conv.YIter.last());
    }

    int IterRange = (IterRangeX>IterRangeY)?IterRangeX:IterRangeY;

    // qDebug() << "Total Iters so far="<<IterRangeX+IterRangeY;
    // qDebug() << "IterRange=" <<IterRange;


    if(!conv.XIter.isEmpty())
    {
        int remainingIter = (conv.YIter.size()-conv.XIter.size());

        timePerIter = double(runtime_sec)  / ( double(conv.YIter.last() +  conv.XIter.last()));

        std::stringstream msg;
        msg << "Approx. remaining iterations: " << remainingIter << " which might need "  << " " << abs((remainingIter * timePerIter  / 3600)+0.5)
            << "h (" << abs(remainingIter * timePerIter)<<"s) based on the Y solution (no setup time, scatter grid computation included).";

        std::string s( msg.str() );
        QString qstr = QString::fromStdString(msg.str());
        logmessage(qstr);
    }

    // qDebug() << "runtime_sec = " << runtime_sec <<"Time per Iteration " << 1000 *  timePerIter  <<"ms";

    customPlot->xAxis->setRange(0, IterRange);

    double MinVal = (Xmin>Ymin)?Ymin:Xmin;
    double MaxVal = (Xmax>Ymax)?Xmax:Ymax;
    // qDebug() << "minval="<<MinVal<<" log10(MinVal)="<<log10(MinVal);

    int logmin = abs(log10(MinVal)-0.99);
    int logmax = abs(log10(MaxVal)+0.99);
    customPlot->yAxis->setRange( pow(10.0,logmax),pow(10.0,-logmin) );

    customPlot->xAxis2->setVisible(true);
    double time_h = (IterRange * timePerIter)/3600;
    customPlot->xAxis2->setRange(0, time_h  );
    customPlot->xAxis2->setLabel("Time (h)" );
    customPlot->xAxis2->setLabelPadding(-50);

    // setup legend:
    customPlot->legend->setFont(QFont(font().family(), 7));
    customPlot->legend->setIconSize(50, 20);
    customPlot->legend->setVisible(true);
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight | Qt::AlignTop);


    // see if we have the residual error information
    double eps = jal->getResErr(jobid);
    if( eps >0)
    {
        QPen pen;
        pen.setStyle(Qt::DashDotLine);

        // add the arrow:
        QCPItemLine *reserrorline = new QCPItemLine(customPlot);
        reserrorline->start->setCoords(0, eps);
        reserrorline->end->setCoords(IterRange, eps);
        reserrorline->setPen(pen);

        QCPItemText *reserrorLabel = new QCPItemText(customPlot);

        reserrorLabel->setPositionAlignment(Qt::AlignLeft|Qt::AlignBottom);
        reserrorLabel->position->setCoords(20, eps);
        reserrorLabel->setText("Set residual error");
        reserrorLabel->setFont(QFont(font().family(), 12));
        reserrorLabel->setPen(QPen(Qt::NoPen));

        if( eps< pow(10.0,-logmin) )
        {
            int logmin = abs(log10(eps)-0.99);
            qDebug() <<"logmin ="<<logmin;
            customPlot->yAxis->setRange( pow(10.0,logmax), pow(10.0,-(logmin+0.1)) );
        }
    }

    customPlot->replot();

    locked=false;
}

void MainWindow::on_actionAbout_triggered()
{
    qDebug() << "on_actionAbout_triggered";
    HelpAbout HA;
    HA.exec();
}

void MainWindow::on_pushButton_release_clicked()
{
    qDebug() <<"release";
    jal->release(ui->listView_runningjobs->currentIndex().row());
}

void MainWindow::on_pushButton_hold_clicked()
{
    qDebug() <<"hold";
    jal->hold( ui->listView_runningjobs->currentIndex().row() );
}

void MainWindow::on_pushButton_kill_clicked()
{
    qDebug() << "kill job ";
    jal->kill( ui->listView_runningjobs->currentIndex().row() );
    refreshjobs();
}

void MainWindow::on_pushButton_refresh_clicked()
{
    refreshjobs();
    logmessage("Refreshed joblist");
}

void MainWindow::on_pushButton_saveplot_clicked()
{
    QCustomPlot*customPlot = findChild<QCustomPlot*>("qcustomplot_convergence");

    QTime nowTime;
    QDate nowDate;
    QString suggestedfileName = nowDate.currentDate().toString("yyyy-MM-dd_")
            + nowTime.currentTime().toString("hh") +"h"
            + nowTime.currentTime().toString("mm") +"m-id" + savejobidtext +".pdf";

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),suggestedfileName,"");

    customPlot->savePdf(fileName);

}

void MainWindow::closeEvent(QCloseEvent *e)
{
    qDebug() << "void MainWindow::closeEvent()";
    Q_UNUSED(e);

    QSettings settings("ADDA-team","ADDAjobmanager");
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("mainWindowState", saveState());
    settings.setValue("splitterSizes_vertical", ui->splitter_vertical->saveState());
    settings.setValue("splitterSizes_horizontal", ui->splitter_horizontal->saveState());

}

void MainWindow::logmessage(QString s)
{
    ui->textBrowser_console->append(s);

    // scroll to bottom
    QScrollBar *sb = ui->textBrowser_console->verticalScrollBar();
    sb->setValue(sb->maximum());

    ui->textBrowser_console->repaint();
    qApp->processEvents();
}

void MainWindow::refreshjobs()
{
    // qDebug()<< "currentMachine="<<currentMachine;

    if(currentMachine.isEmpty() )
    {
        return;
    }

    jal = QSharedPointer<JAL>( new JAL(currentMachine) );

    // signals are automatically disconnected upon destruction
    // needs raw pointer for the connect
    connect(jal.data(), SIGNAL(logmessage(QString)), this, SLOT(displayMessage(QString)));
    connect(jal.data(), SIGNAL(errmessage(QString)), this, SLOT(displayErrorMessage(QString)));

    QListView*qlistview_runningjobs = findChild<QListView*>("listView_runningjobs");

    QStringListModel*list = new QStringListModel(jal->getRunningJobs());
    qlistview_runningjobs->setModel(list);
}

void MainWindow::on_comboBox_accounts_currentIndexChanged(const QString &arg1)
{
    currentMachine = arg1.trimmed();
    refreshjobs();
}


void MainWindow::on_pushButton_showlog_clicked()
{        
    int index = ui->listView_runningjobs->currentIndex().row();

    if( index<0 )
    {
        return;
    }

    QString log = jal->showLog( index );

    logmessage("log for jobid [ " + jal->getJobID(index) + " ] (no convergence data shown) -----------------------------------------");
    logmessage(log);
    logmessage("--------------------------------------------------------------------------------------------------------------------");
}
