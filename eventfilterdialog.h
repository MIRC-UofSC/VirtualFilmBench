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

#ifndef EVENTFILTERDIALOG_H
#define EVENTFILTERDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QRadioButton>

#include "eventfilter.h"

namespace Ui {
class EventFilterDialog;
}

class EventFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EventFilterDialog(
        QWidget *parent = nullptr,
        const EventFilter *f = nullptr,
        EventFilterMatchingMode matchMode = EVENT_FILTER_MATCH_ALL);
    ~EventFilterDialog();

    EventFilter Filter() const;
private:
    Ui::EventFilterDialog *ui;
    int numRows;
    QGroupBox *matchModeGroupBox;
    QRadioButton *buttonAll;
    QRadioButton *buttonAny;

    void AddRow(
        QString attr=QString(),
        EventFilterComp comp=EVENT_FILTER_COMP_ATTR_EQUAL,
        QString pattern=QString());
    void ActivateRow(int row, bool activate=true);
    void ActivateRow(QComboBox *attrBox);

private slots:
    void ActivateThisRow();
};

#endif // EVENTFILTERDIALOG_H
