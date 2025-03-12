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

#include "eventdataform.h"
#include "qmessagebox.h"
#include "ui_eventdataform.h"
#include "mainwindow.h"

#include <QLabel>
#include <QInputDialog>
#include <QMap>
#include <QMenu>
#include <QSignalBlocker>
#include <QTimer>

#include "attributelabel.h"

const QString EventDataForm::VariesString("<<<Varies>>>");

EventDataForm::EventDataForm(
    QWidget *parent,
    const VBFilmEvents *events,
    QList< const vbevent* > eventList)
    :
    QDialog(parent),
    ui(new Ui::EventDataForm)
{
    ui->setupUi(this);

    ui->frameStartBox->setMaximum(INT_MAX);
    ui->frameEndBox->setMaximum(INT_MAX);

    QMap<QString, bool> typeList; // use Map instead of Set for sorting

    // loop over the frames
    if(events) for (auto frameEvents = events->cbegin(),
         frameEnd = events->cend();
         frameEvents != frameEnd; ++frameEvents)
    {
        //loop over the events for this frame
        for(auto event = frameEvents->cbegin(), eventEnd = frameEvents->cend();
             event != eventEnd; ++event)
        {
            typeList[event->TypeName()] = true;

            //loop over the attributes for this event
            for(auto attr = event->Attributes().cbegin(),
                 attrEnd = event->Attributes().cend();
                 attr != attrEnd; ++attr)
            {
                attributeList[attr->first.toLower()] = attr->first;
                if(!attributeValues[attr->first.toLower()].
                     contains(attr->second,Qt::CaseInsensitive))
                {
                    attributeValues[attr->first.toLower()].append(attr->second);
                }
            }
        }
    }

    ui->eventTypeBox->insertItem(
        0, vbevent::coreEventTypeNames[VB_EVENT_GENERIC]);

    // remove the core events from the typeList -- instead list them first
    for(auto i = vbevent::coreEventTypeNames.cbegin(),
         end = vbevent::coreEventTypeNames.cend();
         i != end; ++i)
    {
        typeList.remove(*i);

        if(*i != vbevent::coreEventTypeNames[VB_EVENT_GENERIC] &&
            *i != vbevent::coreEventTypeNames[VB_EVENT_OTHER])
        {
            ui->eventTypeBox->addItem(*i);
        }
    }

    // add the remaining bespoke types
    for(auto i = typeList.cbegin(), end = typeList.cend(); i != end; ++i)
    {
        ui->eventTypeBox->addItem(i.key());
    }

    QStringList values;

    values = attributeValues.value(QString("CreatorContext").toLower());
    if(values.size() > 0)
    {
        values.sort();
        ui->creatorContextBox->addItems(values);
    }

    values = attributeValues.value(QString("CreatorID").toLower());
    if(values.size() > 0)
    {
        values.sort();
        ui->creatorIDBox->addItems(values);
    }

    ui->fullFrameCheckBox->setChecked(true);

    QString disabledStyle = ":disabled { color: #BBBBBB; font-style: italic; }";
    ui->minXBox->setStyleSheet(disabledStyle);
    ui->maxXBox->setStyleSheet(disabledStyle);
    ui->minYBox->setStyleSheet(disabledStyle);
    ui->maxYBox->setStyleSheet(disabledStyle);
    ui->minXBox->setValue(0.0f);
    ui->maxXBox->setValue(0.0f);
    ui->minYBox->setValue(0.0f);
    ui->maxYBox->setValue(0.0f);
    ui->continuousCheckBox->setChecked(false);
    ui->continuousCheckBox->setEnabled(false);

    if(!eventList.isEmpty())
    {
        int p;

        eventListInit = eventList;

        // ==Event Type==
        // nb: MatchExactly is case insensitive, contrary to intuition.
        QString eventType = UniformAttributeValue(eventList, "EventType");
        if(eventType == VariesString)
        {
            ui->eventTypeBox->insertItem(0, VariesString);
            p = 0;
        }
        else
        {
            p = ui->eventTypeBox->findText(eventType, Qt::MatchExactly);
        }

        if(p<0)
        {
            p = ui->eventTypeBox->count();
            ui->eventTypeBox->addItem(eventType);
        }
        ui->eventTypeBox->setCurrentIndex(p);

        // ==CreatorID and CreatorContext==
        // nb: MatchExactly is case insensitive, contrary to intuition.
        QString creator = UniformAttributeValue(eventList, "CreatorContext");
        if(creator == VariesString)
        {
            ui->creatorContextBox->insertItem(0, VariesString);
            p = 0;
        }
        else
        {
            p = ui->creatorContextBox->findText(creator, Qt::MatchExactly);
        }

        if(p<0)
        {
            p = ui->creatorContextBox->count();
            ui->creatorContextBox->addItem(creator);
        }
        ui->creatorContextBox->setCurrentIndex(p);

        creator = UniformAttributeValue(eventList,"CreatorID");
        if(creator == VariesString)
        {
            ui->creatorIDBox->insertItem(0, VariesString);
            p = 0;
        }
        else
        {
            p = ui->creatorIDBox->findText(creator, Qt::MatchExactly);
        }

        if(p<0)
        {
            p = ui->creatorIDBox->count();
            ui->creatorIDBox->addItem(creator);
        }
        ui->creatorIDBox->setCurrentIndex(p);

        // only allow frame and bounding box editing on single events
        if(eventList.size() == 1)
        {
            const vbevent *event = eventList.at(0);
            // == Start and End frames
            ui->frameStartBox->setValue(event->Start());
            ui->frameEndBox->setValue(event->End());

            // Bounding Box extents
            ui->minXBox->setValue(event->BoundsX0());
            ui->maxXBox->setValue(event->BoundsX1());
            ui->minYBox->setValue(event->BoundsY0());
            ui->maxYBox->setValue(event->BoundsY1());
            if(event->HasBounds())
            {
                ui->fullFrameCheckBox->setChecked(false);
            }
            if(event->Start() < event->End())
            {
                ui->continuousCheckBox->setChecked(event->IsContinuous());
                ui->continuousCheckBox->setEnabled(true);
            }
            else
            {
                ui->continuousCheckBox->setChecked(false);
                ui->continuousCheckBox->setEnabled(false);
            }
        }
        else
        {
            ui->formLayout->setRowVisible(ui->frameLayout, false);
            ui->formLayout->setRowVisible(ui->minXYLayout, false);
            ui->formLayout->setRowVisible(ui->maxXYLayout, false);
            ui->formLayout->setRowVisible(ui->fullFrameCheckBox, false);
            ui->formLayout->setRowVisible(ui->continuousCheckBox, false);
        }

        // Confidence
        ui->confidenceBox->
            setText(UniformAttributeValue(eventList, "Confidence"));

        // Notes
        ui->notesBox->setText(UniformAttributeValue(eventList, "Notes"));

        // Other Attributes
        QStringList otherAttributes;
        for(auto e : eventList)
        {
            for(auto &attr : e->Attributes())
            {
                if(!otherAttributes.contains(attr.first, Qt::CaseInsensitive))
                    otherAttributes.append(attr.first);
            }
        }

        for(auto &attr : otherAttributes)
            AddAttributeLine(attr, UniformAttributeValue(eventList, attr));

    }

    ui->frameStartBox->setKeyboardTracking(false);
    connect(ui->frameStartBox, &QSpinBox::valueChanged,
            this, &EventDataForm::FrameStartChanged);
    ui->frameEndBox->setKeyboardTracking(false);
    connect(ui->frameEndBox, &QSpinBox::valueChanged,
            this, &EventDataForm::FrameEndChanged);

    connect(ui->fullFrameCheckBox, &QCheckBox::stateChanged,
            this, &EventDataForm::FullFrameToggled);
    FullFrameToggled(ui->fullFrameCheckBox->checkState());

    connect(ui->addAttributeButton, &QPushButton::clicked,
            this, &EventDataForm::AddAttributeDialog);
}

EventDataForm::~EventDataForm()
{
    delete ui;
}

QString EventDataForm::UniformAttributeValue(
    QList< const vbevent* > eventList, QString name ) const
{
    if(eventList.isEmpty()) return QString();

    QString value = eventList.at(0)->Attribute(name);

    for(int i=1; i<eventList.size(); ++i)
    {
        if(eventList.at(i)->Attribute(name) != value)
            return(VariesString);
    }

    return value;
}

QList< vbevent > EventDataForm::Events() const
{
    QList< vbevent > eventList;

    for(auto eventInit : eventListInit)
    {
        EventID id = EventID(*eventInit);
        vbevent event(id);

        if(ui->eventTypeBox->currentText() == VariesString)
            event.SetType(eventInit->Type());
        else if(ui->eventTypeBox->currentText().isEmpty())
            event.SetType(VB_EVENT_GENERIC);
        else
            event.SetType(ui->eventTypeBox->currentText());

        if(ui->creatorContextBox->currentText() == VariesString)
            event.SetAttribute(
                "CreatorContext",
                eventInit->Attribute("CreatorContext"));
        else if(!ui->creatorContextBox->currentText().isEmpty())
            event.SetAttribute("CreatorContext",
                               ui->creatorContextBox->currentText());

        if(ui->creatorIDBox->currentText() == VariesString)
            event.SetAttribute(
                "CreatorID",
                eventInit->Attribute("CreatorID"));
        else if(!ui->creatorIDBox->currentText().isEmpty())
            event.SetAttribute("CreatorID", ui->creatorIDBox->currentText());

        if(eventListInit.size() == 1)
        {
            event.SetStartAndEnd(
                ui->frameStartBox->value(),
                ui->frameEndBox->value());

            if(ui->fullFrameCheckBox->isChecked())
            {
                event.SetBoundsX0X1Y0Y1(0.0f,0.0f,0.0f,0.0f);
            }
            else
            {
                event.SetBoundsX0X1Y0Y1(
                    ui->minXBox->value(),
                    ui->maxXBox->value(),
                    ui->minYBox->value(),
                    ui->maxYBox->value());
            }
            if(event.Start() < event.End())
                event.SetContinuous(ui->continuousCheckBox->isChecked());
        }
        else
        {
            event.SetStartAndEnd(eventInit->Start(), eventInit->End());
            event.SetBoundsX0X1Y0Y1(
                eventInit->BoundsX0(),
                eventInit->BoundsX1(),
                eventInit->BoundsY0(),
                eventInit->BoundsY1());
        }

        if(ui->confidenceBox->text() == VariesString)
        {
            QString value = eventInit->Attribute("Confidence");
            if(!value.isEmpty())
                event.SetAttribute("Confidence", value);
        }
        else
            event.SetAttribute("Confidence", ui->confidenceBox->text());

        if(ui->notesBox->text() == VariesString)
            event.notes = eventInit->notes;
        else
            event.notes = ui->notesBox->text();

        // Collect the custom attributes
        for(int r=0; r<ui->formLayout->rowCount(); ++r)
        {
            QLayoutItem *labelLayout =
                ui->formLayout->itemAt(r, QFormLayout::LabelRole);

            if(!labelLayout) continue;

            QLabel *label = qobject_cast<QLabel *>(labelLayout->widget());

            if(!label) continue;

            if(label->objectName() != "custom") continue;

            QWidget *widget =
                ui->formLayout->itemAt(r, QFormLayout::FieldRole)->widget();

            QString value;

            if(QComboBox *comboBox = qobject_cast<QComboBox *>(widget))
                value = comboBox->currentText();
            else if(QLineEdit *line = qobject_cast<QLineEdit *>(widget))
                value = line->text();
            else
            {
                value = QString("Input Widget Type %1 Not Handled")
                            .arg(widget->metaObject()->className());
                qDebug() << "Attribute Form: " << value;
            }

            if(value == VariesString)
            {
                value = eventInit->Attribute(label->text());
                if(!value.isEmpty())
                    event.SetAttribute(label->text(), value);
            }
            else
                event.SetAttribute(label->text(), value);
        }

        QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
        QString created = eventInit->Attribute("DateCreated");
        if(created.isEmpty())
        {
            event.SetAttribute("DateCreated", now);
            event.SetAttribute("DateModified", now);
        }
        else
        {
            event.SetAttribute("DateCreated", created);

            if(event == *eventInit)
            {
                // no edits, so copy orginal modification date to result
                event.SetAttribute("DateModified",
                                   eventInit->Attribute("DateModified"));
            }
            else
            {
                event.SetAttribute("DateModified",now);
            }
        }

        eventList.append(event);
    }

    return eventList;
}

void EventDataForm::FrameStartChanged(int value)
{
    if(value > ui->frameEndBox->value())
    {
        const QSignalBlocker blocker(ui->frameEndBox);
        ui->frameEndBox->setValue(value);
    }
    ui->continuousCheckBox->setEnabled(value < ui->frameEndBox->value());
}

void EventDataForm::FrameEndChanged(int value)
{
    if(value < ui->frameStartBox->value())
    {
        const QSignalBlocker blocker(ui->frameStartBox);
        ui->frameStartBox->setValue(value);
    }
    ui->continuousCheckBox->setEnabled(value > ui->frameStartBox->value());
}

void EventDataForm::FullFrameToggled(int value)
{
    ui->minXBox->setDisabled(bool(value));
    ui->maxXBox->setDisabled(bool(value));
    ui->minYBox->setDisabled(bool(value));
    ui->maxYBox->setDisabled(bool(value));

    if((value==0) &&
        (ui->minXBox->value() == 0.0f) &&
        (ui->maxXBox->value() == 0.0f) &&
        (ui->minYBox->value() == 0.0f) &&
        (ui->maxYBox->value() == 0.0f))
    {
        ui->maxXBox->setValue(0.9f);
        ui->maxYBox->setValue(0.9f);
        ui->minXBox->setValue(0.1f);
        ui->minYBox->setValue(0.1f);
    }
}

void EventDataForm::AddAttributeDialog()
{
    MainWindow *mainwindow = MainWindowAncestor(this);
    if(!mainwindow) return;
    QStringList attributes;

    vbevent eventDummy;
    eventDummy.SetType(ui->eventTypeBox->currentText());
    if(eventDummy.Type() != VB_EVENT_GENERIC)
        attributes.append(QString("SubType (%1)").arg(eventDummy.SubTypeName()));

    attributes.append(mainwindow->vbscan.DefaultAttributes());

    bool ok;
    QString item = QInputDialog::getItem(
        this, tr("Add Attribute"), tr("Attribute:"), attributes, 0, true, &ok);

    if((!ok) || item.isEmpty()) return;

    if(item == QString("SubType (%1)").arg(eventDummy.SubTypeName()))
        item = eventDummy.SubTypeName();

    if(vbproject::IsReservedAttribute(item))
    {
        if(item.compare("SubType",Qt::CaseInsensitive)==0)
        {
            int ret = QMessageBox::question(
                this, "SubType resolution",
                QString("'SubType' is a convenience column that aggregates "
                        "several attributes. The SubType attribute for %1 "
                        "events is '%2'. Would you like to add %2?").
                        arg(eventDummy.TypeName(), eventDummy.SubTypeName()),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Yes);

            if(ret == QMessageBox::Yes)
                item = eventDummy.SubTypeName();
            else
                return;
        }
        else
        {
            QMessageBox::warning(
                this, "Reserved Keyword",
                QString("'%1' is reserved for application use.").arg(item));
            return;
        }
    }

    QStringList values = mainwindow->vbscan.DefaultAttributeValues(item);
    values += attributeValues.value(item.toLower());
    if(values.isEmpty())
        AddAttributeLine(item);
    else
    {
        values.removeDuplicates();
        AddAttributeLine(item, values.first(), values );
    }
}

void EventDataForm::AddAttributeLine(
    QString name,
    QString initialValue,
    QStringList pulldownList)
{
    if(vbproject::IsReservedAttribute(name)) return;
    if(formAttributes.contains(name.toLower())) return;

    AttributeLabel *label = new AttributeLabel(name);
    label->setObjectName("custom");
    label->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(
        label, SIGNAL(customContextMenuRequested(QPoint)),
        label, SLOT(ContextMenu(QPoint)));
    connect(
        label, SIGNAL(RemoveAttribute()),
        this, SLOT(RemoveAttributeLine()));

    QComboBox *value = new QComboBox();
    value->setEditable(true);

    if(!pulldownList.isEmpty()) value->addItems(pulldownList);

    if(!initialValue.isEmpty())
    {
        int p = value->findText(initialValue,Qt::MatchExactly);
        if(p<0)
        {
            p = value->count();
            value->addItem(initialValue);
        }
        value->setCurrentIndex(p);
    }

    ui->formLayout->addRow(label,value);
    formAttributes.insert(name.toLower());
}

void EventDataForm::RemoveAttributeLine(QWidget *widget)
{
    if(!widget)
    {
        // if this is receiving a signal from a widget to remove
        // itself, queue that removal for the next pass through the
        // event loop so that the widget's method that was issuing
        // the request is able to return first. To remove it now would
        // result in a crash.
        widget = qobject_cast<QWidget *>(sender());
        if(!widget)
        {
            qDebug() << "RemoveAttribute called witout a widget target";
            return;
        }

        QTimer::singleShot(0, this,
             [this, widget]() { RemoveAttributeLine(widget); } );
    }
    else
    {
        AttributeLabel *label = qobject_cast<AttributeLabel *>(widget);
        if(label) formAttributes.remove(label->text().toLower());
        else qDebug() << "RemoveAttribute called without a label target";
        ui->formLayout->removeRow(widget);
    }
}
