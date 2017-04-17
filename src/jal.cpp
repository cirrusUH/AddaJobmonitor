#include "jal.h"

JAL::JAL(QString login, QObject *parent)
    : QObject(parent)
    , machinetype(UNKOWN)
    , login(login)
{
    // qDebug()<<"JAL::JAL(QString login=" << login << ", QObject *parent) ";

    QString retstr;

    if( login.contains("@") )
    {
        // we need ssh working but also need successful login so set up and check
        machinetype = MachineType::PBSREMOTE;
        qssh = new Qssh(login);

        user=qssh->getUserName();
        host=qssh->getServerName();

        connect(qssh, SIGNAL(logmsg(QString)), this, SLOT(sendlogmsg(QString)));
        connect(qssh, SIGNAL(errmsg(QString)), this, SLOT(senderrmsg(QString)));
    }
    else if(login.contains("localhost"))
    {
        // check if this localhost is windows or linux
#ifdef Q_OS_WIN
        // qDebug() << "check for windows:";
        machinetype = MachineType::WINLOCAL;
        // qDebug() << "machinetype = MachineType::WINLOCAL";
#else
        // qDebug() << "check for non windows";

        retstr = runcommand("uname");
        // qDebug() << retstr;

        if(retstr.contains("Linux"))
        {
            machinetype = MachineType::LINUXLOCAL;

            /*
            // check if it runs a local Torque resouce manager
            retstr = runcommand("qstat -B");
            qDebug() << retstr;

            if(retstr.contains("Server"))
            {
                machinetype=MachineType::PBSLOCAL;
            }
            */
        }
#endif // ifdef Q_OS_WIN
    }
}

JAL::JAL(MachineType machinetype, QString login, QObject *parent)
    : QObject(parent)
    , machinetype(machinetype)
    , login(login)
{
    // qDebug()<<"JAL::JAL(MachineType machinetype, QString login, QObject *parent)";
}

QString JAL::runcommand(QString cmd)
{
    // qDebug() << "QString JAL::runcommand(QString cmd=" << cmd <<")";
    QString logmsg = "cmd: "+ cmd;
    emit logmessage( logmsg );

    QCoreApplication::processEvents();

    if (machinetype==MachineType::UNKOWN)
    {
        process.start(cmd);

        bool ret = process.waitForFinished(20000);
        if( ret )
        {
            stdoutstr = process.readAllStandardOutput();
            stderrstr = process.readAllStandardError();
        }

        //qDebug() << "stdoutstr: " << stdoutstr << endl;
        //qDebug() << "stderrstr: " << stderrstr << endl;

        return stdoutstr;
    }
    if( machinetype == MachineType::LINUXLOCAL || machinetype==MachineType::WINLOCAL  )
    {
        process.start(cmd);

        bool ret = process.waitForFinished(20000);
        if( ret )
        {
            stdoutstr = process.readAllStandardOutput();
            stderrstr = process.readAllStandardError();
        }
        else
        {
            if(cmd.contains("handle"))
            {
                emit errmessage("Failed to execute "+cmd+"\n"
                                "Get from here: https://technet.microsoft.com/en-us/sysinternals/handle.aspx\n"
                                "and place in the system wide search path, c:\\windows for example\n");

                QMessageBox::information(0, "Error", "Could not execute '"+cmd+"'\n"
                                         "Please get this util from here:\n"
                                         "https://technet.microsoft.com/en-us/sysinternals/handle.aspx\n"
                                         "and place c:\\windows for example (must be in system wide search path).");
            }
            else
            {
                QString errmsg = "Could not execute local command: "+cmd;
                QMessageBox::information(0, "Error", errmsg);
                emit errmessage(errmsg);
            }
        }

        //qDebug() << "stdoutstr: " << stdoutstr << endl;
        //qDebug() << "stderrstr: " << stderrstr << endl;
        if(!stderrstr.isEmpty())
        {
            emit errmessage(stderrstr);
        }

        return stdoutstr;
    }
    else if( machinetype==MachineType::PBSREMOTE )
    {
        // all remote running adda processes, assuming PBS
        if(qssh->getHaveSSH())
        {
            qssh->runcommand( cmd );

            //qDebug() << "stdout\n" << qssh->stdoutstr;
            //qDebug() << "stderr\n" << qssh->stderrstr;

            if(!stderrstr.isEmpty())
            {
                emit logmessage(stderrstr);
            }

            return qssh->stdoutstr;
        }
    }
    else
    {
        qDebug()<<"internal error, wrong machinetype, should never happen";
    }

    qssh->runcommand(cmd);

    return "";
}

void JAL::kill(int i)
{
    // qDebug() <<"JAL::kill(int i)";
    if( i<0 || i>joblisttext.size() )
    {
        return;
    }

    QString jobid = getJobID(i);
    qDebug() <<jobid;

    if(  jobid.isEmpty())
    {
        return;
    }

    if( machinetype==MachineType::PBSREMOTE )
    {
        qssh->runsshcmd(login +" qdel " + jobid);
        // qDebug() << "stdout\n" << qssh->stdoutstr;
        // qDebug() << "stderr\n" << qssh->stderrstr;
    }
    else if(machinetype==MachineType::LINUXLOCAL)
    {
        runcommand(" kill " + jobid );
    }
    else if(machinetype==MachineType::WINLOCAL)
    {
        runcommand("taskkill \\PID " +jobid +" \\F");
    }
    else
    {
        qDebug()<<"internal error, wrong machinetype";
    }
}

void JAL::release(int i)
{
    // qDebug() <<"JAL::release(int i)";
    if( i<0 || i>joblisttext.size() )
    {
        return;
    }

    QString jobid = getJobID(i);
    qDebug() <<jobid;

    if( jobid.isEmpty() )
    {
        return;
    }

    if( machinetype==MachineType::PBSREMOTE )
    {
        qssh->runsshcmd(login +" qrls " + jobid);

        // qDebug() << "stdout\n" << qssh->stdoutstr;
        // qDebug() << "stderr\n" << qssh->stderrstr;
    }
    else if(machinetype==MachineType::LINUXLOCAL)
    {
        runcommand("kill -SIGCONT " + jobid);
    }
    else
    {
        qDebug()<<"internal error, wrong machinetype";
    }

}

void JAL::hold(int i)
{
    qDebug() <<"JAL::hold(int i)";

    if( i<0 || i>joblisttext.size() )
    {
        return;
    }

    QString jobid = getJobID(i);
    qDebug() <<jobid;

    if( jobid.isEmpty() )
    {
        return;
    }

    if( machinetype==MachineType::PBSREMOTE )
    {
        qssh->runsshcmd(login +" qhold " + jobid);
    }
    else if(machinetype==MachineType::LINUXLOCAL)
    {
        runcommand("kill -SIGSTOP " +jobid);
    }
    else
    {
        qDebug()<<"internal error, wrong machinetype";
    }
}

QString JAL::getJobID(int i)
{
    if( i<0 || i>joblisttext.size() )
    {
        return "";
    }

    return pid.at(i);
}

QString JAL::getLogin()
{
    return login;
}

// We retrieve a list of running adda jobs on the target system
QStringList JAL::getRunningJobs()
{
    QString retstr;

    joblisttext.clear();

    if(machinetype==MachineType::WINLOCAL)
    {
        // all locally running adda tasks

        // retstr = runcommand("tasklist /v /FI \"IMAGENAME eq adda.exe\" /FI \"USERNAME eq joeblogs\" ");
        retstr = runcommand("tasklist /v /FI \"IMAGENAME eq adda.exe\" /FO csv ");
        qDebug() << retstr;

        joblisttext = retstr.split('\n');

        // update pids
        foreach(QString str, joblisttext)
        {
            qDebug() << str;

            if ( !str.isEmpty())
            {
                bool isrunning = str.contains("adda.exe");

                // expected output:
                /*
Image Name                     PID Session Name        Session#    Mem Usage Status          User Name
              CPU Time Window Title
========================= ======== ================ =========== ============ =============== =========================================
========= ============ ========================================================================
adda.exe                      3620 Console                    1    262,452 K Unknown         win732\user000
               0:19:48 N/A

                 */

                if(isrunning)
                {
                    //qDebug() << "isrunning: '"<<str<<"'";
                    // QStringList tmp = str.split(' ');
                    // QStringList tmp = str.split(QRegExp("\\s+")); // any number of whitespaces
                    str.replace("\"","");
                    QStringList tmp = str.split(','); // any number of whitespaces

                    if( tmp.size() >1 )
                    {
                        pid.append(tmp.at(1));
                        // qDebug() << "pid.append(tmp.at(1))= " << tmp.at(1);
                    }
                    else
                    {
                        pid.append("");
                    }
                    isrunningjob.append(isrunning);

                }
                else
                {
                    isrunningjob.append(false);
                    pid.append("");
                }
            }
            else
            {
                isrunningjob.append(false);
                pid.append("");
            }

        }

        // cleanup for display
        joblisttext.replaceInStrings("\"","");
        joblisttext.replaceInStrings(",","\t");
    }
    else if(machinetype==MachineType::LINUXLOCAL)
    {
        // all locally running adda jobs

        retstr = runcommand("/bin/sh -c \"ps -eo pid,tty,s,etime,args | grep adda \"");

        // pwds <PID> to get the cirectory
        qDebug() << retstr;

        joblisttext =retstr.split('\n');

        // filter
        for( int i=0; i< joblisttext.size(); i++)
        {
            if(joblisttext.at(i).contains("grep") )
            {
                joblisttext[i]=QString("");
            }
        }

        // update pids
        foreach(const QString &str, joblisttext)
        {
            if ( !str.isEmpty() )
            {
                bool isrunning=true;

                // expected output:
                // 24828 pts/11   R+     0:01  |   \_ /opt/adda13b4/src/seq/adda -grid 100
                QStringList tmp = str.trimmed().split(' ');
                pid.append(tmp.at(0));

                isrunningjob.append(isrunning);
            }
            else
            {
                // not a job but a header
                isrunningjob.append(false);
                pid.append("");
            }
        }
    }
    else if( machinetype==MachineType::PBSREMOTE )
    {
        // all remote running adda processes, assuming PBS
        // qDebug() << "JAL::getRunningJobs() machinetype==MachineType::PBS ";

        if(qssh->getHaveSSH())
        {
            joblisttext = qssh->getRunningJobs();

            // update pids
            foreach(const QString &str, joblisttext)
            {
                qDebug() << str;

                if ( !str.isEmpty() && str.contains( qssh->getUserName() )  )
                {
                    bool isrunning =  str.contains(" R ") || str.contains(" B ");

                    isrunningjob.append( isrunning );



                    // expected output:

                    /*
                     * db:
                                                            Req'd  Req'd   Elap
Job ID          Username Queue    Jobname    SessID NDS TSK Memory Time  S Time
--------------- -------- -------- ---------- ------ --- --- ------ ----- - -----
4216939[].sdb   user000  long     50SE1to50R    --    6 144    --  48:00 B   --
4216940[].sdb   user000  long     50SE1to50a    --    6 144    --  48:00 B   --

or

1585260-1.stri-c     user0000  main     GLHx30bb-1        17397    16 128    --  148:0 R 78:47

*/

                    QStringList tmp = str.split('.');
                    pid.append(tmp.at(0));
                }
                else
                {
                    // not a job but a header
                    isrunningjob.append(false);
                    pid.append("");
                }
            }
        }
    }

    if( joblisttext.count() < 2 )
    {
        joblisttext.append("No running adda jobs");
        isrunningjob.append(false);
        pid.append("");
    }

    //qDebug() << "joblisttext" << joblisttext;
    return joblisttext;
}

// Find the logfile and retrieves the convergence data from a file
Convergence JAL::getConvergence(int i)
{
    Convergence conv;

    if( i<0 || i>joblisttext.size() )
    {
        return conv;
    }

    if( isrunningjob.size() < i )
    {
        return conv ;
    }

    if( !isrunningjob.at(i) )
    {
        return conv ;
    }

    QString jobid = getJobID(i);

    if( jobid.isEmpty())
    {
        return conv;
    }

    QString jobdir=workdircache[jobid];
    if( jobdir.isEmpty() )
    {
        jobdir = getJobDirectory(i);
        workdircache[jobid] = jobdir;
    }
    else
    {
        // qDebug() << "workdir cache hit: jopbid [ "<<jobid << " ] has " <<jobdir;
    }

    if( jobdir.isEmpty())
    {
        return conv;
    }

    // qDebug() << jobdir;
    // qDebug() << jobid;

    QStringList log;
    if (machinetype==MachineType::PBSREMOTE )
    {

        QString logfile = jobdir+"/out-" + jobid;

        qssh->runsshcmd(login +" cat " + logfile + "." + qssh->getServerName());

        log = qssh->stdoutstr.split('\n');

        // qDebug() << log;

    }
    else if(machinetype==MachineType::LINUXLOCAL)
    {
        // qDebug() << "MachineType::LINUXLOCAL PID="<< jobid;

        QString logfile = jobdir+"/out-" + jobid;

        runcommand("cat " +logfile);
        log = stdoutstr.split('\n');

        if( log.size()>0 )
        {
            log.removeLast();
        }
    }
    else if(machinetype==MachineType::WINLOCAL)
    {
        // qDebug() << "MachineType::WINLOCAL PID="<< jobid;

        QString logfile = jobdir+"\\log";

        QFile file(logfile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "Error opening file "+logfile;
        }

        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            line.replace("\r","");
            line.replace("\n","");
            log.append(line.trimmed());
        }

        if( log.size()>1 )
        {
            log.removeLast();
            log.removeLast();
        }
    }

    bool calcYflag=false;
    bool calcXflag=false;

    for(int i=0; i<log.count(); i++)
    {
        if(log[i].contains("here we go, calc Y"))
        {
            qDebug() << "found ----- here we go, calc Y";
            calcYflag = true;
            calcXflag = false;
        }

        if(log[i].contains("here we go, calc X"))
        {
            calcXflag = true;
            calcYflag = false;
        }

        if(log[i].contains("RE"))
        {
            QString tmp = log[i].replace("RE_","").trimmed();

            QString Iteration = tmp.section(" ",0,0);
            QString Epsilon = tmp.section(" ",2,2);

            if(calcYflag)
            {
                conv.Y.append(Epsilon.toDouble()  );
                conv.YIter.append(Iteration.toDouble());
            }

            if(calcXflag)
            {
                conv.X.append(Epsilon.toDouble());
                conv.XIter.append(Iteration.toDouble());
            }
        }
    }

    return conv;
}

QString JAL::getJobsText(int i)
{
    if( i<0 || i>joblisttext.size() )
    {
        return "";
    }
    QString tmp;

    if( isrunningjob.at(i) )
    {
        tmp = joblisttext.at(i);
    }
    else
    {
        tmp = "";
    }
    return tmp;
}

long JAL::getRuntime(int i)
{
    if( i<0 || i>joblisttext.size() )
    {
        return 0;
    }

    if (machinetype==MachineType::UNKOWN)
    {

    }
    else if( machinetype == MachineType::LINUXLOCAL )
    {
        QString line = getJobsText( i );
        QString JobID, username, queue, jobname, sessionID, NDS,TSK, mem,reqTime,ElapsTime;

        QStringList j = line.split(QRegExp("\\s+") );

        double runtime_sec=0;
        if( j.size() >3)
        {
            qDebug() <<j;
            qDebug() << "runtime="<<j[3];
            QStringList runtimestr = j[3].split(':');
            // 42:31 minutes
            // 01:42:31 hours

            qDebug() << runtimestr;

            if( runtimestr.count()==1 )
            {
                runtime_sec= runtimestr[0].toInt();
            }
            if(runtimestr.count()==2 )
            {
                runtime_sec=60 * runtimestr[0].toInt() + runtimestr[1].toInt();
            }
            else if(runtimestr.count()==3)
            {

                runtime_sec=3600 * runtimestr[0].toInt() + 60*runtimestr[1].toInt() + runtimestr[2].toInt();
            }
            else
            {
                qDebug() << "error parsing runtime";
                emit errmessage("error parsing runtime");
            }
        }

        return runtime_sec;
    }
    else if ( machinetype==MachineType::WINLOCAL)
    {
        QString line = getJobsText( i );
        qDebug() << "getruntime line " << line;
        QString JobID, username, queue, jobname, sessionID, NDS,TSK, mem,reqTime,ElapsTime;

        QStringList j = line.split(QRegExp("\\s+") );

        double runtime_sec=0;
        if( j.size() >9)
        {
            qDebug() <<j;
            qDebug() << "runtime="<<j[9];
            QStringList runtimestr = j[9].split(':');
            //42:31 minutes
            //01:42:31 hours

            qDebug() << runtimestr;

            if( runtimestr.count()==1 )
            {
                runtime_sec= runtimestr[0].toInt();
            }
            if(runtimestr.count()==2 )
            {
                runtime_sec=60 * runtimestr[0].toInt() + runtimestr[1].toInt();
            }
            else if(runtimestr.count()==3)
            {

                runtime_sec=3600 * runtimestr[0].toInt() + 60*runtimestr[1].toInt() + runtimestr[2].toInt();
            }
            else
            {
                qDebug() << "parse error of runtime";
            }
        }

        return runtime_sec;
    }
    else if( machinetype==MachineType::PBSREMOTE )
    {
        QString line = getJobsText( i );
        QString JobID, username, queue, jobname, sessionID, NDS,TSK, mem,reqTime,ElapsTime;

        QStringList j = line.split(QRegExp("\\s+") );

        double runtime_sec=0;
        if( j.size() >10)
        {
            qDebug() <<j;
            qDebug() << "runtime="<<j[10];
            QStringList runtimestr = j[10].split(':');
            runtime_sec=3600 * runtimestr[0].toInt() + 60*runtimestr[1].toInt();
        }

        return runtime_sec;
    }
    else
    {
        qDebug() << "internal error, wrong machinetype, should never happen";
    }

    return 0;
}

QString JAL::getJobDirectory(int i)
{
    if( i<0 || i>joblisttext.size() )
    {
        return "";
    }

    if(!isrunningjob.at(i))
    {
        return "";
    }

    QString PBS_O_WORKDIR;

    QString jobid = pid.at(i);

    if (machinetype==MachineType::PBSREMOTE )
    {
        qssh->runsshcmd( login +" qstat -f " + jobid );

        QStringList tmp = qssh->stdoutstr.split("\n");
        QString tmplong;
        for(int i=0; i<tmp.count(); i++)
        {
            tmplong += tmp[i].trimmed();
        }

        QStringList lines = tmplong.split(",");

        for(int i=0; i<lines.count(); i++)
        {
            lines[i]=lines[i].trimmed();
            if( lines[i].contains("PBS_O_WORKDIR") )
            {
                QStringList tmp = lines[i].trimmed().split("=");
                if(tmp.count()>1)
                {
                    PBS_O_WORKDIR = tmp[1].trimmed();
                    break;
                }
            }
        }

        qDebug() << "PBS_O_WORKDIR = " << PBS_O_WORKDIR;
    }
    else if(machinetype==MachineType::LINUXLOCAL)
    {
        qDebug() << "machinetype==MachineType::LINUXLOCAL workingdir";
        qDebug() << "jobid="<<jobid;
        PBS_O_WORKDIR = runcommand("pwdx "+jobid).split(' ').at(1).trimmed();
    }
    else if(machinetype==MachineType::WINLOCAL)
    {
        /* Windows does not provide an easy way to find the
           working directory, or "current directory".

           Use sysinternal's third party tool here for now. One should be able to implement this using
           NtQuerySystemInformation, apply a filter and then look for the current directory ":8"
           but this is rather complicated. So getting handle.exe or handle64.exe from here:
           https://technet.microsoft.com/en-us/sysinternals/handle.aspx
           and place the systemwide search path c:\windows\ e.g.

           Windows XP: The handle.exe published on the above link does no longer work on win XP.
           For this it needs and older version of the handle.exe binary (->google).
        */

        QString handleoutput = runcommand("handle -p " + jobid);

        QStringList handleoutputlines = handleoutput.split('\n');

        // windows XP?
        bool winXP=false;

#ifdef WINVER
        qDebug() << " WINVER=" << WINVER;
#if (WINVER < 0x0502)
        winXP=true;
#endif
        qDebug() << " winXP=" << winXP;
#endif

        foreach(const QString &str, handleoutputlines)
        {

            if( ( str.contains("64: File") || str.contains("7C0: File") ) && str.contains("log") )
            {
                QStringList items = str.split(QRegExp("\\s+"));
                if(items.size()>2)
                {
                    if(winXP)
                    {
                        PBS_O_WORKDIR=items.at(4);
                    }
                    else
                    {
                        PBS_O_WORKDIR=items.at(3);
                    }
                    PBS_O_WORKDIR.replace("log","");
                }
                else
                {
                    PBS_O_WORKDIR="";
                }
            }

        }
        // emit sendlogmsg(handleoutput);
    }

    return PBS_O_WORKDIR;
}

QString JAL::showLog(int i)
{
    Convergence conv;

    qDebug() <<"Convergence JAL::showLog(int i=" <<i<< ")";

    if( i<0 || i>joblisttext.size() )
    {
        return "";
    }

    if( isrunningjob.size() < i )
    {
        return "" ;
    }

    if( !isrunningjob.at(i) )
    {
        return "" ;
    }

    QString jobid = getJobID(i);

    if( jobid.isEmpty() )
    {
        return "";
    }

    QString jobdir=workdircache[jobid];
    if( jobdir.isEmpty() )
    {
        jobdir = getJobDirectory(i);
        workdircache[jobid] = jobdir;
    }
    else
    {
        qDebug() << "workdir cache hit";
    }


    if( jobdir.isEmpty())
    {
        return "";
    }

    qDebug() <<jobdir;
    qDebug() <<jobid;

    QString log;
    if (machinetype==MachineType::PBSREMOTE )
    {
        qssh->runsshcmd(login +" cat " + jobdir+"/*" + jobid +"/log  | grep -v 'RE_' " );

        // log = qssh->stdoutstr.split('\n');
        log = qssh->stdoutstr;

        // log file contains string:
        // 'Required relative residual norm: 0.00301995'
        QStringList logfilelines=log.split('\n');

        double eps=-1;

        for(int i=0; i<logfilelines.count(); i++)
        {
            if( logfilelines[i].contains("Required relative residual norm:") )
            {
                eps = logfilelines[i].split(' ').at(4).toDouble();
            }
        }

        reserrcache[jobid] = eps;

        qDebug() << "set reserrcache["<<jobid<<"] = "<<eps;

    }
    else if(machinetype==MachineType::LINUXLOCAL)
    {
        qDebug() << "MachineType::LINUXLOCAL PID="<< jobid;

        QString logfile = jobdir+"/out-" + jobid;

        runcommand("/bin/sh -c \" cat " + logfile.trimmed() + " | grep -v 'RE_' \" " );

        log = stdoutstr;

        // logfile containes string:
        // 'Required relative residual norm: 0.00301995'
        QStringList logfilelines=log.split('\n');

        double eps=-1;

        for(int i=0; i<logfilelines.count(); i++)
        {
            if( logfilelines[i].contains("Required relative residual norm:") )
            {
                eps = logfilelines[i].split(' ').at(4).toDouble();
            }
        }

        reserrcache[jobid] = eps;

        qDebug() << "set reserrcache["<<jobid<<"] = "<<eps;
    }
    else if( machinetype==MachineType::WINLOCAL  )
    {
        log="";
        QStringList logfilelines;
        QString logfile = jobdir+"\\log";

        qDebug() << "opening " +logfile;

        QFile file(logfile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QString errmsg = "Error opening file "+logfile;
            qDebug() << errmsg;

            emit errmessage(errmsg);
        }

        while (!file.atEnd())
        {
            QByteArray line = file.readLine();

            if(!line.contains("RE_"))
            {
                log += line;

                line.replace("\r","");
                line.replace("\n","");

                logfilelines.append(line.trimmed());
            }
        }

        double eps=1;

        for(int i=0; i<logfilelines.count(); i++)
        {
            if( logfilelines[i].contains("Required relative residual norm:") )
            {
                eps = logfilelines[i].split(' ').at(4).toDouble();
            }
        }

        reserrcache[jobid] = eps;

        qDebug() << "set reserrcache["<<jobid<<"] = "<<eps;
    }

    return log;
}

void JAL::getADDAcommandline(int i)
{
    if( i<0 || i>joblisttext.size() )
    {
        return;
    }
    Q_UNUSED(i);
    qDebug() << "JAL::getADDAcommandline(int i): not implemented ";
}

double JAL::getResErr(QString jobid)
{
    qDebug()<< "JAL::getResErr(QString jobid)="<<reserrcache[jobid];
    return reserrcache[jobid];

}

bool JAL::getIsRunningJob(int i)
{
    if( i<0 || i>joblisttext.size() )
    {
        return false;
    }

    return isrunningjob.at(i);
}

// relay (from qssh)
void JAL::sendlogmsg(QString msg)
{
    emit logmessage(msg);
}

// relay (from qssh)
void JAL::senderrmsg(QString msg)
{
    emit errmessage(msg);
}


/* Empty machines branches template

    if (machinetype==MachineType::UNKOWN)
    {

    }
    if( machinetype == MachineType::LINUXLOCAL || machinetype==MachineType::WINLOCAL  )
    {

    }
    else if( machinetype==MachineType::PBSREMOTE )
    {

    }
    else
    {
        qDebug()<<"internal error, wrong machinetype, should never happen";
    }

 */
