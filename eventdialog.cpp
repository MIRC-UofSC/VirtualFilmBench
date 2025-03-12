//-----------------------------------------------------------------------------
// This file is part of Virtual Film Bench
//
// Copyright (c) 2025 University of South Carolina and Thomas Aschenbach
//
// Project contributors include: Thomas Aschenbach (Colorlab, inc.),
// L. Scott Johnson (USC), Greg Wilsbacher (USC), Pingping Cai (USC),
// and Stella Garcia (USC).
//
// Funding for Virtual Film Bench development was provided through a grant
// from the National Endowment for the Humanities with additional support
// from the National Science Foundation’s Access program.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3 of the License, or (at your
// option) any later version.
//
// Virtual Film Bench is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, see http://gnu.org/licenses/.
//
// For inquiries or permissions, contact
// Greg Wilsbacher (gregw@mailbox.sc.edu)
//-----------------------------------------------------------------------------

#include "eventdialog.h"
#include "ui_eventdialog.h"

#include <algorithm>

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QShortcut>
#include <QSpinBox>

#include "mainwindow.h"
#include "eventdataform.h"
#include "eventfilterdialog.h"
#include "eventquickconfig.h"
#include "listselectdialog.h"
#include "decimalelidedelegate.h"

eventdialog::eventdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::eventdialog)
{
    ui->setupUi(this);

    MainWindow *mainwindow = qobject_cast<MainWindow *>(parent);
    if(!mainwindow)
    {
        QMessageBox msgError;
        msgError.setText(
            QString("Internal Error: Event window's parent is not main"));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Window hierarchy error");
        msgError.exec();
        return;
    }

    QSettings settings;
    int nQuick;
    settings.beginGroup("event");
    nQuick = settings.beginReadArray("quick");

    if(nQuick>0)
    {
        for(int i = 0; i < nQuick; ++i)
        {
            settings.setArrayIndex(i);
            QStringList l;
            l << settings.value("label").toString();
            l << settings.value("type").toString();
            l << settings.value("subtype").toString();
            l << settings.value("hotkey").toString();
            quickAddData.append(l);
        }
    }
    else
    {
        QList<QStringList> dflt =
            {
                { "Cement Splice", "Splice", "Cement", QString() },
                { "Printed Splice", "Splice", "Printed", QString() },
                { "Tape Splice", "Splice", "Tape", QString() },
                { "Scratch", "Damage", "Scratch", QString() },
                { "Burn", "Damage", "Burn", QString() }
            };
        quickAddData = dflt;
    }

    settings.endArray();
    settings.endGroup();

    UpdateQuickAddComboBox();

    ui->tableView->setModel(mainwindow->vbscan.FilmEventsTableModel());
    FixConfidenceElide();
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->editEventButton->setEnabled(false);
    ui->deleteEventButton->setEnabled(false);
    ui->filterCheckBox->setChecked(false);
    ui->showImportCheckbox->setChecked(false);

    // Connect slots programmatically to avoid error-prone "ConnectSlotsByName"
    connect(
        ui->tableView, &QTableView::activated,
        this, &eventdialog::EventAction);
    connect(
        ui->addEventButton, &QPushButton::clicked,
        this, &eventdialog::AddEventClicked);
    connect(
        ui->quickAddButton, &QPushButton::clicked,
        this, &eventdialog::QuickAddEvent);
    connect(
        ui->quickAddComboBox, &QComboBox::activated,
        this, &eventdialog::ConfigureQuickAddEvents);
    connect(
        ui->editEventButton,&QPushButton::clicked,
        this, &eventdialog::EditSelectedEvents );
    connect(
        ui->deleteEventButton,&QPushButton::clicked,
        this, &eventdialog::DeleteSelectedEvents );
    connect(
        ui->importEventButton, &QPushButton::clicked,
        this, &eventdialog::ImportEvents );
    connect(
        ui->exportEventButton, &QPushButton::clicked,
        this, &eventdialog::ExportEvents );

    connect(
        ui->filterCheckBox, &QCheckBox::clicked,
        this, &eventdialog::FilterState );
    connect(
        ui->filterButton, &QPushButton::clicked,
        this, &eventdialog::EditFilter );
    connect(
        ui->showImportCheckbox, &QCheckBox::clicked,
        this, &eventdialog::RestrictToLastImport );
    connect(
        ui->confidenceThresholdButton, &QPushButton::clicked,
        this, &eventdialog::ConfigureConfidenceThreshold );
    connect(
        ui->selectColumnsButton, &QPushButton::clicked,
        this, &eventdialog::SelectColumns );

    connect(
        ui->filmNotesButton, &QPushButton::clicked,
        this, &eventdialog::FilmNotesDialog);

    connect(
        mainwindow->vbscan.FilmEventsTableModel(),
        &VBFilmEventsTableModel::FilmEventsColumnsChanged,
        this, &eventdialog::FixConfidenceElide);

    connect(
        mainwindow->vbscan.FilmEventsTableModel(),
        &VBFilmEventsTableModel::FilmEventsTableUpdated,
        this, &eventdialog::UpdateTable);

    connect(
        ui->tableView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this, &eventdialog::FilterSelection);

    connect(
        ui->buttonBox, &QDialogButtonBox::rejected,
        this, &eventdialog::reject);

    UpdateStatusBar();
}

eventdialog::~eventdialog()
{
    delete ui;
}

void eventdialog::reject()
{
    QDialog::reject();
}

void eventdialog::ImportEvents()
{
    MainWindow *mainwindow = MainWindowAncestor(this);

    if(!mainwindow) return;

    if(eventDir.isEmpty())
    {
        QSettings settings;
        settings.beginGroup("default-folder");
        eventDir = settings.value("import").toString();
        if(eventDir.isEmpty())
        {
            eventDir = QStandardPaths::writableLocation(
                QStandardPaths::DocumentsLocation);
        }
        settings.endGroup();
    }

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Event XML"), eventDir, tr("XML Files (*.xml)"));

    if(fileName.isEmpty()) return;

    lastImport = mainwindow->vbscan.ImportEvents(fileName);

    if(lastImport.size() > 0)
    {
        StatusWorking();

        QDateTime earliestDate;
        QDateTime latestDate;
        bool everyDateMissing(true);
        bool anyDateMissing(false);

        for(auto &frameevent : mainwindow->vbscan.FilmEvents())
        {
            for(auto &event : frameevent)
            {
                if(!event.InSet(lastImport)) continue;

                QString dStr = event.Attribute("DateCreated");
                QString modStr = event.Attribute("DateModified");

                if(dStr.isEmpty())
                {
                    if(modStr.isEmpty())
                    {
                        anyDateMissing = true;
                        continue;
                    }
                    else
                        dStr = modStr;
                }

                QDateTime d = QDateTime::fromString(dStr, Qt::ISODate);
                if(!d.isValid())
                {
                    qDebug() << "DateCreated invalid: " << dStr;
                    anyDateMissing = true;
                    continue;
                }

                if(everyDateMissing)
                {
                    earliestDate = latestDate = d;
                    everyDateMissing = false;
                }
                else
                {
                    if(d < earliestDate) earliestDate = d;
                    if(d > latestDate) latestDate = d;
                }
            }
        }

        if(anyDateMissing)
        {
            const QFileInfo info(fileName);
            const QDateTime createDate = info.birthTime();
            const QDateTime modDate = info.lastModified();

            if(everyDateMissing)
            {
                QMessageBox msg(this);
                msg.setText("No creation date given for imported events");
                msg.setInformativeText("What do you want to set the creation date to?");

                QString details;

                QPushButton *today =
                    msg.addButton(tr("Today"), QMessageBox::AcceptRole);

                QPushButton *fileCreate(nullptr);
                if(createDate.isValid())
                {
                    fileCreate =
                        msg.addButton(tr("File Date"),
                                      QMessageBox::AcceptRole);

                    details +=
                        "File Date: " + createDate.toString(Qt::ISODate) + "\n";
                }
                QPushButton *fileMod(nullptr);
                if(modDate.isValid() && (createDate.secsTo(modDate)>0))
                {
                    fileMod =
                        msg.addButton(tr("File Mod Date"),
                                      QMessageBox::AcceptRole);

                    details +=
                        "File Modification Date: " +
                               modDate.toString(Qt::ISODate) + " " +
                               QString::number(createDate.secsTo(modDate)) +
                               "\n";
                }

                msg.addButton(tr("Leave Blank"), QMessageBox::RejectRole);

                if(!details.isEmpty()) msg.setDetailedText(details);

                msg.exec();

                if(msg.clickedButton() == today)
                    SetAllDates(QDateTime::currentDateTime(), lastImport);
                else if(msg.clickedButton() == fileCreate)
                    SetAllDates(createDate, lastImport);
                else if(msg.clickedButton() == fileMod)
                    SetAllDates(modDate, lastImport);
            }
            else
            {
                QMessageBox msg(this);
                msg.setText("Some imported events are missing creation dates.");
                msg.setInformativeText("What do you want to set those creation date to?");

                QString details;

                QPushButton *today =
                    msg.addButton(tr("Today"), QMessageBox::AcceptRole);

                QPushButton *earliest =
                    msg.addButton(tr("Earliest"), QMessageBox::AcceptRole);
                QPushButton *latest =
                    msg.addButton(tr("Latest"), QMessageBox::AcceptRole);

                details += "Earliest imported event creation date: " +
                           earliestDate.toString(Qt::ISODate) + "\n";
                details += "Latest imported event creation date: " +
                           earliestDate.toString(Qt::ISODate) + "\n";

                QPushButton *fileCreate(nullptr);
                if(createDate.isValid())
                {
                    fileCreate =
                        msg.addButton(tr("File Date"),
                                      QMessageBox::AcceptRole);

                    details +=
                        "File Date: " + createDate.toString(Qt::ISODate) + "\n";
                }
                QPushButton *fileMod(nullptr);
                if(modDate.isValid() && (createDate.secsTo(modDate)>0))
                {
                    fileMod =
                        msg.addButton(tr("File Mod Date"),
                                      QMessageBox::AcceptRole);

                    details +=
                        "File Modification Date: " +
                        modDate.toString(Qt::ISODate) + " " +
                        QString::number(createDate.secsTo(modDate)) +
                        "\n";
                }

                msg.addButton(tr("Leave Blank"), QMessageBox::RejectRole);

                msg.setDetailedText(details);

                msg.exec();

                if(msg.clickedButton() == today)
                    SetAllDates(QDateTime::currentDateTime(), lastImport);
                else if(msg.clickedButton() == earliest)
                    SetAllDates(earliestDate, lastImport);
                else if(msg.clickedButton() == latest)
                    SetAllDates(latestDate, lastImport);
                else if(msg.clickedButton() == fileCreate)
                    SetAllDates(createDate, lastImport);
                else if(msg.clickedButton() == fileMod)
                    SetAllDates(modDate, lastImport);
            }
        }

        mainwindow->Render_Frame();

        UpdateTable();
    }

    eventDir = QFileInfo(fileName).absolutePath();

    // macos usually hides this dialog after import, so force it
    // back to the top.
    this->show();
    this->raise();
}

void eventdialog::ExportEvents()
{
    MainWindow *mainwindow = MainWindowAncestor(this);

    if(!mainwindow) return;

    if(eventDir.isEmpty())
    {
        eventDir = mainwindow->ProjectDir();

        if(eventDir.isEmpty())
            eventDir = QStandardPaths::writableLocation(
                QStandardPaths::DocumentsLocation);
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Export Events XML"),
        eventDir,
        tr("XML Files (*.xml)"));

    if(fileName.isEmpty()) return;

    // Export only events that aren't hidden by the filter, if any
    // (use lambda function to turn !isHidden to isToBeIncluded)
    mainwindow->vbscan.ExportEvents(
        fileName,
        [this](int r) { return !ui->tableView->isRowHidden(r); } );
}

// EventAction is done when user doubleclicks in the tableview
void eventdialog::EventAction(const QModelIndex &index)
{
    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

    /*
     * Take the frame number from the event itself rather than from
     * the table in case column zero is changed to something other
     * than the starting frame.
     */

    uint32_t frameNum = tableModel->EventAtRow(index.row())->Start();

    // momentarily disable syncing to the playhead while we snap
    // the playhead to this event
    bool sync = ui->syncScrollCheckbox->isChecked();
    ui->syncScrollCheckbox->setChecked(false);
    emit jump(frameNum);
    ui->syncScrollCheckbox->setChecked(sync);
}

void eventdialog::AddEvent(QString typeName, QString subTypeName)
{
    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    uint32_t frame = mainwindow->CurrentFrame();
    vbevent event(frame);

    event.SetBoundsX0X1Y0Y1(
        mainwindow->MarqueeX0(),
        mainwindow->MarqueeX1(),
        mainwindow->MarqueeY0(),
        mainwindow->MarqueeY1());

    if(!typeName.isEmpty())
    {
        event.SetType(typeName);
        if(!subTypeName.isEmpty())
            event.SetSubType(subTypeName);
    }

    QString creator = mainwindow->vbscan.Properties().Value("CreatorID");
    if(!creator.isEmpty()) event.SetAttribute("CreatorID", creator);
    QString context = mainwindow->vbscan.Properties().Value("CreatorContext");
    if(!context.isEmpty()) event.SetAttribute("CreatorContext", context);

    EventDataForm eventDataForm(
        this,
        tableModel->FilmEvents(),
        QList< const vbevent* >({&event}));

    int code = eventDataForm.exec();

    if(code == QDialog::Accepted)
    {
        StatusWorking();

        event = eventDataForm.Events().at(0);
        tableModel->AddEvent(event);
        mainwindow->MarqueeClear();

        if(event.Start() == frame)
            mainwindow->Render_Frame();        

        UpdateTable();
    }
}

void eventdialog::QuickAddEvent()
{
    int i = ui->quickAddComboBox->currentIndex();
    AddEvent(quickAddData[i][1], quickAddData[i][2]);
}

void eventdialog::ShortcutAddEvent(int i)
{
    AddEvent(quickAddData[i][1], quickAddData[i][2]);
}

void eventdialog::ShortcutEventKey(int num)
{
    if(0<=num && num <=9 && !quickAddForKey[num].isEmpty())
        AddEvent(quickAddForKey[num][1], quickAddForKey[num][2]);
}

void eventdialog::ConfigureQuickAddEvents(int idx)
{
    static int prevIdx(0);

    // "Configure" is the last item in the combobox; return if
    // a different selection is made
    if(idx != ui->quickAddComboBox->count()-1)
    {
        prevIdx = idx;
        return;
    }

    EventQuickConfig cfg(quickAddData, this);

    if(cfg.exec() != QDialog::Accepted)
    {
        ui->quickAddComboBox->setCurrentIndex(prevIdx);
        return;
    }

    quickAddData = cfg.EventTypes();

    UpdateQuickAddComboBox();
}
void eventdialog::EditSelectedEvents()
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    if(!ui->tableView->selectionModel()->hasSelection()) return;

    if(ui->tableView->selectionModel()->selectedRows().size() == 1)
    {
        EditSingleEvent(
            ui->tableView->selectionModel()->selectedRows().at(0).row());
    }
    else
    {
        QMessageBox msg(this);
        msg.setText("Editing multiple events");
        msg.setInformativeText("What do you want to do?");

        QPushButton *edit =
            msg.addButton(tr("Batch Edit"), QMessageBox::AcceptRole);
        QPushButton *adjust =
            msg.addButton(tr("Frame Offset"), QMessageBox::AcceptRole);
        QPushButton *merge =
            msg.addButton(tr("Merge"), QMessageBox::AcceptRole);
        QPushButton *cancel =
            msg.addButton(tr("Cancel"), QMessageBox::RejectRole);

        msg.exec();

        if(msg.clickedButton() == cancel) return;

        QList<int> rows;
        for(auto i: ui->tableView->selectionModel()->selectedRows())
            rows.append(i.row());

        if(msg.clickedButton() == edit)
            EditMultiEvents(rows);
        else if(msg.clickedButton() == adjust)
            EditFrameOffset(rows);
        else if(msg.clickedButton() == merge)
            MergeEvents(rows);
    }
}

void eventdialog::EditSingleEvent(int row)
{
    QList<int> rows;
    rows << row;

    EditMultiEvents(rows);
}

void eventdialog::EditMultiEvents(QList<int> rows)
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

    QList< const vbevent* > events;

    for(auto r: rows) events.append(tableModel->EventAtRow(r));

    EventDataForm eventDataForm(
        this,
        tableModel->FilmEvents(),
        events);

    int code = eventDataForm.exec();

    if(code == QDialog::Accepted)
    {
        StatusWorking();

        QList< vbevent > newEvents = eventDataForm.Events();

        for(int i=0; i<events.size(); ++i)
        {
            EventID id1, id2;
            id1 = events[i]->ID();
            id2 = newEvents[i].ID();

            Q_ASSERT(id1 == id2);

            // verify the row of this event directly,
            // in case the row of this event has been changed by
            // the edits made to the events that have already been
            // updated in the table.
            int row = tableModel->RowOfEvent(events[i]);

            tableModel->UpdateEventAtRow(row, newEvents[i]);
        }

        mainwindow->Render_Frame();

        UpdateTable();
    }
}

void eventdialog::EditFrameOffset(QList<int> rows)
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

    int f0 = tableModel->EventAtRow(rows.first())->Start();

    QString msg =
        QString("Current selection's starting frame is %1. What should it be?").
        arg(f0);

    bool ok(false);
    int f1 = QInputDialog::getInt(
        this, "Frame Offset", msg, f0, 0, 2147483647, 1, &ok);

    int offset = f1-f0;

    if(ok && offset)
    {
        if(offset < 0)
        {
            for(auto r : rows)
            {
                const vbevent *e0 = tableModel->EventAtRow(r);
                vbevent e1 = *e0;
                e1.SetStartAndEnd(e1.Start()+offset, e1.End()+offset);
                tableModel->UpdateEventAtRow(r, e1);
            }
        }
        else
        {
            for(int r=rows.size()-1; r>=0; --r)
            {
                const vbevent *e0 = tableModel->EventAtRow(rows.at(r));
                vbevent e1 = *e0;
                e1.SetStartAndEnd(e1.Start()+offset, e1.End()+offset);
                tableModel->UpdateEventAtRow(rows.at(r), e1);
            }
        }

        UpdateTable();
        mainwindow->Render_Frame();
    }
}

void eventdialog::MergeEvents(QList<int> rows)
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    if(!ui->tableView->selectionModel()->hasSelection()) return;

    int n = ui->tableView->selectionModel()->selectedRows().size();

    if(n<2) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Merge Events");
    QFormLayout form(&dialog);

    QStringList attrList = mainwindow->vbscan.AttributesInUse();
    // don't merge event types
    attrList.removeOne("EventType");

    // preference given to EdgeMarkString
    if(attrList.removeOne("EdgeMarkString"))
        attrList.prepend("EdgeMarkString");

    QComboBox attrComboBox(&dialog);
    attrComboBox.addItems(attrList);
    attrComboBox.setEditable(false);
    form.addRow(QString("Merge"), &attrComboBox);

    QStringList orderList;
    orderList << "Top-to-Bottom" << "Bottom-to-Top";
    QComboBox orderComboBox(&dialog);
    orderComboBox.addItems(orderList);
    orderComboBox.setEditable(false);
    form.addRow(QString("Order"), &orderComboBox);

    QDialogButtonBox buttonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if(dialog.exec() != QDialog::Accepted) return;

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

    QString mergeAttr = attrComboBox.currentText();
    bool reverse = orderComboBox.currentIndex() == 1;

    // build the list in reverse so the row numbers are correct throughout
    // the process as the rows are taken
    QList<vbevent> eventList;
    eventList.reserve(rows.size());
    for(int r=rows.size()-1; r>=0; --r)
    {
        eventList << tableModel->TakeEvent(rows.at(r));
    }

    // If it's not supposed to be reversed, reverse it back to the right order
    if(!reverse) std::reverse(eventList.begin(), eventList.end());

    vbevent newEvent = eventList.takeFirst();
    QString newValue = newEvent.Attribute(mergeAttr);

    for(vbevent& e : eventList)
    {
        if(
            e.Start() == newEvent.Start() &&
            e.BoundsSortType() == newEvent.BoundsSortType()
            )
        {
            newValue += e.Attribute(mergeAttr);
            newEvent.SetBoundsX0X1Y0Y1(
                std::min(newEvent.BoundsX0(), e.BoundsX0()),
                std::max(newEvent.BoundsX1(), e.BoundsX1()),
                std::min(newEvent.BoundsY0(), e.BoundsY0()),
                std::max(newEvent.BoundsY1(), e.BoundsY1())
                );
        }
        else
        {
            newEvent.SetAttribute(mergeAttr, newValue);
            tableModel->AddEvent(vbevent(newEvent));
            newEvent = e;
            newValue = newEvent.Attribute(mergeAttr);
        }
    }
    newEvent.SetAttribute(mergeAttr, newValue);
    tableModel->AddEvent(newEvent);

    UpdateTable();
    mainwindow->Render_Frame();
}

void eventdialog::DeleteSelectedEvents()
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    if(!ui->tableView->selectionModel()->hasSelection()) return;

    int n = ui->tableView->selectionModel()->selectedRows().size();

    if(n==0) return;

    QString eventString("Delete event?");

    if(n>1) eventString = QString("Delete %1 events?").arg(n);

    QMessageBox::StandardButton resBtn =
        QMessageBox::question(
            this, "Delete",
            eventString,
            QMessageBox::Cancel | QMessageBox::Yes,
            QMessageBox::Yes);

    if (resBtn != QMessageBox::Yes)
        return;

    StatusWorking();

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

    // copy the row numbers; the selection is lost after the first deletion
    QModelIndexList rows = ui->tableView->selectionModel()->selectedRows();

    for(int i=n-1; i>=0; --i)
        tableModel->DeleteEvent(rows.at(i).row());

    mainwindow->Render_Frame();

    UpdateStatusBar();
}

void eventdialog::EditFilter()
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    EventFilterDialog editor(this, &(this->filter));

    int code = editor.exec();
    if(code == QDialog::Accepted)
    {
        this->filter = editor.Filter();
        if(ui->filterCheckBox->isChecked())
            UpdateTable();
    }
}

void eventdialog::RestrictToLastImport()
{
    if(ui->filterCheckBox->isChecked() && !lastImport.isEmpty())
        UpdateTable();
}

void eventdialog::FilterState()
{
    UpdateTable();
}

void eventdialog::UpdateTable()
{
    static QTimer *pending = nullptr;

    if(!pending)
    {
        pending = new QTimer(this);
        connect(pending, SIGNAL(timeout()), this, SLOT(UpdateTableOnce()));
        pending->setSingleShot(true);
        pending->setInterval(0);
        pending->start();
    }
    else if(!(pending->isActive()))
    {
        pending->start();
    }
}

void eventdialog::UpdateTableOnce()
{
    StatusWorking();

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());
    int nRow = tableModel->rowCount();

    QList<int> selectedRows;
    if(ui->tableView->selectionModel()->hasSelection())
    {
        for(auto i : ui->tableView->selectionModel()->selectedRows())
            selectedRows << i.row();
    }

    int nFiltered(0);
    int nSelectedAndHidden(0);

    if(ui->filterCheckBox->isChecked())
    {
        EventFilter test = filter;

        if(ui->showImportCheckbox->isChecked() && !lastImport.isEmpty())
            test.AddCondition(lastImport);

        for(int r=0; r<nRow; ++r)
        {
            if(test.EventPasses(*(tableModel->EventAtRow(r))))
            {
                ui->tableView->showRow(r);
                nFiltered++;
            }
            else
            {
                ui->tableView->hideRow(r);
                if(selectedRows.contains(r)) nSelectedAndHidden++;
            }
        }
    }
    else // filter is off, show all rows
    {
        for(int r=0; r<nRow; ++r)
            ui->tableView->showRow(r);
    }

    UpdateStatusBarOnce(nFiltered, nSelectedAndHidden);
}

void eventdialog::StatusWorking()
{
    ui->statusLabel->setText("Working...");
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void eventdialog::UpdateStatusBar(int nShown, int nSelHid)
{
    static QTimer *pending = nullptr;

    if(pending && pending->isActive()) return;

    if(!pending)
        pending = new QTimer(this);
    else
        pending->disconnect();

    connect(
        pending, &QTimer::timeout,
        this,
        [this, nShown, nSelHid]{UpdateStatusBarOnce(nShown, nSelHid);});

    pending->setSingleShot(true);
    pending->setInterval(0);
    pending->start();

    StatusWorking();
}

void eventdialog::UpdateStatusBarOnce(int nShown, int nSelHid)
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    QString status;

    static int lastShown = 0;

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());
    int nRow = tableModel->rowCount();
    int nSelected(0);

    if(nRow==1)
        status = "1 event";
    else
        status = QString("%1 events").arg(nRow);

    if(nShown == STATUS_UNCHANGED)
    {
        nShown = lastShown;
        nSelHid = 0;
    }

    if(ui->filterCheckBox->isChecked())
    {
        // if we don't already have the counts, then count them
        if(nShown == STATUS_RECOMPUTE)
        {
            nShown = 0;
            nSelHid = 0;

            if(ui->tableView->selectionModel()->hasSelection())
            {
                for(int r=0; r<nRow; ++r)
                {
                    if(ui->tableView->isRowHidden(r))
                    {
                        if(ui->tableView->selectionModel()->isRowSelected(r))
                            nSelHid++;
                    }
                    else
                    {
                        nShown++;
                    }
                }
            }
            else
                for(int r=0; r<nRow; ++r)
                    if(!ui->tableView->isRowHidden(r))
                        nShown++;
        }

        status += QString(", %1 shown").arg(nShown);
        if(ui->tableView->selectionModel()->hasSelection())
        {
            nSelected = ui->tableView->selectionModel()->selectedRows().count();

            status += QString(", %1 selected").arg(nSelected);
            if(nSelHid > 0)
                status += QString(
                    ", <b><font color=\"red\">"
                    "%1 selected and hidden"
                    "</font></b>").arg(nSelHid);
        }
    }
    else if(ui->tableView->selectionModel()->hasSelection())
    {
        nSelected = ui->tableView->selectionModel()->selectedRows().count();
        nShown = nSelected;
        status += QString(", %1 selected").arg(nSelected);
    }

    if(mainwindow->vbscan.ConfidenceThresholdIsEnabled())
    {
        status += QString(" (%1 thresholded)").
                  arg(mainwindow->vbscan.NumEventsThresholded());
    }

    ui->statusLabel->setText(status);

    lastShown = nShown;
}

void eventdialog::FixConfidenceElide()
{
    DecimalElideDelegate *delegate =
        qobject_cast<DecimalElideDelegate*>(ui->tableView->itemDelegate());
    if(delegate == nullptr)
    {
        delegate = new DecimalElideDelegate(this);
        ui->tableView->setItemDelegate(delegate);
    }

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());
    int col = tableModel->Columns().indexOf("Confidence");

    delegate->SetColumn(col);
}

void eventdialog::ConfigureConfidenceThreshold()
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Confidence Threshold");
    QFormLayout form(&dialog);

    float confThresh = mainwindow->vbscan.ConfidenceThreshold();
    bool confThreshEnabled = mainwindow->vbscan.ConfidenceThresholdIsEnabled();

    QDoubleSpinBox confidenceSpinbox;
    confidenceSpinbox.setMinimum(0.0);
    confidenceSpinbox.setMaximum(1.0);
    confidenceSpinbox.setDecimals(6);
    confidenceSpinbox.setSingleStep(0.1);
    confidenceSpinbox.setValue(confThresh);
    form.addRow(QString("Confidence"), &confidenceSpinbox);

    QComboBox enabledComboBox(&dialog);
    enabledComboBox.addItem(QString("Disable"));
    enabledComboBox.addItem(QString("Enable"));
    enabledComboBox.setEditable(false);
    enabledComboBox.setCurrentText(confThreshEnabled?"Enable":"Disable");
    form.addRow(QString("Threshold"), &enabledComboBox);

    QDialogButtonBox buttonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    if(dialog.exec() != QDialog::Accepted) return;

    qDebug() << "Threshold " << confidenceSpinbox.value() << (enabledComboBox.currentIndex()==1 ? "On" : "off");
    mainwindow->vbscan.SetConfidenceThreshold(
        confidenceSpinbox.value(),
        enabledComboBox.currentIndex()==1);

    UpdateTable();
    mainwindow->Render_Frame();
}

void eventdialog::SelectColumns()
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

    QStringList colSet;

    colSet = mainwindow->vbscan.AttributesInUse();
    // add the non-attribute special columns that can be selected
    colSet << "Frame" << "End" << "SubType" << "Details";
    colSet << "DateCreated" << "DateModified";
    colSet.sort(Qt::CaseInsensitive);

    ListSelectDialog dialog(this, colSet, tableModel->Columns());

    int code = dialog.exec();
    if(code == QDialog::Accepted)
    {
        tableModel->SetColumns(dialog.SelectedStrings());
    }
}

void eventdialog::FilmNotesDialog()
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    #ifdef FILM_NOTES_MODAL
    bool ok;
    QString text = QInputDialog::getMultiLineText(
        this, "Film Notes", "Notes", mainwindow->vbscan.FilmNotes(), &ok);

    if (ok)
        mainwindow->vbscan.SetFilmNotes(text);
    #else
    static QDialog* dialog(nullptr);

    if(!dialog)
    {
        dialog = new QDialog(this);
        dialog->setModal(false);
        dialog->setWindowTitle("Film Notes");
        QVBoxLayout* vbox = new QVBoxLayout(dialog);
        QLabel* label = new QLabel("Notes:");
        vbox->addWidget(label);

        QPlainTextEdit *text = new QPlainTextEdit();
        text->setPlainText(mainwindow->vbscan.FilmNotes());
        QFontMetrics fm = text->fontMetrics();
        int w = fm.averageCharWidth();
        text->setMinimumWidth(w*80);
        vbox->addWidget(text);

        QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Close);
        connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));
        vbox->addWidget(buttonBox);

        connect(
            text, &QPlainTextEdit::textChanged,
            this, [mainwindow, text](){
                mainwindow->vbscan.SetFilmNotes(text->toPlainText());} );
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();

    #endif // FILM_NOTES_MODAL

}

// Remove any filtered-out rows from the newly-selected list
// and update the status bar
void eventdialog::FilterSelection(
    const QItemSelection &selected,
    const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    StatusWorking();

    int nRows=0;

    for(auto &range : selected) nRows += range.height();

    if(ui->filterCheckBox->isChecked())
    {
        const QSignalBlocker b(ui->tableView->selectionModel());

        this->setCursor(Qt::WaitCursor);

        QMessageBox msgBox(this);
        if(nRows > 100)
        {
            msgBox.setIcon( QMessageBox::Information );
            msgBox.setStandardButtons(QMessageBox::NoButton);
            msgBox.setText("Applying filter to selection");
            msgBox.setModal(false);
            msgBox.show();
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        for(auto &range : selected)
        {
            for(int r = range.top(); r <= range.bottom(); r++)
            {
                if(ui->tableView->isRowHidden(r))
                {
                    ui->tableView->selectionModel()->select(
                        ui->tableView->model()->index(r,0),
                        QItemSelectionModel::Toggle |
                        QItemSelectionModel::Rows);
                }
            }
        }

        // call Update...Once() directly so the messagebox remains active
        UpdateStatusBarOnce(STATUS_UNCHANGED, STATUS_UNCHANGED);

        this->setCursor(Qt::ArrowCursor);
    }
    else
        UpdateStatusBar(STATUS_UNCHANGED, STATUS_UNCHANGED);

    EnableAvailableWidgets(nRows);
}

void eventdialog::SelectAllVisible()
{
    const QSignalBlocker b(ui->tableView->selectionModel());

    StatusWorking();

    ui->tableView->clearSelection();

    VBFilmEventsTableModel *tableModel =
        static_cast<VBFilmEventsTableModel *>(ui->tableView->model());
    int nRow = tableModel->rowCount();

    for(int r=0; r<nRow; ++r)
    {
        if(ui->tableView->isRowHidden(r)) continue;

        QModelIndex idx = ui->tableView->model()->index(r,0);
        ui->tableView->selectionModel()->select(
            idx, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    }

    UpdateStatusBar(ui->tableView->selectionModel()->selectedRows().size(), 0);
}

void eventdialog::EnableAvailableWidgets(int numSel)
{
    ui->editEventButton->setEnabled(numSel > 0);
    ui->deleteEventButton->setEnabled(numSel > 0);
}

void eventdialog::SetAllDates(QDateTime datetime, EventSet set)
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;

    QString dateStr = datetime.toString(Qt::ISODate);

    for(auto &frameevent : mainwindow->vbscan.FilmEvents())
    {
        for(auto &event : frameevent)
        {
            if(!event.InSet(set)) continue;

            event.SetAttribute("DateCreated", dateStr);
        }
    }
}

void eventdialog::UpdateQuickAddComboBox()
{
    // remove the old shortcuts. If they're still specified, they'll be remade
    for(auto s: shortcuts) delete s;
    shortcuts.clear();
    for(int i =0; i<10; ++i) quickAddForKey[i].clear();

    /*
    QObject *glWin = nullptr;
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(mainwindow) glWin = mainwindow->GLWindow();
    */

    int nQuick = quickAddData.size();
    for(int i = 0; i < nQuick; ++i)
    {
        QString key = quickAddData[i].value(3);
        if(key.isEmpty()) continue;


        bool ok;
        int num = key.last(1).toInt(&ok);
        if(!ok) continue;

        quickAddForKey[num] = quickAddData[i];

        QShortcut *shortcut = new QShortcut(QKeySequence(key), this);
        shortcut->setContext(Qt::ApplicationShortcut);
        connect(
            shortcut, &QShortcut::activated,
            this, [this, i](){ShortcutAddEvent(i);});
        shortcuts << shortcut;

        /*
         * QOpenGLWindow doesn't interact with QShortCut
         *
        if(glWin)
        {
            QShortcut *shortcut = new QShortcut(QKeySequence(key), glWin);
            shortcut->setContext(Qt::WindowShortcut);
            connect(
                shortcut, &QShortcut::activated,
                this, [this, i](){ShortcutAddEvent(i);});
            shortcuts << shortcut;
        }
        */
    }

    ui->quickAddComboBox->clear();

    for(QStringList qa : quickAddData)
        ui->quickAddComboBox->addItem(qa[0]);
    ui->quickAddComboBox->insertSeparator(quickAddData.size());
    ui->quickAddComboBox->addItem("Configure");
    ui->quickAddComboBox->setCurrentText(quickAddData[0][0]);
}

void eventdialog::ScrollToFrame(uint32_t frame)
{
    if(
        ui->syncScrollCheckbox->isChecked() &&
        (ui->tableView->height() > 1))
    {
        VBFilmEventsTableModel *tableModel =
            static_cast<VBFilmEventsTableModel *>(ui->tableView->model());

        if(tableModel->rowCount()<=1) return;

        int row = tableModel->RowAtFrame(frame);

        ui->tableView->scrollTo(
            tableModel->index(row, 0),
            QAbstractItemView::PositionAtTop);
    }
}

void eventdialog::mousePressEvent(QMouseEvent *event)
{
    // dialog widget only receives the event if none of the children
    // have acted on it.

    // if none of the children picked up on this click, at least remove
    // focus from the table view for keystrokes don't have unintended
    // edits occur.
    ui->tableView->clearFocus();
    event->ignore();
}
