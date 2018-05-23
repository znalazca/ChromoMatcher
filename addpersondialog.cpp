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

// Dialog for adding a new person

#include "addpersondialog.h"
#include "ui_addpersondialog.h"
#include <QFileDialog>
#include "datum.h"

AddPersonDialog::AddPersonDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddPersonDialog)
{
    ui->setupUi(this);

    // Fixed size and no buttons
    this->setFixedSize(this->width(), this->height());
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // Get settings (needs default folder)
    QString settingsFile = Datum::path() % "/settings.ini";
    settings = new QSettings(settingsFile, QSettings::IniFormat);

    thread = nullptr;
}

AddPersonDialog::~AddPersonDialog()
{
    delete ui;
}

void AddPersonDialog::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open raw data"), settings->value("Person_Path").toString(), tr("Raw data files (*.gz)"));

    if(!fileName.isEmpty())
    {
        QFileInfo fileInfo(fileName);
        // Set default folder
        settings->setValue("Person_Path", fileInfo.path());

        ui->listOutput->clear();

        // Thread for parsing raw data
        thread = new AddPersonThread(fileName);

        connect(thread, SIGNAL(addText(QString)), SLOT(onTextAdded(QString)));
        connect(thread, SIGNAL(preparationFinished(QByteArray*)), SLOT(onPreparationFinished(QByteArray*)));

        this->setEnabled(false);

        thread->start();
    }
}

// Add text to UI from parsing thread
void AddPersonDialog::onTextAdded(QString info)
{
    ui->listOutput->addItem(info);
}

// Signals that parsing thread finished its job
void AddPersonDialog::onPreparationFinished(QByteArray *destination)
{
    if(destination->data() != NULL)
    {
        ui->btnAdd->setEnabled(true);

        processedData = destination;
    }

    this->setEnabled(true);
}

void AddPersonDialog::on_btnCancel_clicked()
{
    this->close();
}

// Cleanup on close
void AddPersonDialog::closeEvent(QCloseEvent * event)
{
    settings->deleteLater();
    if(thread != nullptr)
    {
        thread->deleteLater();
    }

    event->accept();
}

void AddPersonDialog::on_btnAdd_clicked()
{
    QString name = ui->tbName->text();

    // name must follow strict requirements because of Dodecad calculator
    if(!QRegularExpression("^[A-Za-z0-9_-]+$").match(name).hasMatch())
    {
        ui->listOutput->addItem(tr("Name may only contain numbers, latin characters and _ - symbols!"));
        return;
    }

    QFile destFile(Datum::path() % name % ".txt");
    QString sqlFile = Datum::path() % name % ".sqlite";

    if((destFile.exists())&&(QFile::exists(sqlFile)))
    {
        ui->listOutput->addItem(tr("Name is already taken."));
        return;
    }

    // Creates sqlite file for segments
    QSqlDatabase dbase = QSqlDatabase::addDatabase("QSQLITE", "addperson");
    dbase.setDatabaseName(sqlFile);

    if (!dbase.open())
    {
        ui->listOutput->addItem(dbase.lastError().text());
        return;
    }

    QString createPerson = "CREATE TABLE segments ("
            "number INTEGER PRIMARY KEY NOT NULL, "
            "matchid INTEGER DEFAULT 0 NOT NULL, "            // reserved for future
            "chrno INTEGER, "
            "posbegin INTEGER, "
            "posend INTEGER, "
            "cm REAL, "
            "SNP INTEGER DEFAULT 0 NOT NULL, "
            "color TEXT DEFAULT '#FFFFFF' NOT NULL, "
            "name TEXT, "
            "sex TEXT DEFAULT '' NOT NULL, "
            "source INTEGER DEFAULT 1, "
            "kit TEXT DEFAULT '' NOT NULL, "
            "email TEXT DEFAULT '' NOT NULL, "
            "cgroup INTEGER DEFAULT 1 NOT NULL, "
            "comment TEXT DEFAULT '' NOT NULL, "
            "added INTEGER);";

    QSqlQuery q(dbase);

    try
    {
        if(!q.exec(createPerson))
        {
            throw q.lastError().text();
        }

        QString createCGs = "CREATE TABLE cgroups ("
            "number INTEGER PRIMARY KEY NOT NULL, "
            "name TEXT UNIQUE, "
            "color TEXT);";

        if(!q.exec(createCGs))
        {
            throw q.lastError().text();
        }

        QString insertCGs = "INSERT INTO cgroups (name, color) VALUES ('', '#FFFFFF');";

        if(!q.exec(insertCGs))
        {
            throw q.lastError().text();
        }

        QString createSources = "CREATE TABLE sources ("
                               "number INTEGER PRIMARY KEY NOT NULL, "
                               "name TEXT UNIQUE);";

        if(!q.exec(createSources))
        {
            throw q.lastError().text();
        }

        QString insertSources = "INSERT INTO sources (name) VALUES "
                "('Manual'), ('2 or More '), ('Tier 1'), ('FTDNA'), ('MyHeritage');";

        if(!q.exec(insertSources))
        {
            throw q.lastError().text();
        }

        ui->listOutput->addItem(tr("Segments file created. Done."));

        this->closeDB(&dbase);
    }
    catch(QString error)
    {
        ui->listOutput->addItem(error);
        this->closeDB(&dbase);
        QFile::remove(sqlFile);
    } 

    if(!ui->chbRawData->isChecked())
    {
        // Creates .txt file for Dodecad. It does not support sqlite
        if(!destFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            ui->listOutput->addItem(destFile.errorString());
            return;
        }

        destFile.write(processedData->data());
        delete processedData;
        destFile.close();

        ui->listOutput->addItem(tr("File with SNPs created."));
    }

    ui->btnAdd->setEnabled(false);
    ui->chbRawData->setEnabled(false);
    ui->btnBrowse->setEnabled(false);
}

void AddPersonDialog::closeDB(QSqlDatabase *dbase)
{
    dbase->close();
    //Get rid off "database is still in use" messages
    *dbase = QSqlDatabase();
    QSqlDatabase::removeDatabase("addperson");
}

// Checks if a person has raw data for admixture calculation
void AddPersonDialog::on_chbRawData_stateChanged(int arg1)
{
    ui->btnBrowse->setEnabled(!arg1);
    ui->btnAdd->setEnabled(arg1);
}
