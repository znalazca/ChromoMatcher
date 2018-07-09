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

#include "import.h"
#include "ui_import.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QShortcut>

Import::Import(QDialog *parent, Datum *d) :
    QDialog(parent),
    ui(new Ui::Import)
{
    ui->setupUi(this);

    // No need to update QTableWidget yet
    this->setProperty("update", false);

    datum = d;

    // Fixed size and no buttons
    this->setFixedSize(this->width(),this->height());
    this->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    // Default value of cm
    if(datum->Settings()->value("MinimumCM") != QVariant::Invalid)
    {
        ui->sbSegment->setValue(datum->Settings()->value("MinimumCM").toInt());
    }

    ui->listImport->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Allow drop down files
    setAcceptDrops(true);

    // Shortcut to delete files in listWidget. Static analysis tools may point here
    // but it is not a problem because QShortcut will be deleted with listImport anyway
    QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->listImport);
    connect(shortcut, SIGNAL(activated()), this, SLOT(on_btnDelete_clicked()));
}

Import::~Import()
{
    delete ui;
}

void Import::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void Import::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void Import::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void Import::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.size(); i++)
        {
            QString fileName = urlList.at(i).toLocalFile();
            QString fileExtension = fileName.split(".").last();

            bool validExtension = fileExtension == "csv" || fileExtension == "htm" || fileExtension == "html";

            if(validExtension)
            {
                pathList.append(urlList.at(i).toLocalFile());
            }
        }

        ui->listImport->addItems(pathList);
    }
}

void Import::on_btnImport_clicked()
{
    // QList with all segments in all files
    QList<Segment*> segments;
    int count = 0;

    for(int i = 0; i < ui->listImport->count(); i++)
    {
        QString fileName = ui->listImport->item(i)->text();
        QFile file(fileName);

        if(!file.exists())
        {
            this->setStatus(i, false, tr("not found!"));
        }
        else if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            this->setStatus(i, false, file.errorString());
        }
        else if(fileName.split(".").last() == "csv")
        {
            this->CSV(file, segments);
        }
        else
        {
            QString firstLine = file.readLine();

            if(firstLine.contains("<html><head>"))
            {
                this->matchingSegment(file, segments);
            }
            else
            {
                this->twoOrMore(file, segments);
            }
        }

        file.close();

        int segmentsFound = segments.count() - count;
        if(segmentsFound == 0)
        {
            this->setStatus(i, false, tr("0 segments found!"));
        }
        else
        {
            this->setStatus(i, true, QString::number(segmentsFound) % tr(" segments found."));
            count = segments.count();
        }
    }

    datum->DB()->transaction();

    foreach(Segment *segment, segments)
    {
        if(*segment->cm >= ui->sbSegment->value())
        {
            QString insert;

            if(ui->rbIgnore->isChecked())
            {
                insert = "INSERT INTO segments ("
                        "name, "
                        "source, "
                        "chrno, "
                        "posbegin, "
                        "posend, "
                        "cm, "
                        "SNP, "
                        "color, "
                        "sex, "
                        "email, "
                        "cgroup, "
                        "comment, "
                        "added, "
                        "kit) "
                        "VALUES ('%1', '%2', %3, %4, %5, %6, %7, '%8', '%9', '%10', 1, '', %11, '%12');";
            }
            else if(ui->rbOverwrite->isChecked())
            {
                insert = "INSERT OR REPLACE INTO segments ("
                        "number, "
                        "name, "
                        "source, "
                        "chrno, "
                        "posbegin, "
                        "posend, "
                        "cm, "
                        "SNP, "
                        "color, "
                        "sex, "
                        "email, "
                        "cgroup, "
                        "comment, "
                        "added, "
                        "kit) "
                        "VALUES ("
                        "(SELECT number FROM segments WHERE name = '%1' AND posbegin = %4 AND posend = %5 AND chrno = %3 AND kit='%12'), "
                        "'%1', '%2', %3, %4, %5, %6, %7, '%8', '%9', '%10', "
                        "(SELECT cgroup FROM segments WHERE name = '%1' AND posbegin = %4 AND posend = %5 AND chrno = %3 AND kit='%12'), "
                        "(SELECT comment FROM segments WHERE name = '%1' AND posbegin = %4 AND posend = %5 AND chrno = %3 AND kit='%12'), "
                        "%11, '%12');";
            }
            else if(ui->rbPreserve->isChecked())
            {
                insert = "INSERT OR IGNORE INTO segments ("
                        "number, "
                        "name, "
                        "source, "
                        "chrno, "
                        "posbegin, "
                        "posend, "
                        "cm, "
                        "SNP, "
                        "color, "
                        "sex, "
                        "email, "
                        "cgroup, "
                        "comment, "
                        "added, "
                        "kit) "
                        "VALUES ("
                        "(SELECT number FROM segments WHERE name = '%1' AND posbegin = %4 AND posend = %5 AND chrno = %3 AND kit='%12'), "
                        "'%1', '%2', %3, %4, %5, %6, %7, '%8', '%9', '%10', 1, '', %11, '%12');";
            }

            QString arg = insert.arg(segment->name->replace("'", "''"))
                    .arg(*segment->source)
                    .arg(*segment->chrno)
                    .arg(*segment->posbegin)
                    .arg(*segment->posend)
                    .arg(*segment->cm)
                    .arg(segment->SNP != nullptr ? *segment->SNP : 0)
                    .arg(segment->color != nullptr ? *segment->color : "#FFFFFF")
                    .arg(segment->sex != nullptr ? *segment->sex : "")
                    .arg(segment->email != nullptr ? *segment->email : "")
                    .arg(QDateTime::currentDateTime().toSecsSinceEpoch())
                    .arg(segment->kit != nullptr ? *segment->kit : "");

            QSqlQuery q;

            if(!q.exec(arg))
            {
                QMessageBox::critical(this, tr("Error!"), q.lastError().text());
                datum->DB()->rollback();
                count = 0;
                break;
            }
        }
    }

    qDeleteAll(segments.begin(), segments.end());
    segments.clear();

    datum->DB()->commit();

    QMessageBox::information(this, tr("Finished."), QString::number(count) % tr(" segments processed."));

    // Set minimum segment length in settings
    datum->Settings()->setValue("MinimumCM", ui->sbSegment->value());

    // Now need to update QTableWidget
    this->setProperty("update", true);

    ui->gbDuplicates->setEnabled(false);
    ui->btnImport->setEnabled(false);
    ui->btnDelete->setEnabled(false);
    ui->btnBrowse->setEnabled(false);
    this->setAcceptDrops(false);
}

// sets status of file processing
void Import::setStatus(int i, bool isOk, QString text)
{
    if(isOk)
    {
        ui->listImport->item(i)->setTextColor(Qt::darkGreen);
    }
    else
    {
        ui->listImport->item(i)->setTextColor(Qt::red);
    }

    QString fileName = ui->listImport->item(i)->text();
    ui->listImport->item(i)->setText(fileName % " - " % text);
}

void Import::on_btnBrowse_clicked()
{
    // Select multiple files for import
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select files with segments"), datum->Settings()->value("Matches_Path").toString(), tr("Web page files or CSV (*.htm *.html *.csv)"));

    if(fileNames.count() == 0)
    {
        return;
    }

    // Set default path in settings
    QFileInfo fileInfo(fileNames.at(0));
    datum->Settings()->setValue("Matches_Path", fileInfo.path());

    ui->listImport->addItems(fileNames);
}

// Process CSV file from FTDNA of MyHeritage
void Import::CSV(QFile &file, QList<Segment*> &segments)
{
    // index for parsing different sources
    int n = 5;

    file.readLine();

    while(!file.atEnd())
    {
        QString line = file.readLine();
        QStringList records = line.split(QRegExp(",(?=(?:(?:[^\"]*\"){2})*[^\"]*$)")); // comma not in "

        int source;

        if(records.count() == 7) // 7 - FTDNA, 9 - MyHeritage
        {
            source = FTDNA;
        }
        else if(records.count() == 9)
        {
            n = 7;
            source = MyHeritage;
        }
        else
        {
            continue; // Parsing error
        }

        Segment *segment = new Segment();

        segment->source = new int(source);

        QString name = records.at(1);
        name = name.replace("\"", "");
        segment->name = new QString(name);

        // Replace X with number 23 for X chromosome
        if(records.at(2) == "X")
        {
            segment->chrno = new int(23);
        }
        else
        {
            segment->chrno = new int(records.at(2).toInt());
        }

        segment->posbegin = new int(records.at(3).toInt());
        segment->posend = new int(records.at(4).toInt());

        segment->cm = new double(records.at(n).toDouble());
        segment->SNP = new int(records.at(n+1).toInt());

        segments.append(segment);
    }
}

// Gedmatch matching segment parsing
void Import::matchingSegment(QFile &file, QList<Segment*> &segments)
{
    Segment *segment;

    while(!file.atEnd())
    {
        QString line = file.readLine();

        // Get kit
        QRegularExpression matcher(">[AMTHWEGZ]{1}[0-9]+<");
        if(matcher.match(line).hasMatch())
        {
            QStringList records = line.split(QRegExp("<\\/td>"));
            QStringList pureRecords;

            foreach (QString record, records)
            {
                // Leave only useful data and preserve compatibility with older saved pages
                QString pureRecord = record.replace(QRegExp("<tr>|<td align=[\"]*center[\"]*>|<\\/td>|,|<td>"), "");
                pureRecords.append(pureRecord);
            }

            if(pureRecords.count() != 10)
            {
                continue; // parsing error
            }

            segment = new Segment();

            segment->source = new int(Tier1);
            segment->kit = new QString(pureRecords.at(0));
            segment->chrno = new int(pureRecords.at(1).toInt());
            segment->posbegin = new int(pureRecords.at(2).toInt());
            segment->posend = new int(pureRecords.at(3).toInt());
            segment->cm = new double(pureRecords.at(4).toDouble());
            segment->SNP = new int(pureRecords.at(5).toInt());
            segment->name = new QString(pureRecords.at(6));
            segment->sex = new QString(pureRecords.at(7));

            QString email = pureRecords.at(8);
            email = email.replace("'", "''");

            segment->email = new QString(email);

            // Get color
            line = file.readLine();

            matcher.setPattern("#[A-Z0-9]{6}");
            if(matcher.match(line).hasMatch())
            {
                segment->color = new QString(matcher.match(line).captured());
                segments.append(segment);
            }
            else
            {
                delete segment;
            }
        }
    }
}

// Gedmatch people who match 2 or more kits parse
void Import::twoOrMore(QFile &file, QList<Segment*> &segments)
{
    Segment *segment;

    int chrno = 1;
    QString name;
    QString kit;

    while(!file.atEnd())
    {
        QString line = file.readLine();

        // Get chromosome
        QRegularExpression matcher("Chr [0-9]+<br><table");
        if(matcher.match(line).hasMatch())
        {
            chrno = matcher.match(line).captured().replace(QRegExp("Chr |<br><table"), "").toInt();
            continue; // nothing else to parse on this line
        }
        // Get name - it may be empty
        matcher.setPattern("<td>.*<br>");
        if(matcher.match(line).hasMatch())
        {
            name = matcher.match(line).captured().replace(QRegularExpression("<td>|<br>"), "");
        }
        // Get kit
        matcher.setPattern("kit_num=[AMTHWEGZ]{1}[0-9]+");
        if(matcher.match(line).hasMatch())
        {
            kit = matcher.match(line).captured().replace("kit_num=", "");
            continue; // nothing else to parse on this line
        }
        // Get positions and cm
        matcher.setPattern("[0-9]+ - [0-9]+.+cM\\)");
        if(matcher.match(line).hasMatch())
        {
            QString pureline = matcher.match(line).captured().replace(" ", "");
            QStringList lines = pureline.split(",");

            for(int i = 0; i < lines.count(); i++)
            {
                QString newline = lines.at(i);

                segment = new Segment();

                segment->posbegin = new int(newline.split("-").at(0).toInt());
                segment->posend = new int(newline.split("-").at(1).split("(").at(0).toInt());
                segment->cm = new double(newline.replace(QRegExp("[0-9]+-[0-9]+\\(|cM\\)"), "").toDouble());

                segment->chrno = new int(chrno);
                segment->name = new QString(name);
                segment->source = new int(TwoOrMore);
                segment->kit = new QString(kit);

                segments.append(segment);

                if((i + 1) < lines.count())
                {
                    chrno = *segments.last()->chrno;
                    name = *segments.last()->name;
                    kit = *segments.last()->kit;
                }
            }
        }
    }
}

void Import::on_btnCancel_clicked()
{
    this->close();
}

void Import::on_btnDelete_clicked()
{
    if(ui->btnDelete->isEnabled()) // Ignore shortcut if button is not enabled
    {
        qDeleteAll(ui->listImport->selectedItems());
    }
}

