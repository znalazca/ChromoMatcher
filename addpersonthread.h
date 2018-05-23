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

#ifndef ADDPERSONTHREAD_H
#define ADDPERSONTHREAD_H

#include <QThread>
#include <QFile>
#include <QFileDialog>
#include <quagzipfile.h>
#include <algorithm>
#include <QApplication>
#include <QTextStream>
#include <QRegularExpression>

// Custom structure for SNPs
// It is better to put it fully in heap because QList of this structure is huge.
struct Record
{
    QString *SNP;
    QString *chrNo;
    QString *Position;
    QString *Alleles;

    ~Record();
};

class AddPersonThread : public QThread
{
   Q_OBJECT

public:
    AddPersonThread(QString fileName);

protected:
   virtual void run();

signals:
   void addText(QString info);
   void preparationFinished(QByteArray *destination);

private:
   QString fileName_;
};

#endif // ADDPERSONTHREAD_H
