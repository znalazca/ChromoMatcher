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

// Dialog for manually adding a new segment or editing existing one

#include "addsegment.h"
#include "ui_addsegment.h"
#include <QMessageBox>

AddSegment::AddSegment(QDialog *parent, Datum *d) :
    QDialog(parent),
    ui(new Ui::AddSegment)
{
    ui->setupUi(this);

    // Sets if QTableWidget should be updated
    this->setProperty("update", false);

    datum = d;

    // No buttons and fixed size
    this->setFixedSize(this->width(),this->height());
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // Set initial values to zero
    initialChr = 0;
    initialPosbegin = 0;
    initialPosend = 0;

    // Populate combobox with chromosomes
    for(int i = 1; i < 24; i++)
    {
        ui->cbChromosome->addItem(QString::number(i));
    }

    // Sets spinbox range to chromosome lenth
    emit on_cbChromosome_currentTextChanged("1");

    // Add sexes
    ui->cbSex->addItem("");
    ui->cbSex->addItem("M");
    ui->cbSex->addItem("F");

    // Populate combobox with control groups
    foreach (CG *cg, *datum->getCGs())
    {
        ui->cbCG->addItem(*cg->name);
    }

    // Default color for segment
    QColor color("#FFFFFF");

    // If data is provided than we edit segment (not create a new one)
    // In such case we fill dialog with provided data
    // and make some controls read only (nonsense to edit when segment already presents).
    if(datum->Row()->count() != 0)
    {
        ui->tbNumber->setText(datum->Row()->at(Datum::Number)->text());
        ui->tbNumber->setReadOnly(true);

        emit on_cbChromosome_currentTextChanged(datum->Row()->at(Datum::ChrNo)->text());
        int chrIdx = ui->cbChromosome->findText(datum->Row()->at(Datum::ChrNo)->text());
        ui->cbChromosome->setCurrentIndex(chrIdx);
        initialChr = datum->Row()->at(Datum::ChrNo)->text().toInt();

        initialPosbegin = datum->Row()->at(Datum::Begin)->text().toInt();
        ui->sbBegin->setValue(initialPosbegin);

        initialPosend = datum->Row()->at(Datum::End)->text().toInt();
        ui->sbEnd->setValue(initialPosend);

        ui->sbCM->setValue(datum->Row()->at(Datum::CM)->text().toDouble());

        ui->sbSNP->setValue(datum->Row()->at(Datum::SNP)->text().toInt());

        ui->tbName->setText(datum->Row()->at(Datum::Name)->text());

        int sexIndex = 0;
        sexIndex = ui->cbSex->findText(datum->Row()->at(Datum::Sex)->text());
        ui->cbSex->setCurrentIndex(sexIndex);

        color = QColor(datum->Row()->at(Datum::Scheme)->foreground().color());

        ui->tbKit->setText(datum->Row()->at(Datum::Kit)->text());

        ui->tbEmail->setText(datum->Row()->at(Datum::Email)->text());

        int cgIndex = ui->cbCG->findText(datum->Row()->at(Datum::ControlGroup)->text());
        ui->cbCG->setCurrentIndex(cgIndex);

        ui->tbComment->setText(datum->Row()->at(Datum::Comment)->text());
    }

    QPalette pal = palette();
    pal.setColor(QPalette::Background, color);
    ui->lbColor->setAutoFillBackground(true);
    ui->lbColor->setPalette(pal);
}

AddSegment::~AddSegment()
{
    delete ui;
}

void AddSegment::on_btnOk_clicked()
{
    QRegularExpression r("[AMTHWEGZ]{1}[0-9]+");
    bool isMatch = r.match(ui->tbKit->text()).hasMatch();

    if((ui->sbBegin->value() >= ui->sbEnd->value())||
            ((!ui->tbKit->text().isEmpty())&&(!isMatch)))
    {
        QMessageBox::critical(this, tr("Error"), tr("These requirements must be followed: "
                                                    "end value is bigger than 1 and Begin < End, "
                                                    "Gedmatch Kit must be correct if presents."));
        return;
    }

    QString query;
    QString arg;

    // create a new segment
    if(datum->Row()->count() == 0)
    {
        query = "INSERT INTO segments ("
            "name, "
            "cgroup, "
            "sex, "
            "email, "
            "kit, "
            "comment, "
            "chrno, "
            "posbegin, "
            "posend, "
            "cm, "
            "SNP, "
            "color, "
            "added) "
            "VALUES ('%1', %2, '%3', '%4', '%5', '%6', %7, %8, %9, %10, %11, '%12', '%13');";

        arg = query.arg(ui->tbName->text())
            .arg(*datum->getCG(ui->cbCG->currentText())->number)
            .arg(ui->cbSex->currentText())
            .arg(ui->tbEmail->text())
            .arg(ui->tbKit->text())
            .arg(ui->tbComment->text())
            .arg(ui->cbChromosome->currentText())
            .arg(ui->sbBegin->value())
            .arg(ui->sbEnd->value())
            .arg(ui->sbCM->value())
            .arg(ui->sbSNP->text())
            .arg(ui->lbColor->palette().background().color().name())
            .arg(QDateTime::currentDateTime().toSecsSinceEpoch());

        // QTableWidget needs to be updated
        this->setProperty("update", true);
    }
    // update existing segment
    else
    {
        query = "UPDATE segments SET "
            "name = '%1', "
            "cgroup = %2, "
            "sex = '%3', "
            "email = '%4', "
            "kit = '%5', "
            "comment = '%6', "
            "chrno = %7, "
            "posbegin = %8, "
            "posend = %9, "
            "cm = %10, "
            "SNP = %11 "
            "WHERE number = %12;";

        arg = query.arg(ui->tbName->text())
            .arg(*datum->getCG(ui->cbCG->currentText())->number)
            .arg(ui->cbSex->currentText())
            .arg(ui->tbEmail->text())
            .arg(ui->tbKit->text())
            .arg(ui->tbComment->text())
            .arg(ui->cbChromosome->currentText())
            .arg(ui->sbBegin->value())
            .arg(ui->sbEnd->value())
            .arg(ui->sbCM->value())
            .arg(ui->sbSNP->text())
            .arg(ui->tbNumber->text());

        bool hasChrChanged = ui->cbChromosome->currentText().toInt() != initialChr;
        bool hasBeginChanged = ui->sbBegin->value() != initialPosbegin;
        bool hasEndChanged = ui->sbEnd->value() != initialPosend;

        if((hasChrChanged)||(hasBeginChanged)||(hasEndChanged))
        {
            // QTableWidget needs to be fully updated
            this->setProperty("update", true);
        }
        else
        {
            // Partially update data in MainWindow::QTableWidget
            datum->Row()->at(Datum::ChrNo)->setText(ui->cbChromosome->currentText());
            datum->Row()->at(Datum::Begin)->setText(ui->sbBegin->text());
            datum->Row()->at(Datum::End)->setText(ui->sbEnd->text());
            datum->Row()->at(Datum::CM)->setText(QString::number(ui->sbCM->value()));

            if(ui->sbSNP->value() == 0)
            {
                datum->Row()->at(Datum::SNP)->setText("");
            }
            else
            {
                datum->Row()->at(Datum::SNP)->setText(ui->sbSNP->text());
            }

            datum->Row()->at(Datum::Name)->setText(ui->tbName->text());
            datum->Row()->at(Datum::Sex)->setText(ui->cbSex->currentText());
            datum->Row()->at(Datum::Kit)->setText(ui->tbKit->text());
            datum->Row()->at(Datum::Email)->setText(ui->tbEmail->text());
            datum->Row()->at(Datum::ControlGroup)->setText(ui->cbCG->currentText());
            datum->Row()->at(Datum::Comment)->setText(ui->tbComment->text());

            // Update control group in MainWindow::QTableWidget
            CG *cg = datum->getCG(ui->cbCG->currentText());
            QColor color(*cg->color);

            for(int i = 0; i < datum->Row()->count(); i++)
            {
                datum->Row()->at(i)->setBackgroundColor(color);
            }
        }
    }

    QSqlQuery q;

    if(!q.exec(arg))
    {
        QMessageBox::critical(this, tr("Error!"), q.lastError().text());
    }
    else
    {
        QMessageBox::information(this, tr("Success!"), tr("Segment successfully imported/updated."));

        this->close();
    }
}

void AddSegment::on_btnClose_clicked()
{
    this->close();
}

// Sets spinbox range for selected chromosome
void AddSegment::on_cbChromosome_currentTextChanged(const QString &arg1)
{
    if(arg1 == "1") {ui->sbBegin->setMaximum(249250621);ui->sbEnd->setMaximum(249250621);}
    if(arg1 == "2") {ui->sbBegin->setMaximum(243199373);ui->sbEnd->setMaximum(243199373);}
    if(arg1 == "3") {ui->sbBegin->setMaximum(198022430);ui->sbEnd->setMaximum(198022430);}
    if(arg1 == "4") {ui->sbBegin->setMaximum(191154276);ui->sbEnd->setMaximum(191154276);}
    if(arg1 == "5") {ui->sbBegin->setMaximum(180915260);ui->sbEnd->setMaximum(180915260);}
    if(arg1 == "6") {ui->sbBegin->setMaximum(171115067);ui->sbEnd->setMaximum(171115067);}
    if(arg1 == "7") {ui->sbBegin->setMaximum(159138663);ui->sbEnd->setMaximum(159138663);}
    if(arg1 == "8") {ui->sbBegin->setMaximum(146364022);ui->sbEnd->setMaximum(146364022);}
    if(arg1 == "9") {ui->sbBegin->setMaximum(141213431);ui->sbEnd->setMaximum(141213431);}
    if(arg1 == "10") {ui->sbBegin->setMaximum(135534747);ui->sbEnd->setMaximum(135534747);}
    if(arg1 == "11") {ui->sbBegin->setMaximum(135006516);ui->sbEnd->setMaximum(135006516);}
    if(arg1 == "12") {ui->sbBegin->setMaximum(133851895);ui->sbEnd->setMaximum(133851895);}
    if(arg1 == "13") {ui->sbBegin->setMaximum(115169878);ui->sbEnd->setMaximum(115169878);}
    if(arg1 == "14") {ui->sbBegin->setMaximum(107349540);ui->sbEnd->setMaximum(107349540);}
    if(arg1 == "15") {ui->sbBegin->setMaximum(102531392);ui->sbEnd->setMaximum(102531392);}
    if(arg1 == "16") {ui->sbBegin->setMaximum(90354753);ui->sbEnd->setMaximum(90354753);}
    if(arg1 == "17") {ui->sbBegin->setMaximum(81195210);ui->sbEnd->setMaximum(81195210);}
    if(arg1 == "18") {ui->sbBegin->setMaximum(78077248);ui->sbEnd->setMaximum(78077248);}
    if(arg1 == "19") {ui->sbBegin->setMaximum(59128983);ui->sbEnd->setMaximum(59128983);}
    if(arg1 == "20") {ui->sbBegin->setMaximum(63025520);ui->sbEnd->setMaximum(63025520);}
    if(arg1 == "21") {ui->sbBegin->setMaximum(48129895);ui->sbEnd->setMaximum(48129895);}
    if(arg1 == "22") {ui->sbBegin->setMaximum(51304566);ui->sbEnd->setMaximum(51304566);}
    if(arg1 == "23") {ui->sbBegin->setMaximum(155270560);ui->sbEnd->setMaximum(155270560);}
}
