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

// Thread for parsing RAW data

#include "addpersonthread.h"
#include <QStringBuilder>
#include <QDebug>

// function for sorting Records with std::sort
bool compareSNPs(const Record* lhs, const Record* rhs)
{
    return (*lhs->SNP < *rhs->SNP);
}

AddPersonThread::AddPersonThread(QString fileName)
{
    fileName_ = fileName;
}

void AddPersonThread::run()
{
    // Unpack RAW data file    
    QuaGzipFile gzFile(fileName_);

    if(!gzFile.open(QIODevice::ReadOnly))
    {
        emit addText(tr("File open error!") % " " % gzFile.errorString());
        emit preparationFinished(NULL); // Preparation is not successful
    }

    emit addText(tr("Extracting file..."));

    QFile outFile(QCoreApplication::applicationDirPath() % "/" % "rawdata.csv");

    if(!outFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        emit addText(tr("Can not open destination file!") % " " % outFile.errorString());
        emit preparationFinished(NULL);
    }

    QByteArray sourceData = gzFile.readAll();
    outFile.write(sourceData);
    gzFile.close();

    emit addText(tr("File is extracted. Trying to parse it..."));

    outFile.seek(0);

    QList<Record*> records;

    // Parse every line of RAW file
    while(!outFile.atEnd())
    {
        QString line = outFile.readLine();

        QRegularExpression matcher = QRegularExpression("\"rs[0-9]+\",\"([0-9]+|X)\",\"[0-9]+\",\"[ACGT-]+\"", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = matcher.match(line);

        if(match.hasMatch())
        {
            QString pureLine = match.captured().replace("\"", "");
            QStringList rawList = pureLine.split(",");

            if(rawList.count() != 4)
            {
                continue;
            }

            Record *record = new Record();

            record->SNP = new QString(rawList.at(0));
            record->chrNo = new QString(rawList.at(1));
            record->Position = new QString(rawList.at(2));
            record->Alleles = new QString(rawList.at(3));

            records.append(record);
        }
    }

    // Remove temporary file which was unpacked
    outFile.close();
    outFile.remove();

    int SNPs = records.count();

    emit addText(QString::number(SNPs) % (tr(" SNPs found. Sorting SNPs...")));

    std::sort(records.begin(), records.end(), compareSNPs);

    emit addText(tr("Sorting is done."));

    // Create QByteArray with sorted data
    QByteArray *destination = new QByteArray;

    foreach(Record *record, records)
    {
        destination->append(*record->SNP % " " % *record->chrNo % " " % *record->Position % " " % *record->Alleles % "\n");
    }

    // delete custom structure objects
    qDeleteAll(records.begin(), records.end());
    records.clear();

    // done without errors
    emit preparationFinished(destination);
}

// Destructor for structure
Record::~Record()
{
    delete SNP;
    delete chrNo;
    delete Position;
    delete Alleles;
}
