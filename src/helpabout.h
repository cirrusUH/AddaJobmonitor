/*
  Help window containing GPLv3 text

  Licence: Gnu General Public License, Version 3, 29 June 2007

  G Ritter, London, Jan 2017, University of Hertforshire, UK
*/

#ifndef HELPABOUT_H
#define HELPABOUT_H

#include <QDialog>
#include "ui_helpabout.h"

class HelpAbout : public QDialog, private Ui::HelpAbout
{
    Q_OBJECT
public:
    explicit HelpAbout(QDialog *parent = 0);

private:

signals:

public slots:

};

#endif // HELPABOUT_H
