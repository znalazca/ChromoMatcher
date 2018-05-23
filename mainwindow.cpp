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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QEvent>
#include <QDebug>
#include <QLocale>
#include "addpersondialog.h"
#include "about.h"
#include "import.h"
#include "controlgroups.h"
#include "admixture.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QStandardItem>
#include <QPainter>
#include <QtGlobal>
#include <QShortcut>
#include <QStringBuilder>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Prepare settings object
    QString settingsFile = Datum::path() % "settings.ini";
    settings = new QSettings(settingsFile, QSettings::IniFormat);

    // Get locale and window position from settings
    QString sysLocale = settings->value("Locale").toString();

    if(sysLocale.isEmpty())
    {
        sysLocale = QLocale::system().name();
        settings->setValue("Locale", sysLocale);
    }

    if(sysLocale.contains("ru", Qt::CaseInsensitive))
    {
        ui->actionRussian->setChecked(true);
    }
    else
    {
        ui->actionEnglish->setChecked(true);
    }

    translator = new QTranslator(this);
    changeTranslator(sysLocale);

    // Set window size and position according settings
    if(settings->value("isMaximized").toBool())
    {
        this->setWindowState(Qt::WindowMaximized);
    }
    else
    {
        this->resize(settings->value("Width").toInt(), settings->value("Height").toInt());
        this->move(QPoint(settings->value("X").toInt(), settings->value("Y").toInt()));
    }

    // Prepare QTableWidget

    // Del key for QTableWidget. No need to delete this shortcut because ui->tableWidget never destroys
    // until program works.
    QShortcut* shortcut = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableWidget);
    connect(shortcut, SIGNAL(activated()), this, SLOT(deleteSegments()));
    // Context menu for QTableWidget
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    // Allow selection of entire rows only
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Stretch last section
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setColumnCount(17);

    // Hide header while no person opened
    ui->tableWidget->horizontalHeader()->hide();

    // Set up header columns
    ui->tableWidget->setHorizontalHeaderItem(Datum::ChrNo, new QTableWidgetItem(tr("Chr")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Begin, new QTableWidgetItem(tr("Begin")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::End, new QTableWidgetItem(tr("End")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::CM, new QTableWidgetItem(tr("cm")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::SNP, new QTableWidgetItem(tr("SNP")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Name, new QTableWidgetItem(tr("Name")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Sex, new QTableWidgetItem(tr("Sex")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Scheme, new QTableWidgetItem(tr("Scheme")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Source, new QTableWidgetItem(tr("Source", "Header")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Kit, new QTableWidgetItem(tr("Kit", "Header")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Email, new QTableWidgetItem(tr("Email")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::ControlGroup, new QTableWidgetItem(tr("Control Group", "Header")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Comment, new QTableWidgetItem(tr("Comment")));
    ui->tableWidget->setHorizontalHeaderItem(Datum::Added, new QTableWidgetItem(tr("Imported at")));

    if(!settings->contains("Columns/1"))
    {
        // Set default values
        settings->setValue("Columns/1", 30);
        settings->setValue("Columns/2", 100);
        settings->setValue("Columns/3", 100);
        settings->setValue("Columns/4", 70);
        settings->setValue("Columns/5", 50);
        settings->setValue("Columns/7", 130);
        settings->setValue("Columns/8", 30);
        settings->setValue("Columns/9", 200);
        settings->setValue("Columns/10", 75);
        settings->setValue("Columns/11", 75);
        settings->setValue("Columns/12", 130);
        settings->setValue("Columns/13", 130);
        settings->setValue("Columns/14", 130);
        settings->setValue("Columns/16", 130);
    }

    for(int i = 0; i < ui->tableWidget->columnCount(); i++)
    {
        int width = settings->value("Columns/" % QString::number(i)).toInt();

        ui->tableWidget->setColumnWidth(i, width);
    }

    ui->tableWidget->setColumnHidden(Datum::Number, true);
    ui->tableWidget->setColumnHidden(Datum::Color, true);
    ui->tableWidget->setColumnHidden(Datum::CGroupColor, true);

    // Reserved for future
    ui->mainToolBar->setVisible(false);

    // Get persons
    this->getPersons();

    // Create directory for data if not exists
    QDir dir(Datum::path());
    if(!dir.exists())
    {
        dir.mkdir(dir.path());
    }

    datum = NULL;
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Save window size and position in settings
void MainWindow::closeEvent (QCloseEvent *event)
{
    settings->setValue("Width", this->geometry().width());
    settings->setValue("Height", this->geometry().height());
    settings->setValue("isMaximized", this->isMaximized());
    settings->setValue("X", this->pos().x());
    settings->setValue("Y", this->pos().y());

    for(int i = 0; i < ui->tableWidget->columnCount(); i++)
    {
        int width = ui->tableWidget->columnWidth(i);

        settings->setValue("Columns/" % QString::number(i), width);
    }

    event->accept();
}

// Get persons and add them to menuperson
void MainWindow::getPersons()
{
    // Clear persons list in menu
    int actionsnum = ui->menuPerson->actions().size();

    for(int i = actionsnum - 1; i > 3; i--)
    {
        QAction *action = ui->menuPerson->actions().at(i);
        ui->menuPerson->removeAction(action);
    }

    ui->menuPerson->addSeparator();

    QDir dir(Datum::path());

    QStringList filters;
    filters.append("*.sqlite");

    QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);

    foreach(QString file, files)
    {
        QFileInfo fileinfo(file);
        QString name = fileinfo.completeBaseName();
        ui->menuPerson->addAction(name, this, SLOT(on_personSelected_triggered()));
    }

    if(files.count() != 0)
    {
        ui->menuPerson->addSeparator();
    }

    ui->menuPerson->addAction(tr("Exit"), this, SLOT(on_actionExit_triggered()));
}

// When person is selected need to populate QTableWidget with data and create Datum object for child dialogs
bool MainWindow::setPerson(QString person)
{
    if(datum != NULL)
    {
        delete datum;
    }

    datum = new Datum(person, settings);

    ui->tableWidget->setRowCount(0);
    ui->tableWidget->horizontalHeader()->show();

    QSqlQuery *q = new QSqlQuery("SELECT "
            "a.number, "
            "a.chrno, "
            "a.posbegin, "
            "a.posend, "
            "a.cm, "
            "a.SNP, "
            "a.color, "
            "a.name, "
            "a.sex, "
            "'', "
            "c.name source, "
            "a.kit, "
            "a.email, "
            "b.name cgroupname, "
            "a.comment, "
            "b.color cgroupcolor, "
            "added "
        "FROM segments a "
        "JOIN cgroups b ON a.cgroup = b.number "
        "JOIN sources c ON a.source = c.number "
        "ORDER BY chrno, posbegin;");

    if(!q->isSelect())
    {
        QMessageBox::critical(this, tr("Error!"), q->lastError().text());
        return false;
    }

    int row = 0;

    // Disable signals during population
    ui->tableWidget->blockSignals(true);

    while(q->next())
    {
        ui->tableWidget->insertRow(row);

        for(int col = 0; col < ui->tableWidget->columnCount(); col++)
        {
            QTableWidgetItem *item = new QTableWidgetItem();

            if(col != Datum::Comment) // Comment is editable
            {
                item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            }

            // Background color depends on control group
            QColor color(q->value(Datum::CGroupColor).toString());
            item->setBackgroundColor(color);

            if(col == Datum::Added) // Convert datetime to readable text
            {
                item->setText(QDateTime::fromSecsSinceEpoch(q->value(Datum::Added).toLongLong()).toString());
            }
            else if((col == Datum::SNP)&&(q->value(Datum::SNP) == "0")) // SNP should be blank if it is 0
            {
                item->setText("");
            }
            else if(col == Datum::Scheme) // image
            {
                QPixmap pix(190,25);
                pix.fill(Qt::black);
                QPainter painter(&pix);
                QColor color(q->value(Datum::Color).toString());
                QRect rect = this->getRectangle(q->value(Datum::ChrNo).toInt(),
                                                q->value(Datum::Begin).toInt(),
                                                q->value(Datum::End).toInt());
                painter.fillRect(rect, color);

                item->setForeground(color); // needed for edit segment dialog
                item->setData(Qt::DecorationRole, pix);
            }
            else
            {
                item->setText(q->value(col).toString());
            }

            ui->tableWidget->setItem(row, col, item);
        }
        row++;
    }

    delete q;

    // Population is done - accept signals now
    ui->tableWidget->blockSignals(false);

    this->setWindowTitle("ChromoMatcher - " % *datum->Person());

    // Menu actions become active when person is selected
    ui->actionRename->setEnabled(true);
    ui->actionDelete->setEnabled(true);
    ui->actionClosePerson->setEnabled(true);

    ui->actionImport->setEnabled(true);
    ui->actionAdd_Manual->setEnabled(true);
    ui->actionFind->setEnabled(true);
    ui->actionManage_Control_Groups->setEnabled(true);

    return true;
}

// Context menu for segments
void MainWindow::on_tableWidget_customContextMenuRequested(const QPoint &pos)
{
    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    QList<int> numbers = datum->getSelectedNumbers(&selection);

    if(numbers.count() == 0)
    {
        return;
    }

    // One menu is general, second is for assigning control group
    QMenu* menu = new QMenu(this);
    connect(menu, SIGNAL(aboutToHide()), this, SLOT(menuHidden()));

    QMenu *menuCGs = new QMenu(menu);
    menuCGs->setTitle(tr("Control Group"));

    foreach(CG *cg, *datum->getCGs())
    {
        QAction *action = new QAction(*cg->name, menuCGs);
        connect(action, SIGNAL(triggered()), this, SLOT(cgSelected()));
        menuCGs->addAction(action);
    }

    menu->addMenu(menuCGs);

    QAction *del = new QAction(tr("Delete..."), menu);
    connect(del, SIGNAL(triggered()), this, SLOT(deleteSegments()));
    menu->addAction(del);

    // You can edit, show admixture or mass select date/source segments only for single segment selection
    if(numbers.count() == 1)
    {
        menu->addSeparator();

        QAction *admixture = new QAction(tr("Admixture..."), menu);
        connect(admixture, SIGNAL(triggered()), this, SLOT(showAdmixture()));
        admixture->setEnabled(datum->Admixture()); // if admixture information is available
        menu->addAction(admixture);

        QAction *edit = new QAction(tr("Edit..."), menu);
        connect(edit, SIGNAL(triggered()), this, SLOT(editSegment()));
        menu->addAction(edit);

        QMenu *menuSelect = new QMenu(menu);
        menuSelect->setTitle(tr("Select All of the Same"));
        menu->addMenu(menuSelect);

        QAction *allDate = new QAction(tr("Date and Time"), menuSelect);
        connect(allDate, SIGNAL(triggered()), this, SLOT(selectDate()));
        menuSelect->addAction(allDate);

        QAction *allSource = new QAction(tr("Source", "Menu"), menuSelect);
        connect(allSource, SIGNAL(triggered()), this, SLOT(selectSource()));
        menuSelect->addAction(allSource);

        int row = datum->getSelectedRows(&selection).at(0);
        QString cg = ui->tableWidget->item(row, Datum::ControlGroup)->text();
        bool cgCondition = cg != "";

        if(cgCondition)
        {
            QAction *allCG = new QAction(tr("Control Group", "Menu"), menuSelect);
            connect(allCG, SIGNAL(triggered()), this, SLOT(selectCG()));
            menuSelect->addAction(allCG);
        }

        QString kit = ui->tableWidget->item(row, Datum::Kit)->text();        
        bool kitCondition = QRegularExpression("[AMTHWEGZ]{1}[0-9]+").match(kit).hasMatch();

        if(kitCondition)
        {
            QAction *allKit = new QAction(tr("Kit", "Menu"), menuSelect);
            connect(allKit, SIGNAL(triggered()), this, SLOT(selectKit()));
            menuSelect->addAction(allKit);
        }
    }

    // Show menu
    menu->popup(ui->tableWidget->viewport()->mapToGlobal(pos));
}

// Menu cleanup
void MainWindow::menuHidden()
{
    sender()->deleteLater();
}

// Select all rows of the same date
void MainWindow::selectDate()
{
    this->selectRows(Datum::Added);
}

// Select all rows of the same source
void MainWindow::selectSource()
{
    this->selectRows(Datum::Source);
}

// Select all rows of the same control group
void MainWindow::selectCG()
{
    this->selectRows(Datum::ControlGroup);
}

// Select all rows of the same kit
void MainWindow::selectKit()
{
    this->selectRows(Datum::Kit);
}

// Select rows on search condition
void MainWindow::selectRows(int row)
{
    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    QString str = selection.indexes().at(row).data().toString();

    QAbstractItemModel *model = ui->tableWidget->model();
    QItemSelectionModel *selectionModel = ui->tableWidget->selectionModel();

    QItemSelection s;

    QModelIndex begin;
    QModelIndex end;

    bool isContinuous = false;
    bool isFound = false;
    int count = ui->tableWidget->rowCount();

    // Need to create ranges at first because other methods of mass selectins is very slow.
    for(int i = 0; i <= count; i++)
    {
        if(i < count)
        {
            isFound = ui->tableWidget->item(i, row)->text() == str;
        }

        if((isFound)&&(i < count - 1)&&((!isContinuous)))
        {
            begin = model->index(i, 0);
            isContinuous = true;
            continue;
        }

        if(((isContinuous)&&(!isFound))||((isFound)&&(i == count)))
        {
            isContinuous = false;
            end = model->index(i - 1, ui->tableWidget->columnCount() - 1);
            s.append(QItemSelectionRange(begin, end));
        }
    }

    // Next line of code may be very slow if there are a lot of ranges.
    selectionModel->select(s, QItemSelectionModel::Clear | QItemSelectionModel::Select);
}

// Status bar displays number of selected rows
void MainWindow::on_tableWidget_itemSelectionChanged()
{
    QItemSelectionModel *model = ui->tableWidget->selectionModel();
    int number = model->selectedRows().count();

    if(model->hasSelection())
    {
        ui->statusBar->showMessage(QString::number(number) % tr(" segments(s) selected."));
    }
    else
    {
        ui->statusBar->showMessage("");
    }
}

// Open edit segment window on menu action
void MainWindow::editSegment()
{
    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    int number = selection.indexes().at(Datum::Number).row();
    QTableWidgetItem *item = ui->tableWidget->item(number, 0);

    emit on_tableWidget_itemDoubleClicked(item);
}

// Show admixture for selected segment
void MainWindow::showAdmixture()
{
    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    int row = datum->getSelectedRows(&selection).at(Datum::Number);

    QList <QTableWidgetItem*> *list = this->getSelectedRow(row);
    datum->setRow(list);

    Admixture admixture(0, datum);
    admixture.setModal(true);
    admixture.exec();

    datum->Row()->clear();
}

// Set control group for selected segment(s)
void MainWindow::cgSelected()
{
    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    // Get control group title and number
    QString title = qobject_cast<QAction*>(sender())->text();

    CG *cg = datum->getCG(title);

    int cgidx = *cg->number;
    QColor color(*cg->color);

    // We need to update both database and QTableWidget
    QList<int> numbers = datum->getSelectedNumbers(&selection);
    QList<int> rows = datum->getSelectedRows(&selection);

    // Update database
    datum->DB()->transaction();

    foreach(int number, numbers)
    {
        try
        {
            datum->updateCG(number, cgidx);
        }
        catch(QString e)
        {
            QMessageBox::critical(this, tr("Error!"), e);
            return;
        }
    }

    datum->DB()->commit();

    // Update CG on QTableWidget
    ui->tableWidget->blockSignals(true);
    foreach(int row, rows)
    {
        for(int i = 0; i < ui->tableWidget->columnCount(); i++)
        {
            QTableWidgetItem *item = ui->tableWidget->item(row, i);

            if(item->column() == Datum::ControlGroup)
            {
                item->setText(*cg->name);
            }

            item->setBackgroundColor(color);
        }
    }
    ui->tableWidget->blockSignals(false);
}

// Get selected row
QList <QTableWidgetItem*>* MainWindow::getSelectedRow(int row)
{
    QList <QTableWidgetItem*> *list = new QList<QTableWidgetItem*>;

    for(int i = 0; i < ui->tableWidget->columnCount(); i++)
    {
        list->append(ui->tableWidget->item(row, i));
    }

    return list;
}

// Open edit segment dialog on double click
void MainWindow::on_tableWidget_itemDoubleClicked(QTableWidgetItem *item)
{
    if(item->column() == Datum::Comment) // Ignore comment column double clicks
    {
        return;
    }

    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    int number = datum->getSelectedNumbers(&selection).at(0);

    QList <QTableWidgetItem*> *list = this->getSelectedRow(item->row());

    datum->setRow(list);

    ui->tableWidget->blockSignals(true);
    emit on_actionAdd_Manual_triggered();

    // Select row (it may be different than before doubleclick)
    int row = 0;
    for(int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        bool condition = ui->tableWidget->item(i, 0)->text().toInt() == number;
        if(condition)
        {
            row = i;
            break;
        }
    }
    ui->tableWidget->selectRow(row);

    ui->tableWidget->blockSignals(false); // It will return here anyway

    datum->Row()->clear();
}

// Delete selected segments
void MainWindow::deleteSegments()
{
    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    QList<int> rows = datum->getSelectedRows(&selection);

    if(rows.count() == 0)
    {
        return;
    }

    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(this, tr("Question"), tr("Are you sure you want to delete selected segments?"),
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No)
    {
        return;
    }

    // Need to start clearing QTableWidget from the end otherwise row numbers will be broken
    std::reverse(rows.begin(), rows.end());

    datum->DB()->transaction();

    ui->tableWidget->blockSignals(true);

    foreach(int row, rows)
    {
        int number = ui->tableWidget->item(row, 0)->text().toInt();

        try
        {
            datum->deleteSegment(number);
        }
        catch(QString e)
        {
            QMessageBox::critical(this, tr("Error!"), e);
            return;
        }

        ui->tableWidget->removeRow(row);
    }

    ui->tableWidget->blockSignals(false);

    datum->DB()->commit();
    ui->statusBar->showMessage("");
}

// Update comment
void MainWindow::on_tableWidget_itemChanged(QTableWidgetItem *item)
{
    int segNumber = ui->tableWidget->item(item->row(), 0)->text().toInt();

    try
    {
        datum->updateComment(segNumber, item->text());
    }
    catch(QString e)
    {
        QMessageBox::critical(this, tr("Error!"), e);
    }
}

// Get rectangle for micro chart
QRect MainWindow::getRectangle(int chr, int begin, int end)
{
    qreal size = 0;

    switch(chr)
    {
        case 1: size = 249250621; break;
        case 2: size = 243199373; break;
        case 3: size = 198022430; break;
        case 4: size = 191154276; break;
        case 5: size = 180915260; break;
        case 6: size = 171115067; break;
        case 7: size = 159138663; break;
        case 8: size = 146364022; break;
        case 9: size = 141213431; break;
        case 10: size = 135534747; break;
        case 11: size = 135006516; break;
        case 12: size = 133851895; break;
        case 13: size = 115169878; break;
        case 14: size = 107349540; break;
        case 15: size = 102531392; break;
        case 16: size = 90354753; break;
        case 17: size = 81195210; break;
        case 18: size = 78077248; break;
        case 19: size = 59128983; break;
        case 20: size = 63025520; break;
        case 21: size = 48129895; break;
        case 22: size = 51304566; break;
        case 23: size = 155270560; break;
    }

    qreal cf = size / 190;

    int posbegin = qRound(begin / cf);
    int posend = qRound(end / cf);

    // at least 1px rectangle is required
    if(posbegin >= posend)
    {
        posend = posbegin + 1;
    }

    return QRect(posbegin, 0, posend - posbegin, 25);
}

// Change language
void MainWindow::changeTranslator(QString postfix)
{
    if (!translator->isEmpty())
    {
        QCoreApplication::removeTranslator(translator);
    }
    translator->load(QApplication::applicationName() % "_" % postfix, QCoreApplication::applicationDirPath());
    QCoreApplication::installTranslator(translator);
    updateUI();
}

// Update UI when language changed
void MainWindow::updateUI()
{
    ui->menuPerson->setTitle(tr("Person"));
    ui->actionAdd->setText(tr("Add..."));
    ui->actionRename->setText(tr("Rename..."));
    ui->actionDelete->setText(tr("Delete..."));
    ui->actionClosePerson->setText(tr("Close Person"));

    ui->menuSegments->setTitle(tr("Segments"));
    ui->actionImport->setText(tr("Import..."));
    ui->actionAdd_Manual->setText(tr("Add Manual..."));
    ui->actionFind->setText(tr("Find..."));
    ui->actionManage_Control_Groups->setText(tr("Manage Control Groups..."));

    ui->menuLanguage->setTitle(tr("Language"));
    ui->actionEnglish->setText(tr("English"));
    ui->actionRussian->setText(tr("Russian"));

    ui->menuHelp->setTitle(tr("Help"));
    ui->actionManual->setText(tr("Manual"));
    ui->actionAbout->setText(tr("About..."));
}

// Language changed
void MainWindow::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        updateUI();
    }
    else
    {
        QMainWindow::changeEvent(event);
    }
}

// Locale changed to English
void MainWindow::on_actionEnglish_triggered()
{
    ui->actionRussian->setChecked(false);
    settings->setValue("Locale", "en");
    changeTranslator("en");
}

// Locale changed to Russian
void MainWindow::on_actionRussian_triggered()
{
    ui->actionEnglish->setChecked(false);
    settings->setValue("Locale", "ru_RU");
    changeTranslator("ru_RU");
}

// Show add new person dialog
void MainWindow::on_actionAdd_triggered()
{
    AddPersonDialog addPersonDialog;
    addPersonDialog.setModal(true);
    addPersonDialog.exec();

    this->getPersons();
}

// Person selected from main menu
void MainWindow::on_personSelected_triggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    this->setPerson(action->text());
}

// Person closed
void MainWindow::on_actionClosePerson_triggered()
{
    delete datum;
    datum = NULL;

    ui->tableWidget->setRowCount(0);
    ui->tableWidget->horizontalHeader()->hide();

    this->setWindowTitle("ChromoMatcher");

    ui->actionRename->setEnabled(false);
    ui->actionDelete->setEnabled(false);
    ui->actionClosePerson->setEnabled(false);

    ui->actionImport->setEnabled(false);
    ui->actionAdd_Manual->setEnabled(false);
    ui->actionFind->setEnabled(false);
    ui->actionManage_Control_Groups->setEnabled(false);
}

// Delete person
void MainWindow::on_actionDelete_triggered()
{
    QMessageBox::StandardButton reply;

    reply = QMessageBox::question(this, tr("Question"), tr("Are you sure you want to delete this person: ") +
                                  datum->Person(), QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        if(datum->Admixture()) // if admixture information available
        {
            QFile::remove(Datum::path() % *datum->Person() % ".txt");
        }
        datum->DB()->close();
        QFile::remove(Datum::path() + *datum->Person() + ".sqlite");

        delete datum;
        datum = NULL;

        emit on_actionClosePerson_triggered();
        this->getPersons();
    }
}

// Rename a person
void MainWindow::on_actionRename_triggered()
{
    bool ok;

    QString newName = QInputDialog::getText(this,tr("Rename a person"),tr("Enter new name: "), QLineEdit::Normal, "", &ok);

    if(!ok)
    {
        return;
    }

    // Strict name requirements for Dodecad
    if(!QRegularExpression("^[A-Za-z0-9_-]+$").match(newName).hasMatch())
    {
        QMessageBox::critical(this, tr("Error!"), tr("Name may only contain numbers, latin characters and _ - symbols!"));
        return;
    }

    QString oldFile1 = Datum::path() % *datum->Person() % ".sqlite";
    QString oldFile2 = Datum::path() % *datum->Person() % ".txt";

    QString newFile1 = Datum::path() % newName % ".sqlite";
    QString newFile2 = Datum::path() % newName % ".txt";

    if((QFile::exists(newFile1))||(QFile::exists(newFile2)))
    {
        QMessageBox::critical(this, tr("Error!"), tr("File(s) already exists!"));
        return;
    }

    datum->DB()->close();

    bool isOk = QFile::rename(oldFile1, newFile1);

    if(!isOk)
    {
        QMessageBox::critical(this, tr("Error!"), tr("Can't rename file1!"));
        return;
    }

    if(datum->Admixture()) // If admixture information available
    {
        isOk = QFile::rename(oldFile2, newFile2);

        if(!isOk)
        {
            QMessageBox::critical(this, tr("Error!"), tr("Can't rename file2!"));
            QFile::rename(newFile1, oldFile1);
            datum->DB()->open();

            return;
        }
    }

    this->getPersons();
    this->setPerson(newName);

    QMessageBox::information(this, tr("Success!"), tr("Person is renamed."));
}

// Find person based on name, email or Gedmatch kit
void MainWindow::on_actionFind_triggered()
{
    bool ok;

    QString search = QInputDialog::getText(this,tr("Search"),tr("Enter name, email or Gedmatch kit number: "),
                                           QLineEdit::Normal, *datum->SearchStr(), &ok);

    if(!ok)
    {
        return;
    }

    if(search == "")
    {
        return;
    }

    // Save last search string for QInputDialog
    datum->setSearchStr(search);

    // check if search contains Gedmatch kit
    //bool isGedmatch = search.contains(QRegExp("[AaMmTtHhWwEeGgZz]{1,1}[0-9]+"));

    // Get last selected row - do not search above it
    QItemSelection selection(ui->tableWidget->selectionModel()->selection());
    QList<int> rows = datum->getSelectedRows(&selection);

    int row = 0;

    if(rows.count() != 0)
    {
        row = rows.last() + 1;
    }

    bool isFirstLoop = true; // If search reaches bottom of the table and nothing is found
                             // one time search starts from the beginning.

    // Iterate rows to find required text
    for(int i = row; i <= ui->tableWidget->rowCount(); i++)
    {
        // checks if it should start from the beginning
        if(i == ui->tableWidget->rowCount())
        {
            if(!isFirstLoop)
            {
                break;
            }
            else
            {
                i=0;
                isFirstLoop = false;
            }
        }

        if((ui->tableWidget->item(i, Datum::Name)->text().contains(search, Qt::CaseInsensitive))||
                // Search for source only if it is Gedmatch kit
                (ui->tableWidget->item(i, Datum::Kit)->text().contains(search, Qt::CaseInsensitive))||
                (ui->tableWidget->item(i, Datum::Email)->text().contains(search, Qt::CaseInsensitive)))
        {
            // Select row if found
            ui->tableWidget->selectRow(i);
            break;
        }
    }
}

// Show About dialog
void MainWindow::on_actionAbout_triggered()
{
    About about;
    about.setModal(true);
    about.exec();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

// Child dialog is closed. Check if we need to update QTableWidget
void MainWindow::dialogIsFinished()
{
    QDialog *obj = qobject_cast<QDialog*>(sender());

    bool isUpdate = obj->property("update").toBool();

    if(isUpdate)
    {        
        this->setPerson(*datum->Person());
    }
}

// Show import dialog
void MainWindow::on_actionImport_triggered()
{
    Import import(0, datum);
    import.setModal(true);
    connect(&import, SIGNAL(finished (int)), this, SLOT(dialogIsFinished()));
    import.exec();
}

// Manually add segment or edit existed one
void MainWindow::on_actionAdd_Manual_triggered()
{
    AddSegment addSegment(0, datum);
    addSegment.setModal(true);
    connect(&addSegment, SIGNAL(finished (int)), this, SLOT(dialogIsFinished()));
    addSegment.exec();
}

// Show dialog with control group management
void MainWindow::on_actionManage_Control_Groups_triggered()
{
    ControlGroups controlGroups(0, datum);
    controlGroups.setModal(true);
    connect(&controlGroups, SIGNAL(finished (int)), this, SLOT(dialogIsFinished()));
    controlGroups.exec();
}

void MainWindow::on_actionManual_triggered()
{
    QDesktopServices::openUrl(QUrl("https://znalazca.github.io/crm/"));
}
