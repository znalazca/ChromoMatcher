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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTranslator>
#include <QSettings>
#include <QtSql>
#include <QCloseEvent>
#include <QRect>
#include "addsegment.h"
#include "datum.h"
#include <QTableWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void getPersons();
    bool setPerson(QString person);

private slots:
    void on_actionEnglish_triggered();
    void on_actionRussian_triggered();
    void on_actionAdd_triggered();
    void on_actionExit_triggered();
    void on_personSelected_triggered();
    void on_actionClosePerson_triggered();
    void on_actionDelete_triggered();
    void on_actionRename_triggered();
    void on_actionAbout_triggered();
    void on_actionImport_triggered();
    void on_actionAdd_Manual_triggered();
    void on_actionManage_Control_Groups_triggered();
    void on_tableWidget_itemChanged(QTableWidgetItem *item);
    void deleteSegments();
    void dialogIsFinished();
    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);
    void on_tableWidget_customContextMenuRequested(const QPoint &pos);
    void cgSelected();
    void showAdmixture();
    void editSegment();
    void on_actionFind_triggered();
    void menuHidden();
    void on_actionManual_triggered(); // to implement
    void selectDate();
    void selectSource();
    void selectKit();
    void selectCG();
    void on_tableWidget_itemSelectionChanged();

private:
    Ui::MainWindow *ui;

    QTranslator* translator;
    void changeTranslator(QString postfix);
    void updateUI();
    void changeEvent(QEvent *event);
    void closeEvent(QCloseEvent *event);
    QRect getRectangle(int chr, int begin, int end);
    QList <QTableWidgetItem*>* getSelectedRow(int row);
    void selectRows(int row);

    QSettings *settings;
    Datum *datum;
};

#endif // MAINWINDOW_H
