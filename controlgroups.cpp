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

// Dialog for managing control groups

#include "controlgroups.h"
#include "ui_controlgroups.h"
#include <QMessageBox>
#include <QColorDialog>
#include <QInputDialog>
#include <QtSql>

ControlGroups::ControlGroups(QDialog *parent, Datum *d) :
    QDialog(parent),
    ui(new Ui::ControlGroups)
{
    ui->setupUi(this);

    // No need to update QTableWidget yet
    this->setProperty("update", false);

    datum = d;

    // Window is fixed and without buttons
    this->setFixedSize(this->width(),this->height());
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // This label background is set to control group color
    ui->lbColor->setAutoFillBackground(true);

    this->displayCGs();
}

ControlGroups::~ControlGroups()
{
    delete ui;
}

void ControlGroups::displayCGs()
{
    ui->listCGs->clear();
    datum->readCGs();
    ui->lbColor->setPalette(this->palette());

    foreach(CG *cg, *datum->getCGs())
    {
        // Default control group is not manageable
        if(*cg->number == 1)
        {
            continue;
        }

        ui->listCGs->addItem(*cg->name);
    }
}

void ControlGroups::on_btnClose_clicked()
{
    this->close();
}

// Delete control group
void ControlGroups::on_btnDelete_clicked()
{
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(this, tr("Question"), tr("Are you sure you want to delete this control group: ") +
                                  ui->listCGs->selectedItems().at(0)->text(), QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No)
    {
        return;
    }

    int i = 0;

    // Get numbers (primary key) for control group (name is unique)
    foreach (CG *cg, *datum->getCGs())
    {
        if(cg->name == ui->listCGs->currentItem()->text())
        {
            i = *cg->number;
        }
    }

    if(i == 0)
    {
        QMessageBox::critical(this, tr("Error!"), tr("Can not find index of ") % ui->listCGs->currentItem()->text());
        return;
    }

    QSqlDatabase::database().transaction();

    QSqlQuery q;

    // Change control group to default for segments with deleted control group
    QString str1 = "UPDATE segments SET cgroup = 1 WHERE cgroup = %1;";
    QString arg1 = str1.arg(i);

    if(!q.exec(arg1))
    {
        QMessageBox::critical(this, tr("Error!"), q.lastError().text());
        return;
    }

    // Delete control group
    QString str2 = "DELETE FROM cgroups WHERE number = %1;";
    QString arg2 = str2.arg(i);

    if(!q.exec(arg2))
    {
        QMessageBox::critical(this, tr("Error!"), q.lastError().text());
        return;
    }

    QSqlDatabase::database().commit();

    this->displayCGs();

    // Now we need to update QTableWidget
    this->setProperty("update", true);
}

// Change color of control group
void ControlGroups::on_btnColor_clicked()
{
    QColor initialColor(*datum->getCG(ui->listCGs->selectedItems().at(0)->text())->color);
    QColor color = QColorDialog::getColor(initialColor, this);

    // Cancel was pressed
    if(!color.isValid())
    {
        return;
    }

    // Color must not be black
    if(color.name() == "#000000")
    {
        QMessageBox::critical(this, tr("Error!"), tr("Incorrect color!"));
        return;
    }

    QSqlQuery q;
    QString str = "UPDATE cgroups SET color='%1' WHERE name='%2';";
    QString arg = str.arg(color.name())
            .arg(ui->listCGs->selectedItems().at(0)->text());

    if(!q.exec(arg))
    {
        return;
    }

    // Update dialog
    int index = ui->listCGs->currentRow();
    this->displayCGs();
    ui->listCGs->setCurrentRow(index);
    this->displayColor();

    // Update QTableWidget when dialog closes
    this->setProperty("update", true);
}

// Add new control group
void ControlGroups::on_btnAdd_clicked()
{
    bool ok;

    QString newGroup = QInputDialog::getText(this,tr("Create new control group"),tr("Title: "), QLineEdit::Normal, "", &ok);

    if(!ok)
    {
        return;
    }

    if(newGroup.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"), tr("Incorrect title!"));
        return;
    }

    QSqlQuery q;
    QString str = "INSERT INTO cgroups (name, color) VALUES ('%1', '#FFFF33');";
    QString arg = str.arg(newGroup);

    if(!q.exec(arg))
    {
        QMessageBox::critical(this, tr("Error!"), q.lastError().text());
        return;
    }

    // Update dialog.
    this->displayCGs();
    int row = ui->listCGs->count() - 1;
    ui->listCGs->setCurrentRow(row);
    this->displayColor();

    // Update QTableWidget when dialog closes
    this->setProperty("update", true);
}

// Rename control group
void ControlGroups::on_btnRename_clicked()
{
    bool ok;

    QString newGroup = QInputDialog::getText(this,tr("Rename control group"),tr("Title: "), QLineEdit::Normal, "", &ok);

    if(!ok)
    {
        return;
    }

    if(newGroup.isEmpty())
    {
        QMessageBox::warning(this, tr("Error"), tr("Incorrect title!"));
        return;
    }

    QSqlQuery q;
    QString str = "UPDATE cgroups SET name = '%1' WHERE name = '%2';";
    QString arg = str.arg(newGroup).arg(ui->listCGs->currentItem()->text());

    if(!q.exec(arg))
    {
        QMessageBox::critical(this, tr("Error!"), q.lastError().text());
        return;
    }

    // Update dialog.
    int index = ui->listCGs->currentRow();
    this->displayCGs();
    ui->listCGs->setCurrentRow(index);
    this->displayColor();

    this->setProperty("update", true);
}

// Display control group color on a label
void ControlGroups::displayColor()
{
    foreach(CG *cg, *datum->getCGs())
    {
        if(cg->name == ui->listCGs->currentItem()->text())
        {
            QPalette pal = palette();
            pal.setColor(QPalette::Background, *cg->color);
            ui->lbColor->setPalette(pal);
        }
    }
}

// Show color when control group chosen
void ControlGroups::on_listCGs_currentTextChanged(const QString &currentText)
{
    foreach(CG *cg, *datum->getCGs())
    {
        if(cg->name == currentText)
        {
            QPalette pal = palette();
            pal.setColor(QPalette::Background, *cg->color);
            ui->lbColor->setPalette(pal);
        }
    }

    ui->btnDelete->setEnabled(true);
    ui->btnRename->setEnabled(true);
    ui->btnColor->setEnabled(true);
}
