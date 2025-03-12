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

#ifndef EVENTQUICKCONFIG_H
#define EVENTQUICKCONFIG_H

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QObject>
#include <QWidget>

struct QuickEventWidgetRow
{
    QCheckBox *deleteButton;
    QLineEdit *label;
    QLineEdit *eventType;
    QLineEdit *eventSubType;
    QComboBox *hotKey;
};

class EventQuickConfig : public QDialog
{
    Q_OBJECT
public:
    EventQuickConfig(QList<QStringList> &eventTypes, QWidget *parent=nullptr);
    QList<QStringList> EventTypes() const;

private:
    QGridLayout *grid;
    QList<QuickEventWidgetRow> widgets;

    int colAction;
    int colLabel;
    int colType;
    int colSubType;
    int colHotKey;

    void SaveAsDefaults() const;

private slots:
    void AddRow(
        QString label=QString(),
        QString eType=QString(),
        QString eSubType=QString(),
        QString hotKey=QString());
    void AddRow(QStringList l)
        { AddRow(l.value(0), l.value(1), l.value(2), l.value(3)); }
    void Click(QAbstractButton *button);

};

#endif // EVENTQUICKCONFIG_H
