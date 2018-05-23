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

#ifndef ADMIXTURE_H
#define ADMIXTURE_H

#include <QDialog>
#include <QPieSeries>
#include "datum.h"

namespace Ui {
class Admixture;
}

class Admixture : public QDialog
{
    Q_OBJECT

public:
    explicit Admixture(QWidget *parent = 0, Datum *d = 0);
    ~Admixture();

private slots:
    void on_btnClose_clicked();
    void on_btnCalculate_clicked();
    void calculationFinished(int);
    void on_twPercent_itemSelectionChanged();

private:
    Ui::Admixture *ui;

    Datum *datum;
    QtCharts::QPieSeries *series;
};

#endif // ADMIXTURE_H
