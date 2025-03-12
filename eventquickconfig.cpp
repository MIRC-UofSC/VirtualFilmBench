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

#include "eventquickconfig.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QScrollArea>
#include <QVBoxLayout>

EventQuickConfig::EventQuickConfig(
    QList<QStringList> &eventTypes, QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    QWidget *scrollContents = new QWidget;

    QVBoxLayout *scrollVbox = new QVBoxLayout(scrollContents);
    //grid = new QGridLayout(scrollContents);
    grid = new QGridLayout;
    scrollVbox->addLayout(grid,0);
    scrollVbox->addStretch(1);

    colAction=0;
    colLabel=1;
    colType=2;
    colSubType=3;
    colHotKey=4;

    Qt::Alignment align = Qt::AlignLeft | Qt::AlignTop;
    grid->addWidget(new QLabel("Delete"),0,colAction, align);
    grid->addWidget(new QLabel("Label"),0,colLabel, align);
    grid->addWidget(new QLabel("Event Type"),0,colType, align);
    grid->addWidget(new QLabel("Sub type"),0,colSubType, align);
    grid->addWidget(new QLabel("Hot-Key"),0,colHotKey, align);
    grid->setRowStretch(0, 0);

    for(auto &quickEvent : eventTypes)
        AddRow(quickEvent);

    scrollArea->setWidget(scrollContents);
    layout->addWidget(scrollArea,1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    QPushButton *addRowButton = new QPushButton("Add");
    addRowButton->setObjectName("add");
    buttonBox->addButton(addRowButton, QDialogButtonBox::ActionRole);
    QPushButton *makeDefaultButton = new QPushButton("Make Default");
    makeDefaultButton->setObjectName("default");
    buttonBox->addButton(makeDefaultButton, QDialogButtonBox::AcceptRole);
    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(
        buttonBox, SIGNAL(clicked(QAbstractButton*)),
        this, SLOT(Click(QAbstractButton*)));
    layout->addWidget(buttonBox);
}

QList<QStringList> EventQuickConfig::EventTypes() const
{
    QStringList data;
    QList<QStringList> eventTypes;
    QStringList usedHotKeys;

    for(auto widgetRow : widgets)
    {
        if(widgetRow.deleteButton->isChecked()) continue;
        if(widgetRow.label->text().isEmpty()) continue;

        data.clear();
        data << widgetRow.label->text();
        data << widgetRow.eventType->text();
        data << widgetRow.eventSubType->text();

        if(widgetRow.hotKey->currentIndex() > 0)
        {
            // ignore requests to use the same hotKey
            QString hotKey = widgetRow.hotKey->currentText();
            if(usedHotKeys.contains(hotKey))
            {
                data << QString();
            }
            else
            {
                data << hotKey;
                usedHotKeys << hotKey;
            }
        }
        else
            data << QString();

        eventTypes << data;
    }

    return eventTypes;
}

void EventQuickConfig::SaveAsDefaults() const
{
    QList<QStringList> eventTypes = EventTypes();

    QSettings settings;
    settings.beginGroup("event");
    if(settings.contains("quick")) settings.remove("quick");

    settings.beginWriteArray("quick");
    for(int i=0; !eventTypes.isEmpty(); ++i)
    {
        settings.setArrayIndex(i);
        QStringList l = eventTypes.takeFirst();
        settings.setValue("label", l[0]);
        settings.setValue("type", l[1]);
        settings.setValue("subtype", l[2]);
        settings.setValue("hotkey", l[3]);
    }
    settings.endArray();

    settings.endGroup();
}

void EventQuickConfig::AddRow(
    QString label, QString eventType, QString eventSubType, QString hotKey)
{
    const QStringList hotKeyChoices =
        {
            "-",
            "CTRL+1",
            "CTRL+2",
            "CTRL+3",
            "CTRL+4",
            "CTRL+5",
            "CTRL+6",
            "CTRL+7",
            "CTRL+8",
            "CTRL+9",
            "CTRL+0"
        };

    QCheckBox *checkbox;
    QLineEdit *le;
    QComboBox *combo;
    int k=0;

    int r = widgets.size();

    // add a new row to the widgets list
    widgets.resize(r+1);
    r = r+1;

    // add widgets to that row in the grid (and in the widgets list)
    checkbox = new QCheckBox();
    grid->addWidget(checkbox, r, colAction);
    widgets.last().deleteButton = checkbox;

    le = new QLineEdit(label);
    grid->addWidget(le, r, colLabel);
    widgets.last().label = le;

    le = new QLineEdit(eventType);
    grid->addWidget(le, r, colType);
    widgets.last().eventType = le;

    le = new QLineEdit(eventSubType);
    grid->addWidget(le, r, colSubType);
    widgets.last().eventSubType = le;

    combo = new QComboBox();
    combo->addItems(hotKeyChoices);
    if(!hotKey.isEmpty() && (k=combo->findText(hotKey))>=0)
        combo->setCurrentIndex(k);
    grid->addWidget(combo, r, colHotKey);
    widgets.last().hotKey = combo;

    grid->setRowStretch(r,1);
}


void EventQuickConfig::Click(QAbstractButton *button)
{
    if(button->objectName() == "default")
        SaveAsDefaults();
    else if(button->objectName() == "add")
        AddRow();
}
