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

#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

#include "vbevent.h"
#include <QDialog>
#include <QItemSelectionModel>
#include <QShortcut>

#include "eventfilter.h"

#define STATUS_RECOMPUTE (-1)
#define STATUS_UNCHANGED (-2)

namespace Ui {
class eventdialog;
}

class eventdialog : public QDialog
{
    Q_OBJECT

public:
    explicit eventdialog(QWidget *parent = nullptr);
    ~eventdialog();
    void reject() Q_DECL_OVERRIDE;

private:
    Ui::eventdialog *ui;
    QString eventDir;
    EventSet lastImport;
    EventFilter filter;

    QList<QStringList> quickAddData;
    QList< QShortcut* > shortcuts;
    QStringList quickAddForKey[10];

private slots:
    void ImportEvents();
    void ExportEvents();
    void EventAction(const QModelIndex &index); // for activating an event
    void AddEvent(QString typeName=QString(), QString subTypeName=QString());
    void AddEventClicked() { AddEvent(); }
    void QuickAddEvent();
    void ShortcutAddEvent(int i);
    void ConfigureQuickAddEvents(int idx);
    void EditSelectedEvents();
    void EditSingleEvent(int row);
    void EditMultiEvents(QList<int> rows);
    void EditFrameOffset(QList<int> rows);
    void MergeEvents(QList<int> rows);
    void DeleteSelectedEvents();

    void EditFilter();
    void RestrictToLastImport();
    void FilterState();

    void UpdateTable();
    void UpdateTableOnce();
    void StatusWorking();
    void UpdateStatusBar(
        int nShown=STATUS_RECOMPUTE,
        int nSelectedAndHidden=STATUS_RECOMPUTE);
    void UpdateStatusBarOnce(int nShown, int nSelectedAndHidden);
    void FixConfidenceElide();

    void ConfigureConfidenceThreshold();
    void SelectColumns();

    void FilmNotesDialog();

    void FilterSelection(
        const QItemSelection &selected,
        const QItemSelection &deselected);
    void SelectAllVisible();

private:
    void SetAllDates(QDateTime datetime, EventSet set);
    void EnableAvailableWidgets(int nSel);
    void UpdateQuickAddComboBox();
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

signals:
    void jump(uint32_t frameNum);

public slots:
    void ScrollToFrame(uint32_t frameNum);
    void ShortcutEventKey(int num);
};

#endif // EVENTDIALOG_H
