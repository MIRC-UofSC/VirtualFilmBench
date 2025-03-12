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

#ifndef EVENTDATAFORM_H
#define EVENTDATAFORM_H

#include <QDialog>

#include "vbproject.h"

namespace Ui {
class EventDataForm;
}

class EventDataForm : public QDialog
{
    Q_OBJECT

public:
    explicit EventDataForm(
        QWidget *parent,
        const VBFilmEvents *allEvents,
        QList< const vbevent* > eventList );

    ~EventDataForm();

    QList< vbevent > Events() const;

private:
    Ui::EventDataForm *ui;
    QList< const vbevent* > eventListInit;

    QMap<QString, QString> attributeList;
    QMap<QString, QStringList> attributeValues;

    QSet<QString> formAttributes;

    static const QString VariesString;

    QString UniformAttributeValue(
        QList< const vbevent* > eventList,
        QString name) const;

private slots:
    void FrameStartChanged(int value);
    void FrameEndChanged(int value);
    void FullFrameToggled(int value);
    void AddAttributeDialog();
    void AddAttributeLine(
        QString name,
        QString value = QString(),
        QStringList pulldownList = QStringList());

public slots:
    void RemoveAttributeLine(QWidget *widget = nullptr);

};

#endif // EVENTDATAFORM_H
