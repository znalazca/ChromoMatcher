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

#ifndef ADDPERSONDIALOG_H
#define ADDPERSONDIALOG_H

#include <QDialog>
#include <addpersonthread.h>
#include <QSettings>
#include <QtSql>
#include <QCloseEvent>

namespace Ui {
class AddPersonDialog;
}

class AddPersonDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPersonDialog(QWidget *parent = 0);
    ~AddPersonDialog();

private slots:
    void on_btnBrowse_clicked();
    void on_btnCancel_clicked();
    void on_btnAdd_clicked();
    void on_chbRawData_stateChanged(int arg1);

private:
    Ui::AddPersonDialog *ui;

    QByteArray *processedData;    
    QSettings *settings;
    AddPersonThread *thread;

    void closeDB(QSqlDatabase *dbase);
    void closeEvent(QCloseEvent * event);

public slots:
    void onTextAdded(QString info);
    void onPreparationFinished(QByteArray *destination);
};

#endif // ADDPERSONDIALOG_H
