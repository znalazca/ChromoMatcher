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

#include "admixture.h"
#include "ui_admixture.h"
#include <QMessageBox>
#include <QChart>
#include <QChartView>
#include <QStringBuilder>

using namespace QtCharts;

Admixture::Admixture(QWidget *parent, Datum *d) :
    QDialog(parent),
    ui(new Ui::Admixture)
{
    ui->setupUi(this);

    // Dialog with fixed size and no data
    this->setFixedSize(this->width(),this->height());
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    datum = d;

    // Match name as window title
    this->setWindowTitle(datum->Row()->at(Datum::Name)->text());

    // File with weights as title of a calculator
    QStringList nameFilter("*.F");
    QDir directory(Datum::path());
    QStringList files = directory.entryList(nameFilter);

    int index = 0;

    foreach(QString file, files)
    {
        QFileInfo info(file);
        ui->cbCalculator->addItem(info.completeBaseName());

        if(datum->Settings()->value("Calculator") == info.completeBaseName())
        {
            index = ui->cbCalculator->count() - 1;
        }
    }

    // Have to set index after populating combobox because text can't be set during population
    ui->cbCalculator->setCurrentIndex(index);

    // Prepare QTableWidget for admixture information
    ui->twPercent->setColumnCount(2);
    // Allow selection of entire row only
    ui->twPercent->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->twPercent->setSelectionMode(QAbstractItemView::SingleSelection);
    // Set headers
    ui->twPercent->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Population")));
    ui->twPercent->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("%")));
    ui->twPercent->setColumnWidth(0, 120);
    ui->twPercent->setColumnWidth(1, 40);
    // Stretch last section
    ui->twPercent->horizontalHeader()->setStretchLastSection(true);
    ui->twPercent->horizontalHeader()->hide();
}

Admixture::~Admixture()
{
    delete ui;
}

void Admixture::on_btnClose_clicked()
{
    this->close();
}

void Admixture::on_btnCalculate_clicked()
{
    // disable calculator groupbox because it should be untouched when other process works
    ui->gbCalculator->setEnabled(false);

    // Block signals while QTableWidget is being populated
    ui->twPercent->blockSignals(true);

    // Save default calculator in settings
    QString calculator = ui->cbCalculator->currentText();
    datum->Settings()->setValue("Calculator", calculator);

    // Prepare source file names for Dodecad
    QString txt = calculator.split('.').at(0) % ".txt";
    QString weights = calculator % ".F";
    QString allele = calculator.split('.').at(0) % ".alleles";

    // Create file with params for Dodecad
    QFile par(Datum::path() % "start.par");
    if(!par.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QMessageBox::critical(this, tr("Error!"), par.errorString());
        ui->gbCalculator->setEnabled(true);
        return;
    }

    QTextStream stream(&par);

    // Write params to the file.
    stream << "1d-7" << endl;
    stream << calculator.split('.').at(1) << endl;
    stream << *datum->Person() << ".txt" << endl;

    QFile allelesFile(Datum::path() % allele);
    if(!allelesFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, tr("Error!"), allelesFile.errorString());
        ui->gbCalculator->setEnabled(true);
        return;
    }

    int lines = 0;

    while(!allelesFile.atEnd())
    {
        allelesFile.readLine();
        lines++;
    }

    allelesFile.close();

    stream << lines << endl;
    stream << txt << endl;
    stream << weights << endl;
    stream << allele << endl;
    stream << "silent" << endl;
    stream << "target" << endl;
    stream << datum->Row()->at(Datum::ChrNo)->text() << endl; // chromosome number
    stream << datum->Row()->at(Datum::Begin)->text() << endl; // begin
    stream << datum->Row()->at(Datum::End)->text() << endl; // end

    par.close();    

    // Start Dodecad process
    QProcess *process = new QProcess(this);
    connect(process, SIGNAL(finished(int)), this, SLOT(calculationFinished(int)));
    process->setWorkingDirectory(Datum::path());

    QString platform = QSysInfo::kernelType();

    if(platform == "winnt")
    {
        process->setProgram(Datum::path() % "DIYDodecadWin.exe");
    }
    else if(platform == "linux")
    {
        process->setProgram(Datum::path() % "DIYDodecadLinux32");
    }
    else
    {
        QMessageBox::critical(this, tr("Error!"), tr("Unknown platform: ") % platform % tr(" Can not start Dodecad."));
        par.remove();
        ui->gbCalculator->setEnabled(true);
    }

    QStringList arg;
    arg.append("start.par");
    process->setArguments(arg);
    process->start();
}

// Dodecad calculation finished
void Admixture::calculationFinished(int result)
{
    // Delete file with Dodecad params
    QFile::remove(Datum::path() % "start.par");

    QStringList poplist;
    QString tg;

    try
    {
        // Dodecad finished with error
        if(result != 0)
        {
            throw tr("Process finished with error code: ") % QString::number(result);
        }

        // Can't read file with admixture data
        QFile popfile(Datum::path() % ui->cbCalculator->currentText().split(".").at(0) % ".txt");
        if(!popfile.open(QIODevice::ReadOnly))
        {
            throw popfile.errorString();
        }

        // Prepare population list
        while(!popfile.atEnd())
        {
            poplist.append(popfile.readLine());
        }

        popfile.close();

        // Find percentage for population
        QFile target(Datum::path() % "target.txt");
        if(!target.open(QIODevice::ReadOnly))
        {
            throw(target.errorString());
        }

        target.readLine();
        tg = target.readLine();

        target.close();
        target.remove();
    }
    catch(QString e)
    {
        QMessageBox::critical(this, tr("Segment may not include less than 10 SNP!"), e);
        ui->gbCalculator->setEnabled(true);
        return;
    }

    QStringList adlist = tg.split(QRegExp("\\ +")); // split second line by spaces

    for(int i = 0; i < 6; i++)
    {
        adlist.removeFirst(); // First record will be empty because of double space at the beginning of a line
    }

    // Create QPieSeries and populate it with population name and %
    series = new QPieSeries();

    // Clear previously filled QTableWidget (if any)
    ui->twPercent->setRowCount(0);

    for(int i = 0; i < poplist.count(); i++)
    {
        double val = adlist.at(i).toDouble();
        series->append(poplist.at(i), val);

        // Populate QTableView with admixture information
        ui->twPercent->insertRow(i);

        QTableWidgetItem *itemPop = new QTableWidgetItem();
        itemPop->setFlags(itemPop->flags() ^ Qt::ItemIsEditable);
        itemPop->setText(poplist.at(i));
        QTableWidgetItem *itemPercent = new QTableWidgetItem();
        itemPercent->setFlags(itemPercent->flags() ^ Qt::ItemIsEditable);
        itemPercent->setText(QString::number(val));

        ui->twPercent->setItem(i, 0, itemPop);
        ui->twPercent->setItem(i, 1, itemPercent);

        if(val != 0)
        {
            QPieSlice *slice = series->slices().at(i);
            slice->setObjectName(poplist.at(i));
            slice->setLabelVisible(); // Set population name visible at eash slice
        }
    }

    // Display chart
    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(series);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QLayoutItem *child;
    while((child = ui->gridLayout->takeAt(0)) != 0)
    {
        delete child->widget();
        delete child;
    }

    // Add chart to dialog
    ui->gridLayout->addWidget(chartView);
    // Show header of QTableWidget
    ui->twPercent->horizontalHeader()->show();
    // Release QTableWidget signals
    ui->twPercent->blockSignals(false);
    // Ready for next calculations
    ui->gbCalculator->setEnabled(true);
}

// Explode selected slice
void Admixture::on_twPercent_itemSelectionChanged()
{
    // Get selection
    QItemSelection selection(ui->twPercent->selectionModel()->selection());
    QList<int> rows = datum->getSelectedRows(&selection);

    // Get name of a selected population
    QString name = ui->twPercent->item(rows.at(0), 0)->text();

    // Search for slice with selected name and explode it
    foreach(QPieSlice *slice, series->slices())
    {
        if(name == slice->objectName())
        {
            slice->setExploded();
        }
        else
        {
            // Hide explosion if not needed
            slice->setExploded(false);
        }
    }
}

