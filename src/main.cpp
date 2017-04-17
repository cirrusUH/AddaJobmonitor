/*
  Adda job manager

  Gui based runtime control and supervision of Adda[1] jobs running remotely (PBS or Linux)
  or locally (Windows and Linux).

  Remote targets are accessed via ssh (Putty on windows, OpenSSH on Linux)

  Local machine can be Windows (should work on all versions, XP needs an old version of handle.exe installed)

  Read docs for specific requirements for capturing the stdout of non Torque (PBS) running jobs.

  Qt used for development (or known to work):
    - QtCreator 3.5.1, Linux
    -

  C++: -std=c++1y

  [1] Adda-team: https://github.com/adda-team/adda

  Licence: Gnu General Public License, Version 3, 29 June 2007

  G Ritter, London, Jan 2017, School of Physics Astronomy and Mathematics, University of Hertforshire, UK

  QCustomplot for plotting:  http://www.qcustomplot.com/,  Version: 2.0.0-beta,
                             Autor: Emanuel Eichhammer, Version 2.0.0-beta, GPLv3
*/

#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
