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

// This class contains data related to opened person.
// Also it contains objects which are passed to child dialogs
// and implements SQL queries which are needed by different classes.

#include "datum.h"
#include <QtAlgorithms>
#include <QApplication>
#include <QList>
#include <QFile>

Segment::Segment()
{
    SNP = nullptr;
    color = nullptr;
    sex = nullptr;
    email = nullptr;
    kit = nullptr;
}

Segment::~Segment()
{
    delete chrno;
    delete posbegin;
    delete posend;
    delete cm;
    delete name;
    delete source;

    if(SNP != nullptr) delete SNP;
    if(color != nullptr) delete color;
    if(sex != nullptr) delete sex;
    if(kit != nullptr) delete kit;
    if(email != nullptr) delete email;
}

CG::~CG()
{
    delete number;
    delete name;
    delete color;
}

Datum::Datum(QString personStr, QSettings *s)
{
    // Create QList of contol groups and segments
    CGs = new QList<CG*>;

    person = new QString(personStr);
    settings = s;

    dbase = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    dbase->setDatabaseName(Datum::path() % *person % ".sqlite");
    dbase->open();

    this->readCGs();

    row = new QList<QTableWidgetItem*>;
    searchStr = new QString();

    // Check if admixture information is available
    QString admixture = Datum::path() % *person % ".txt";
    this->isAdmixture = QFile::exists(admixture);
}

Datum::~Datum()
{
    // Delete custom structure objects in QList and QList itself
    qDeleteAll(CGs->begin(), CGs->end());
    delete CGs;

    dbase->close();
    // Get rid of all these warning messages like "default connection still in use" despite database is being closed etc.
    delete dbase;
    dbase = new QSqlDatabase();
    QSqlDatabase::removeDatabase("qt_sql_default_connection");
    delete dbase;

    delete person;
    delete searchStr;
    delete row;
}

// Reads control groups
void Datum::readCGs()
{
    qDeleteAll(CGs->begin(), CGs->end());
    CGs->clear();

    QSqlQuery q("SELECT * FROM cgroups ORDER BY name;");

    if(!q.isSelect())
    {
        throw q.lastError().text();
    }

    while(q.next())
    {
        CG *cg = new CG();

        cg->number = new int(q.value(0).toInt());
        cg->name = new QString(q.value(1).toString());
        cg->color = new QString(q.value(2).toString());

        CGs->append(cg);
    }
}

// Returns all control groups
QList<CG*>* Datum::getCGs()
{
    return CGs;
}

// Returns control group based on name (name is unique)
CG* Datum::getCG(QString name)
{
    foreach(CG *cg, *CGs)
    {
        if(*cg->name == name)
        {
            return cg;
        }
    }

    return NULL;
}

// Returns settings object
QSettings* Datum::Settings()
{
    return settings;
}

// Represents current selected row in QTableWidget
void Datum::setRow(QList<QTableWidgetItem*> *items)
{
    row = items;
}

QList<QTableWidgetItem*>* Datum::Row()
{
    return row;
}

// Get selected rows or numbers in QTableWidget
QList<int> Datum::getSelectedRows(QItemSelection *selection)
{
    QList<int> rows;

    if(selection->indexes().count() == 0)
    {
        return rows;
    }

    foreach(const QModelIndex & index, selection->indexes())
    {
       int i = index.row();
       if(!rows.contains(i))
       {
           rows.append(i);
       }
    }

    return rows;
}

QList<int> Datum::getSelectedNumbers(QItemSelection *selection)
{
    QList<int> numbers;

    if(selection->indexes().count() == 0)
    {
        return numbers;
    }

    foreach(const QModelIndex & index, selection->indexes())
    {
       if(index.column() != 0)
       {
           continue;
       }

       int i = index.data().toInt();
       if(!numbers.contains(i))
       {
           numbers.append(i);
       }
    }

    return numbers;
}

// Set last search string
void Datum::setSearchStr(QString str)
{
    searchStr = new QString(str);
}

// and return it
QString *Datum::SearchStr()
{
    return searchStr;
}

// Sets new control group for a segment
void Datum::updateCG(int segNumber, int cgNumber)
{
    QSqlQuery q;
    QString str = "UPDATE segments SET cgroup = %1 WHERE number = %2;";
    QString arg = str.arg(cgNumber).arg(segNumber);

    if(!q.exec(arg))
    {
        throw q.lastError().text();
    }
}

// Updates comment for a segment
void Datum::updateComment(int segNumber, QString comment)
{
    QSqlQuery q;
    QString str = "UPDATE segments SET comment = '%1' WHERE number = %2;";
    QString arg = str.arg(comment).arg(segNumber);

    if(!q.exec(arg))
    {
        throw q.lastError().text();
    }
}

// Deletes segment
void Datum::deleteSegment(int segNumber)
{
    QSqlQuery q;
    QString str = "DELETE FROM segments WHERE number = %1;";
    QString arg = str.arg(segNumber);

    if(!q.exec(arg))
    {
        throw q.lastError().text();
    }
}

QSqlDatabase* Datum::DB()
{
    return dbase;
}

QString Datum::path()
{
    return QApplication::applicationDirPath() % "/data/";
}

// Returns person's name
QString* Datum::Person()
{
    return person;
}

// Returns true if admixture information available
bool Datum::Admixture()
{
    return isAdmixture;
}
