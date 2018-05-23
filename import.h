/*
 Copyright (C) 2018 Dzmitry Hatouka (Gotowka). htotatut@gmail.com

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. See COPYING.txt
 If not, see <https://www.gnu.org/licenses/>.

 Please note that this program has CLA (Contributor Licence Agreement).
 You accept it by making any contribution to this program.
 See file CONTRIBUTE.txt for more details.
 List of 3-rd party components and their licenses is in 3RDPARTIES.txt
*/

#ifndef IMPORT_H
#define IMPORT_H

#include <QDialog>
#include <QSettings>
#include <QFile>
#include <QtSql>
#include "datum.h"

namespace Ui {
class Import;
}

class Import : public QDialog
{
    Q_OBJECT

public:
    explicit Import(QDialog *parent = 0, Datum *d = 0);
    ~Import();

private slots:
    void on_btnCancel_clicked();
    void on_btnBrowse_clicked();

private:
    Ui::Import *ui;

    Datum *datum;

    void twoOrMore(QFile *file, QList<Segment*> *segments);
    void matchingSegment(QFile *file, QList<Segment*> *segments);
    void CSV(QFile *file, QList<Segment*> *segments);

    // Enumeration represents sources
    enum
    {
        Manual = 1,
        TwoOrMore = 2,
        Tier1 = 3,
        FTDNA = 4,
        MyHeritage = 5
    };
};

#endif // IMPORT_H
