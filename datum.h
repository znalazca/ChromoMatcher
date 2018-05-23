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

#ifndef DATUM_H
#define DATUM_H

#include <QString>
#include <QtSql>
#include <QTableWidgetItem>

// structure for segment
struct Segment
{
    int *chrno;
    int *posbegin;
    int *posend;
    double *cm;
    int *SNP;
    QString *color;
    QString *name;
    QString *sex;
    int *source;
    QString *kit;
    QString *email;

    Segment();
    ~Segment();
};

// structure for control group
struct CG
{
    int *number;
    QString *name;
    QString *color;

    ~CG();
};

class Datum
{
public:
    Datum(QString personStr, QSettings *s);
    ~Datum();

public:
    static QString path();

    void readCGs();
    QList<CG*>* getCGs();
    CG* getCG(QString name);
    void updateCG(int segNumber, int cgNumber);
    void updateComment(int segNumber, QString comment);
    void deleteSegment(int segNumber);
    QString* Person();
    QSqlDatabase* DB();
    QSettings* Settings();
    void setRow(QList<QTableWidgetItem*> *items);
    QList<QTableWidgetItem*>* Row();
    QList<int> getSelectedRows(QItemSelection *selection);
    QList<int> getSelectedNumbers(QItemSelection *selection);
    void setSearchStr(QString str);
    QString* SearchStr();
    bool Admixture();

    // Enumeration represents columns in ui->tableWidget
    enum
    {
        Number = 0,
        ChrNo = 1,
        Begin = 2,
        End = 3,
        CM = 4,
        SNP = 5,
        Color = 6,
        Name = 7,
        Sex = 8,
        Scheme = 9,
        Source = 10,
        Kit = 11,
        Email = 12,
        ControlGroup = 13,
        Comment = 14,
        CGroupColor = 15,
        Added = 16
    };

private:
    QList<CG*> *CGs;
    QString *person;
    QSqlDatabase *dbase;
    QSettings *settings;
    QList<QTableWidgetItem*> *row;
    QString *searchStr;
    bool isAdmixture;
};

#endif // DATUM_H
