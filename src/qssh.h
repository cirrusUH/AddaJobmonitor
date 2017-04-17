/*
  Layer to the system installed ssh.

  On windows this is assumes installation of putty [1] into the standard suggested
  location and correctly setup ssh key management (keybased login into the target machine) via pageant.

  On linux the command 'ssh' (usually openssh) is expected, as well as correctly set up ssh-key based login
  into the target.

  [1] http://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html

  Licence: Gnu General Public License, Version 3, 29 June 2007

  G Ritter, London, Jan 2017, University of Hertforshire, UK
*/

#ifndef QSSH_H
#define QSSH_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QProcess>
#include <QDebug>
#include <QMessageBox>
#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <QThread>


// this is not needed on newer Qts
class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};


class Qssh : public QObject
{
    Q_OBJECT

public:
    Qssh(QString login);
    ~Qssh();

    bool runcommand(QString cmd);
    bool runsshcmd(QString cmd);
    bool checkforssh();
    bool getHaveSSH();

    QString getUserName();
    QString getServerName();

    QStringList getRunningJobs();

    QString stdoutstr;
    QString stderrstr;

    QString getstdoutstr();
private:
    bool havessh;
    bool wecanrun;
    QString username;
    QString hostname;
    QString sshcommand;

signals:
    void stderrorsignal(QString s);
    void logmsg(QString s);
    void errmsg(QString s);

private slots:
    void clearsshcommandwaittime();
};

#endif // QSSH_H
