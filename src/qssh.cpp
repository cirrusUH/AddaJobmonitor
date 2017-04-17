#include "qssh.h"

Qssh::Qssh(QString login)
    : wecanrun(true)
{
    sshcommand = "ssh -C ";

    // try using PuTTY  on windows
#ifdef Q_OS_WIN
    // this works only on winXP:
    // sshcommand = "plink -agent -ssh -C ";
    sshcommand = "\"c:\\Program Files\\PuTTY\\plink.exe\" -agent -ssh -C ";
#endif 

    checkforssh();

    QStringList strlist = login.split('@');
    if( strlist.size()>1)
    {
        username = strlist.at(0);
        hostname = strlist.at(1);
    }
}

Qssh::~Qssh()
{
    qDebug() << "~Qssh()";
}

bool Qssh::runsshcmd(QString cmd)
{

    return runcommand(sshcommand + " " +cmd);

}

bool Qssh::runcommand(QString cmd)
{
    // ssh command rate limit, prevents triggering any ip ssh access intrusion detection scripts on the target
    const int sshratelimit_msec = 10000;

    emit logmsg("cmd: " + cmd +",  ssh command rate limit: " + QString::number(sshratelimit_msec/1000 )+ " cmds/s...");
    QCoreApplication::processEvents();

    // ssh command rate limit
    QTimer::singleShot(sshratelimit_msec, this, SLOT(clearsshcommandwaittime()));

    while( !wecanrun )
    {
        // QThread::msleep(20); // new qt
        Sleeper::msleep(20);
        QCoreApplication::processEvents();
    }

    wecanrun=false;

    QProcess process;

    process.start(cmd);

    qDebug() << process.environment();

    bool ret = process.waitForFinished(20000);

    if( ret )
    {
        stdoutstr = process.readAllStandardOutput();
        stderrstr = process.readAllStandardError();
        emit logmsg( process.readAllStandardError() );

    }
    else
    {

        QString errmsg = "---\nCould not execute remote command:\n" + cmd + "\nUser and account are correctly working?\n"
                + "You need ssh keys set up correctly (see ssh-copy-id and ssh-add), and "
                + "password less login must work. This is via PuTTY agent (pageant.exe) and plink.exe on Windows.\n---\n";

        emit logmsg(errmsg);
    }

    process.close();

    return ret;
}

bool Qssh::getHaveSSH()
{
    return havessh;
}

QString Qssh::getUserName()
{
    return username;
}


QString Qssh::getServerName()
{
    return hostname;
}

QStringList Qssh::getRunningJobs()
{
    runcommand(sshcommand + username+"@"+hostname +" qstat -u " + username );

    QStringList lines = stdoutstr.split('\n');

    /*
    // list also other user's jobs
    runcommand(sshcommand + username+"@"+hostname +" qstat -u " + "otheruser000" );
    QStringList othertmp = stdoutstr.split('\n');
        lines = lines + othertmp;
    // display other user's jobs
*/

    QStringList tmp;

    for(int i=0; i<lines.count(); i++)
    {
        // if(lines[i].contains(" R "))  // we probably want to see all jobs
        {
            tmp.append( lines[i]);
        }
    }

    return tmp;
}

bool Qssh::checkforssh()
{
    bool ret;

#ifdef Q_OS_WIN
    qDebug() << "plink detection:";
    ret = runcommand("\"c:\\Program Files\\PuTTY\\plink.exe\" -V");
    qDebug() << stdoutstr;
    qDebug() << stderrstr;

    if(ret)
    {
        // linux:     "ssh -V: OpenSSH_6.6.1p1, OpenSSL 1.0.1e-fips 11 Feb 2013"
        // msys2-win:
        // PuTTY winxp:  "plink: Release 0.67"
        // Putty win7_32 "plink: Release 0.67"

        if(stdoutstr.contains("Release") )
        {
            qDebug() << "found plink";
            emit logmsg("Found c:\\Program Files\\PuTTY\\plink.exe.");

            havessh = true;
            return havessh;
        }
    }
#endif

    ret = runcommand("ssh -V");
    wecanrun=true; // this is a local command

    if(ret)
    {
        // linux:     ssh -V: OpenSSH_6.6.1p1, OpenSSL 1.0.1e-fips 11 Feb 2013
        // msys2-win:
        if(stderrstr.contains("SSH")||stderrstr.contains("SSL") )
        {
            qDebug() << "found ssh";
            emit logmsg("Found ssh");
            havessh = true;
            return havessh;
        }
    }

    QMessageBox::information(0, "SSH error", "Could not execute ssh/plink, remote commands will not work.");
    emit errmsg(QString("SSH error: Could not execute ssh/plink, remote commands will not work."));

    havessh=false;
    return havessh;
}

QString Qssh::getstdoutstr()
{
    return stdoutstr;
}

void Qssh::clearsshcommandwaittime()
{
    wecanrun=true;
}
