/*
  Job Abstraction Layer (JAL)

  Translates job control commands to the target platform.

  Licence: Gnu General Public License, Version 3, 29 June 2007

  G Ritter, London, Jan 2017, University of Hertforshire, UK
*/

#ifndef JAL_H
#define JAL_H

#include <QObject>
#include <QDebug>
#include <QProcess>
#include <QCoreApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QProcess>
#include <QFile>

#include "qssh.h"


// This holds adda convergence data, as output into log files.
class Convergence
{
public:
    QVector<double> XIter;
    QVector<double> YIter;
    QVector<double> Y;
    QVector<double> X;
};

class JAL : public QObject
{
    Q_OBJECT

public:
    enum  MachineType {UNKOWN, PBSREMOTE, PBSLOCAL, WINLOCAL, LINUXLOCAL};

private:
    MachineType machinetype;

    QString stdoutstr;
    QString stderrstr;
    QProcess process;

    QString login;
    QString user, host;

    QString runcommand(QString cmd);

    Qssh*qssh;

    QStringList joblisttext;
    QStringList pid;
    QList<bool> isrunningjob;
    QMap<QString,QString> workdircache;
    QMap<QString,double > reserrcache;

public:
    explicit JAL(QString login, QObject *parent = 0);
    explicit JAL(MachineType machinetype, QString login, QObject *parent = 0);

    void kill(int i);
    void release(int i);
    void hold(int i);

    QString getJobID(int i);
    QString getLogin();
    QStringList getRunningJobs();
    Convergence getConvergence(int i);
    QString getJobsText(int i);
    long getRuntime(int i);
    QString getJobDirectory(int i);
    QString showLog(int i);
    void getADDAcommandline(int i);
    double getResErr(QString jobid);
    bool getIsRunningJob( int i);

signals:
    void logmessage(QString logmsg);
    void errmessage(QString logmsg);

public slots:
    void sendlogmsg(QString msg);
    void senderrmsg(QString msg);
};

#endif // JAL_H
