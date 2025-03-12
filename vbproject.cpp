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

#include "vbproject.h"
#include "filmgauge.h"

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QDomDocument>
#include <QErrorMessage>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

vbproject::vbproject() :
    zeroframe(0),
    overlap_framestart(0.0f),
    overlap_frameend(0.0f),
    filmEvents(),
    filmEventsTableModel(nullptr),
    confidenceThreshold(0.0),
    confidenceThresholdIsEnabled(false)
{
    LoadDefaultAttributeValues(":/data/filmevent.dflt");

    // Add properties in the order they should appear in the Properties dialog
    properties.Add("FileURL");
    properties.SetMandatory("FileURL");
    properties.Add(Property(
        "Gauge", "35mm", PROPERTY_TYPE_TEXT, FilmGauge::gaugeList));
    properties.SetMandatory("Gauge");
    properties.Add(Property(
        "ScannerPolarity", "Positive", PROPERTY_TYPE_BOOL, {"Positive","Negative"}));
    properties.Add("CreatorID");
    properties.Add("CreatorContext");
    properties.Add("FilmAssetID");
    properties.Add("ReelID");
    properties.Add("Title");
    properties.Add("InputID");
    properties.SetMandatory("InputID");
    properties.Add("Notes");

    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    properties.Add(Property("CreationDate",now,PROPERTY_SYSTEM_DATE));
    properties.Add(Property("ModificationDate",now,PROPERTY_SYSTEM_DATE));
}

void vbproject::SetProperties(PropertyList l)
{
    properties = l;

    QSettings settings;
    settings.beginGroup("creator-info");
    QString id = settings.value("id").toString();
    if(id.isEmpty())
    {
        id = properties.Value("CreatorID");
        if(!id.isEmpty())
            settings.setValue("id", id);
    }
    QString context = settings.value("context").toString();
    if(context.isEmpty())
    {
        context = properties.Value("CreatorContext");
        if(!context.isEmpty())
            settings.setValue("context", context);
    }
    settings.endGroup();
}

int vbproject::FramesPerFoot() const
{
    class FilmGauge g(FilmGauge());
    return g.FramesPerFoot();
}

bool vbproject::save(QString filename)
{
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    SetLastModificationDate(now);

    // Create a new XML document
    QDomDocument doc("vbproject");
    QDomElement root = doc.createElement("vbproject");
    doc.appendChild(root);

    if(!FilmNotes().isEmpty())
    {
        QDomElement notesElement = doc.createElement("film_notes");
        QDomText notesText = doc.createTextNode(FilmNotes());
        notesElement.appendChild(notesText);
        root.appendChild(notesElement);
    }

    // Add Document Properties
    QDomElement propElement = doc.createElement("Properties");
    root.appendChild(propElement);
    for(auto &prop : properties.List())
    {
        if(prop.Value().isEmpty()) continue;
        QDomElement propNode = doc.createElement(prop.Name());
        QDomText propVal = doc.createTextNode(prop.Value());
        propNode.appendChild(propVal);
        propElement.appendChild(propNode);
    }

    if(confidenceThresholdIsEnabled && confidenceThreshold > 0.0f)
    {
        QDomElement propNode = doc.createElement("ConfidenceThreshold");
        QDomText propVal =
            doc.createTextNode(QVariant(confidenceThreshold).toString());
        propNode.appendChild(propVal);
        propElement.appendChild(propNode);
    }

    // Add project settings to the XML document
    QDomElement settings = doc.createElement("settings");
    root.appendChild(settings);
    settings.setAttribute("filmgauge", FilmGauge());
    settings.setAttribute("zeroframe", QString::number(zeroframe));
    settings.setAttribute("overlap_framestart",
                          QString::number(overlap_framestart));
    settings.setAttribute("overlap_frameend",
                          QString::number(overlap_frameend));
    // Add event list to the XML document
    QDomElement eventList = doc.createElement("eventList");
    root.appendChild(eventList);

    bool done=false;
    auto frameIterator = filmEvents.cbegin(), frameEnd = filmEvents.cend();
    while(!done)
    {
        const QList<vbevent> * frameEvents;
        if(frameIterator == frameEnd)
        {
            frameEvents = &lowConfidenceEvents;
            done = true;
        }
        else
        {
            frameEvents = &(frameIterator.value());
            frameIterator++;
        }
        for (const vbevent &event : *frameEvents)
        {
            QDomElement eventNode = doc.createElement("event");
            eventNode.setAttribute("eventtype", event.TypeName());

            eventNode.setAttribute("start", QString::number(event.Start()));
            if(event.End() > event.Start())
                eventNode.setAttribute("end", QString::number(event.End()));

            if(event.HasBounds())
            {
                eventNode.setAttribute("boundingbox_x0",
                                       QString::number(event.BoundsX0()));
                eventNode.setAttribute("boundingbox_x1",
                                       QString::number(event.BoundsX1()));
                eventNode.setAttribute("boundingbox_y0",
                                       QString::number(event.BoundsY0()));
                eventNode.setAttribute("boundingbox_y1",
                                       QString::number(event.BoundsY1()));
            }

            if(event.IsContinuous())
                eventNode.setAttribute("IsContinuous", QString("true"));

            QStringList specialReserved;
            specialReserved << "CreatorID" << "CreatorContext" <<
                "DateCreated" << "DateModified" << "notes";
            for(auto &spec : specialReserved)
            {
                if(!event.Attribute(spec).isEmpty())
                    eventNode.setAttribute(spec, event.Attribute(spec));
            }

            for(auto attr = event.Attributes().cbegin(),
                 end = event.Attributes().cend();
                 attr != end; ++attr)
            {
                if(IsReservedAttribute(attr->first)) continue;
                eventNode.setAttribute(attr->first, attr->second);
            }
            eventList.appendChild(eventNode);
        }
    }

    if(filmEventsTableModel && !filmEventsTableModel->Columns().isEmpty())
    {
        QDomElement columnsView = doc.createElement("column-view-order");
        root.appendChild(columnsView);
        for(auto &col : filmEventsTableModel->Columns())
        {
            QDomElement colNode = doc.createElement("column");
            QDomText colName = doc.createTextNode(col);
            colNode.appendChild(colName);
            columnsView.appendChild(colNode);
        }
    }

    // Save the XML document to the specified file
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&file);
        stream << doc.toString();
        file.close();
        return true;
    }

    return false;
}

bool vbproject::load(QString filename)
{
    // Clear existing event list
    //qDeleteAll(events);
    FilmEventsTableModel()->Clear();

    // Load XML data from file
    QDomDocument doc("vbproject");
    QFile file(filename);
    if(!(file.open(QIODevice::ReadOnly | QIODevice::Text)))
        return false;
    doc.setContent(&file);
    file.close();

    QDomElement root = doc.documentElement();

    QDomElement notesElement = root.firstChildElement("film_notes");
    if(!notesElement.isNull())
        SetFilmNotes(notesElement.text());
    else
        SetFilmNotes(QString());

    // Extract Document Properties
    QDomElement propElement = root.firstChildElement("Properties");
    if(!propElement.isNull())
    {
        QDomNodeList propNodeList = propElement.childNodes();

        for(int i=0; i<propNodeList.size(); ++i)
        {
            if(propNodeList.at(i).toElement().tagName() == "ConfidenceThreshold")
            {
                SetConfidenceThreshold(
                    propNodeList.at(i).toElement().text().toFloat(),
                    true);
            }
            else
            {
                properties.SetValue(
                    propNodeList.at(i).toElement().tagName(),
                    propNodeList.at(i).toElement().text());
            }
        }
    }

    // read some properties from metadata for compatibility with old XML format
    // Extract project metadata
    QDomElement metadata = root.firstChildElement("metadata");
    if(!metadata.isNull())
    {
        if(FileURL().isEmpty() && metadata.hasAttribute("fileURL"))
            SetFileURL(metadata.attribute("fileURL"));
        if(Title().isEmpty() && metadata.hasAttribute("title"))
            SetTitle(metadata.attribute("title"));
        if(RollID().isEmpty() && metadata.hasAttribute("roll_id"))
            SetRollID(metadata.attribute("roll_id"));
        if(FilmAssetID().isEmpty() && metadata.hasAttribute("film_asset_id"))
            SetFilmAssetID(metadata.attribute("film_asset_id"));
        if(Notes().isEmpty() && metadata.hasAttribute("notes"))
            SetNotes(metadata.attribute("notes"));

        if(CreationDate().isEmpty() && metadata.hasAttribute("creationdate"))
            SetCreationDate(metadata.attribute("creationdate"));
        if(LastModifiedDate().isEmpty() &&
            metadata.hasAttribute("lastmodificationdate")
            )
            SetLastModificationDate(metadata.attribute("lastmodifieddate"));
    }

    // Extract project settings
    QDomElement settings = root.firstChildElement("settings");
    if(FileURL().isEmpty() && settings.hasAttribute("fileURL"))
        SetFileURL(settings.attribute("fileURL"));
    if(FilmGauge().isEmpty() && settings.hasAttribute("filmgauge"))
        SetFilmGauge(settings.attribute("filmgauge"));
    if(settings.hasAttribute("zeroframe"))
        zeroframe = settings.attribute("zeroframe").toInt();
    else
        zeroframe = 0;

    overlap_framestart = settings.attribute("overlap_framestart").toFloat();
    overlap_frameend = settings.attribute("overlap_frameend").toFloat();

    FilmEventsTableModel()->BeginBatchAddEvent();

    // Extract event list
    QDomElement eventList = root.firstChildElement("eventList");

    // use a list of tags for backwards compatibility with previous vesions
    QStringList compatibleTags = { "event", "event_join" };
    for ( const auto& tag : compatibleTags )
    {
        QDomNodeList eventNodes = eventList.elementsByTagName(tag);
        for (int i = 0; i < eventNodes.size(); i++)
        {
            QDomElement eventNode = eventNodes.at(i).toElement();

            // Extracted known/expected/special values for bespoke
            // processing. Remove them as they're processed so that
            // they don't accidentally get processed a second time in
            // the final "other attributes" loop.

            vbevent event(eventNode.attribute("start").toInt());
            eventNode.removeAttribute("start");

            if(eventNode.hasAttribute("eventtype"))
            {
                event.SetType(eventNode.attribute("eventtype"));
                eventNode.removeAttribute("eventtype");
            }
            else if(tag == "event_join")
            {
                event.SetType(VB_EVENT_JOIN);
            }

            if(eventNode.hasAttribute("end"))
            {
                event.SetEnd(eventNode.attribute("end").toInt());
                eventNode.removeAttribute("end");
            }

            if(eventNode.hasAttribute("boundingbox_w"))
            {
                event.SetBoundsCenterAndSize(
                    eventNode.attribute("boundingbox_x").toFloat(),
                    eventNode.attribute("boundingbox_y").toFloat(),
                    eventNode.attribute("boundingbox_w").toFloat(),
                    eventNode.attribute("boundingbox_h").toFloat());
                eventNode.removeAttribute("boundingbox_x");
                eventNode.removeAttribute("boundingbox_y");
                eventNode.removeAttribute("boundingbox_w");
                eventNode.removeAttribute("boundingbox_h");
            }
            else if(eventNode.hasAttribute("boundingbox_x0"))
            {
                event.SetBoundsX0X1Y0Y1(
                    eventNode.attribute("boundingbox_x0").toFloat(),
                    eventNode.attribute("boundingbox_x1").toFloat(),
                    eventNode.attribute("boundingbox_y0").toFloat(),
                    eventNode.attribute("boundingbox_y1").toFloat());
                eventNode.removeAttribute("boundingbox_x0");
                eventNode.removeAttribute("boundingbox_x1");
                eventNode.removeAttribute("boundingbox_y0");
                eventNode.removeAttribute("boundingbox_y1");
            }

            if(eventNode.hasAttribute("IsContinuous"))
            {
                bool c(eventNode.attribute("IsContinuous").toLower() == "true");
                event.SetContinuous(c);
                eventNode.removeAttribute("IsContinuous");
            }
            // add any other custom attibutes given
            QDomNamedNodeMap attrMap = eventNode.attributes();

            for (int i = 0; i < attrMap.count(); ++i)
            {
                QDomAttr attr = attrMap.item(i).toAttr();
                QString attrName = attr.name();              
                QString attrValue = attr.value();

                event.SetAttribute(attrName, attrValue);
            }

            FilmEventsTableModel()->AddEvent(event);
        }
    }

    FilmEventsTableModel()->EndBatchAddEvent();

    QDomElement columnsView = root.firstChildElement("column-view-order");
    if(!columnsView.isNull())
    {
        QStringList columns;
        QDomNodeList colNodeList = columnsView.elementsByTagName("column");
        for(int i=0; i<colNodeList.size(); ++i)
            columns << colNodeList.at(i).toElement().text();
        if(!columns.isEmpty())
            FilmEventsTableModel()->SetColumns(columns);
    }

    if(InputID().isEmpty()) SetInputID(QUrl(FileURL()).fileName());

    return true;
}

EventSet vbproject::ImportEvents(const QString filename)
{
    QFile infile(filename);
    QDomDocument xmlBOM;

    EventSet eventSet;

    qDebug() << "ImportEvents";

    if(!infile.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox msgError;
        msgError.setText(
            QString("Cannot open file %1").arg(filename));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Error opening XML file");
        msgError.exec();
        return eventSet;
    }

    qDebug() << "Reading XML";

    // Set data into the QDomDocument for processing
    QDomDocument::ParseResult parse = xmlBOM.setContent(&infile);
    infile.close();

    // nb: the 6.5 docs say casting to bool returns true if there is an
    // error and false otherwise, but the opposite is the case.
    if(!bool(parse))
    {
        QMessageBox msgError;
        msgError.setText(
            QString("Error reading XML file, line %1, column %2<br/>%3")
                .arg(parse.errorLine)
                .arg(parse.errorColumn)
                .arg(parse.errorMessage.toHtmlEscaped()));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Error reading XML file");
        msgError.exec();
        return eventSet;
    }

    QDomElement root = xmlBOM.documentElement();

    QDomElement element;
    QDomElement attr;
    QDomElement subAttr;

    vbevent dflt;

    FilmEventsTableModel()->BeginBatchAddEvent();

    for(element = root.firstChildElement();
         !element.isNull();
         element = element.nextSiblingElement())
    {
        if(element.tagName() != "event")
        {
            dflt.SetAttribute(
                element.tagName(),
                element.firstChild().toText().data());
            continue;
        }

        int start = 0;
        int end = 0;
        QString coord;
        QString eventTypeName = QString();
        QString tagName;

        vbevent event;

        for(auto &a: dflt.Attributes())
            event.SetAttribute(a.first, a.second);

        // loop over the attributes of this event
        for(attr = element.firstChildElement();
             !attr.isNull();
             attr = attr.nextSiblingElement())
        {
            tagName = attr.tagName();
            if(tagName == "join") tagName = "splice";
            else if (tagName=="event_confidence") tagName="confidence";


            if (tagName=="location_absolute_in")
            {
                start=attr.firstChild().toText().data().toInt();
                event.SetStart(start);
            }
            else if (tagName=="location_absolute_out")
            {
                end=attr.firstChild().toText().data().toInt();
                event.SetEnd(end);
            }
            else if (tagName=="location_pixels")
            {
                coord=attr.firstChild().toText().data();
                if(!coord.isEmpty())
                {
                    QStringList co = coord.split(",");
                    if (co.length()>3)
                    {
                        event.SetBoundsCenterAndSize(
                            co[0].toDouble(),
                            co[1].toDouble(),
                            co[2].toDouble(),
                            co[3].toDouble());
                    }
                }
            }
            else if (tagName=="location_is_continuous")
            {
                bool c(attr.firstChild().toText().data().toLower()=="true");
                event.SetContinuous(c);
            }
            else if (vbevent::coreEventTypeNames.
                     contains(tagName, Qt::CaseInsensitive))
            {
                event.SetType(tagName);
                event.SetAttribute(
                    event.SubTypeName(), attr.firstChild().toText().data());
            }
            else if (attr.tagName()=="event_type")
            {
                eventTypeName = attr.firstChild().toText().data();
                event.SetType(eventTypeName);
            }
            else
            {
                event.SetAttribute(tagName, attr.firstChild().toText().data());
            }
        }

        FilmEventsTableModel()->AddEvent(event);

        eventSet += event;
    }

    FilmEventsTableModel()->EndBatchAddEvent();

    qDebug() << "Import done; returning";

    return eventSet;
}

void vbproject::ExportEvents(
    const QString filename, std::function<bool(int)> include) const
{
    QFile outfile(filename);
    if(!outfile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox msgError;
        msgError.setText(
            QString("Cannot open file %1").arg(filename));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Error opening XML file");
        msgError.exec();
        return;
    }
    QDomDocument doc("Virtual_Bench");
    QDomElement root = doc.createElement("Virtual_Bench");
    doc.appendChild(root);

    QDomElement attr;
    QDomText attrVal;
    QList<Property> propList = Properties().List();

    for(Property &prop : propList)
    {
        if(prop.Name() == QString("FileURL")) continue;

        if(!prop.Value().isEmpty())
        {
            attr = doc.createElement(prop.Name());
            attrVal = doc.createTextNode(prop.Value());
            attr.appendChild(attrVal);
            root.appendChild(attr);
        }
    }

    int row=0;
    for(auto frameEvents = filmEvents.cbegin(), frameEnd = filmEvents.cend();
         frameEvents != frameEnd; frameEvents++, row++)
    {
        for(auto event = frameEvents->cbegin(), end = frameEvents->cend();
             event != end; event++, row++)
        {
            if(!include(row)) continue;

            QDomElement eventNode = doc.createElement("event");
            root.appendChild(eventNode);

            attr = doc.createElement(event->TypeName());
            attrVal = doc.createTextNode(event->SubType());
            attr.appendChild(attrVal);
            eventNode.appendChild(attr);

            attr = doc.createElement("location_absolute_in");
            attrVal = doc.createTextNode(QVariant(event->Start()).toString());
            attr.appendChild(attrVal);
            eventNode.appendChild(attr);

            attr = doc.createElement("location_absolute_out");
            attrVal = doc.createTextNode(QVariant(event->End()).toString());
            attr.appendChild(attrVal);
            eventNode.appendChild(attr);

            if(event->HasBounds())
            {
                attr = doc.createElement("location_pixels");
                QString s = QString("%1,%2,%3,%4")
                                .arg(event->BoundsCenterX())
                                .arg(event->BoundsCenterY())
                                .arg(event->BoundsSizeX())
                                .arg(event->BoundsSizeY());
                attrVal = doc.createTextNode(s);
                attr.appendChild(attrVal);
                eventNode.appendChild(attr);
            }

            if(event->IsContinuous())
            {
                attr = doc.createElement("location_is_continuous");
                attrVal = doc.createTextNode("true");
                attr.appendChild(attrVal);
                eventNode.appendChild(attr);
            }

            if(!event->notes.isEmpty())
            {
                attr = doc.createElement("notes");
                attrVal = doc.createTextNode(event->notes);
                attr.appendChild(attrVal);
                eventNode.appendChild(attr);
            }

            for(auto &a : event->Attributes())
            {
                attr = doc.createElement(a.first);
                attrVal = doc.createTextNode(a.second);
                attr.appendChild(attrVal);
                eventNode.appendChild(attr);
            }
        }
    }

    QTextStream stream(&outfile);
    stream << doc.toString();
    outfile.close();
}

QList< vbevent* > vbproject::FilmEventsForFrame(uint32_t frame)
{
    QList< vbevent* > events;

    // Get the frame events that start here
    if(FilmEvents().contains(frame))
        for(vbevent &e: FilmEvents()[frame])
            events.append(&e);

    // Get the two-frame events that end here
    if((frame>0) && FilmEvents().contains(frame-1))
    {
        for(vbevent &e: FilmEvents()[frame-1])
        {
            if(e.End()>=frame)
                events.append(&e);
        }
    }

    // Get the multi-frame events that extend to/past here
    for(vbevent* ep : multiFrameEvents)
    {
        // skip Start == frame and frame-1, since they're included above
        uint32_t base = frame;
        if(base>1) base -= 1;

        if((ep->Start() < base) && (ep->End() >= frame))
            events.append(ep);
    }

    return events;
}

VBFilmEventsTableModel * vbproject::FilmEventsTableModel()
{
    if(!filmEventsTableModel)
    {
        filmEventsTableModel = new VBFilmEventsTableModel(
            nullptr, &filmEvents, &lowConfidenceEvents);

        // Connect the slots that keep the MultiFrameEvent list up-to-date
        connect(
            filmEventsTableModel, SIGNAL(MultiFrameEventAdded(vbevent*)),
            this, SLOT(MultiFrameEventAdd(vbevent*)));

        connect(
            filmEventsTableModel, SIGNAL(MultiFrameEventDeleted(vbevent*)),
            this, SLOT(MultiFrameEventDelete(vbevent*)));

        connect(
            filmEventsTableModel, SIGNAL(MultiFrameEventsCleared()),
            this, SLOT(MultiFrameEventDeleteAll()));
    }

    return filmEventsTableModel;
}

void vbproject::SetConfidenceThreshold(float threshold, bool enabled)
{
    filmEventsTableModel->SetConfidenceThreshold(enabled?threshold:0.0f);

    // adding to the trash bin?
    if(enabled && ((!this->confidenceThresholdIsEnabled) ||
                    threshold > this->confidenceThreshold))
    {
        QList<int> rows;
        int row=0;
        for(auto frameEvents = filmEvents.cbegin(),
             frameEnd = filmEvents.cend();
             frameEvents != frameEnd; frameEvents++)
        {
            for(auto event = frameEvents->cbegin(),
                 end = frameEvents->cend();
                 event != end; event++, row++)
            {
                if(event->EffectiveConfidence() < threshold)
                {
                    lowConfidenceEvents.append(*event);
                    rows.append(row);
                }
            }
        }
        while(!rows.isEmpty())
            filmEventsTableModel->DeleteEvent(rows.takeLast());
    }
    else // restoring from the trash
    {
        QList<int> indices;
        int idx=0;
        for(auto event = lowConfidenceEvents.cbegin(),
             end = lowConfidenceEvents.cend();
             event != end; event ++, idx++)
        {
            if(event->EffectiveConfidence() >= threshold)
            {
                filmEventsTableModel->AddEvent(*event);
                indices.append(idx);
            }
        }
        while(!indices.isEmpty())
            lowConfidenceEvents.remove(indices.takeLast());
    }

    confidenceThreshold = threshold;
    confidenceThresholdIsEnabled = enabled;
}

QStringList vbproject::AttributesInUse() const
{
    if(filmEvents.isEmpty()) return QStringList();

    QSet<QString> attrSet;

    // seed ths set with the reserved attributes that can be used
    // as filters
    attrSet << "EventType" << "CreatorContext" << "CreatorID" << "Notes";

    for (auto it = filmEvents.cbegin(); it != filmEvents.cend(); ++it)
    {
        for (const vbevent &event : it.value())
        {
            for(auto attr = event.Attributes().cbegin(),
                 end = event.Attributes().cend();
                 attr != end; ++attr)
            {
                if(IsReservedAttribute(attr->first)) continue;
                attrSet << attr->first;
            }
        }
    }
    QStringList list = attrSet.values();
    list.sort(Qt::CaseInsensitive);
    return list;
}

void vbproject::UpdateFrameEventBoundingBox(vbevent *event,
        float x0, float x1, float y0, float y1)
{
    if(event)
        event->SetBoundsX0X1Y0Y1(x0,x1,y0,y1);
}

void vbproject::UpdateFrameEventBoundingBox(int frame, int eventNum,
        float x0, float x1, float y0, float y1)
{
    qDebug() << "Frame " << frame << ", seq " << eventNum;
    if(!filmEvents.contains(frame))
    {
        QMessageBox msgError;
        msgError.setText(
            QString("Internal Error: Received signal to update non-existent event at %1").
            arg(frame));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Event does not exist");
        msgError.exec();
        return;
    }

    if(eventNum >= filmEvents[frame].size())
    {
        QMessageBox msgError;
        msgError.setText(
            QString("Internal Error: Received signal to update non-existent event at %1[%2]").
            arg(frame).arg(eventNum));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Event does not exist");
        msgError.exec();
        return;
    }

    filmEvents[frame][eventNum].SetBoundsX0X1Y0Y1(x0,x1,y0,y1);
}

void vbproject::MultiFrameEventAdd(vbevent *e)
{
    multiFrameEvents.append(e);
}

void vbproject::MultiFrameEventDelete(vbevent *e)
{
    if(!multiFrameEvents.removeOne(e))
    {
        QMessageBox msgError;
        msgError.setText(
            QString("Internal Error: attempt to remove unlisted multi-frame event %1-%2").
            arg(e->Start()).arg(e->End()));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Event not in MFE list");
        msgError.exec();
        return;
    }
}

void vbproject::MultiFrameEventDeleteAll()
{
    multiFrameEvents.clear();
}

QStringList vbproject::DefaultAttributeValues(QString attribute) const
{
    // Use case-insensitive lookup for the key value and return the
    // list of values at that key
    QRegularExpression attr(
        vbevent::MakeAttributeName(attribute),
        QRegularExpression::CaseInsensitiveOption);

    int i = DefaultAttributes().indexOf(attr);
    if(i<0) return QStringList();

    return defaultAttributeValues[DefaultAttributes().at(i)];
}

void vbproject::LoadDefaultAttributeValues(QString filename)
{
    // Clear existing defaults
    defaultAttributeValues.clear();

    // Load XML data from file
    QDomDocument doc;
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Cannot open " << filename;
        return;
    }
    doc.setContent(&file);
    file.close();

    QDomElement defaults = doc.documentElement();
    if(defaults.isNull() || defaults.tagName() != "event-attribute-values")
        return;

    QDomNodeList attributes = defaults.elementsByTagName("attribute");
    QStringList values;

    for(int i=0; i<attributes.size(); ++i)
    {
        QDomElement attr = attributes.at(i).toElement();
        QDomElement name = attr.firstChildElement("name");
        if(name.isNull()) continue;

        values.clear();

        QDomNodeList valueNodes = attr.elementsByTagName("value");
        for(int j=0; j<valueNodes.size(); ++j)
        {
            // Take the values given and add them to the a QStringList
            // of possible values, with the default(s) at the
            // front of the list.

            // nb: if more than one default value is given, they will
            // be in the list in reverse of the order they appear in
            // the XML file.
            QDomElement valueNode = valueNodes.at(j).toElement();
            if(
                valueNode.hasAttribute("default") &&
                (valueNode.attribute("default").toLower() == "true")
                )
            {
                values.prepend(valueNode.text());
            }
            else
            {
                values.append(valueNode.text());
            }
        }

        defaultAttributeValues[vbevent::MakeAttributeName(name.text())] = values;
    }
}

//#######################################################################

static VBFilmEvents::size_type FilmEventsSize(const VBFilmEvents &e)
{
    VBFilmEvents::size_type s(0);

    for (auto it = e.cbegin(); it != e.cend(); ++it)
        s += it.value().length();

    return s;
}


VBFilmEvents::size_type vbproject::NumFilmEvents()
{
    return FilmEventsSize(filmEvents);
}


//#######################################################################
// Table Model Class
//#######################################################################

VBFilmEventsTableModel::VBFilmEventsTableModel(
    QObject *parent,
    VBFilmEvents *filmEventsArg,
    QList<vbevent> *trashArg) :
    QAbstractTableModel(parent),
    filmEvents(filmEventsArg),
    trash(trashArg),
    confidenceThreshold(0.0f),
    inBatchAddEventMode(false)
{
    columns << "Frame" << "Type" << "SubType" << "Notes" << "CreatorContext" <<
        "CreatorID" << "Confidence" << "Details";
}

int VBFilmEventsTableModel::rowCount(const QModelIndex &parent) const
{
    // parent is only applicable to to tree models
    if ( parent.isValid() || !filmEvents )
        return 0;
    else
        return int(FilmEventsSize(*filmEvents));
}

int VBFilmEventsTableModel::columnCount(const QModelIndex &parent) const
{
    // parent is only applicable to to tree models
    if (parent.isValid())
        return 0;
    else
        return columns.count();
}

const vbevent *VBFilmEventsTableModel::EventAtRow(int row) const
{
    const vbevent *event(nullptr);

    // Find the frame whose event list contains the target event
    // Then pick the event from the frame's list at the correct position.

    for (auto i = filmEvents->begin(), end = filmEvents->end();
         i != end && !event; ++i)
    {
        if(row < i.value().length())
        {
            event = &(i.value().at(row));
        }
        else
            row -= i.value().length();
    }

    return event;
}

int VBFilmEventsTableModel::RowOfEvent(const vbevent *event) const
{
    int row=0;
    for (auto i = filmEvents->begin(), end = filmEvents->end();
         i != end; ++i)
    {
        if((*i)[0].Start() >= event->Start())
        {
            if((*i)[0].Start() > event->Start()) break;

            for(auto &e : (*i))
            {
                if(e.ID() == event->ID()) return row;
                else row++;
            }
        }

        row += i->length();
    }

    return -1;
}

int VBFilmEventsTableModel::RowAtFrame(uint32_t frame) const
{
    int row=0;
    for (auto i = filmEvents->begin(), end = filmEvents->end();
         i != end; ++i)
    {
        if((*i)[0].Start() == frame) return row;
        if((*i)[0].Start() > frame) return std::max(row-1,0);
        row += i->length();
    }

    return std::max(row-1,0); // return last row if we're off the end
}

Qt::ItemFlags VBFilmEventsTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::ItemIsEnabled;

    QString col = columns.at(index.column());
    if(
        (col.compare("Start",Qt::CaseInsensitive)==0) ||
        (col.compare("Frame",Qt::CaseInsensitive)==0) ||
        (col.compare("End",Qt::CaseInsensitive)==0) ||
        (col.compare("DateModified",Qt::CaseInsensitive)==0) ||
        (col.compare("DateCreated",Qt::CaseInsensitive)==0) ||
        (col.compare("Details",Qt::CaseInsensitive)==0)
        )
        return QAbstractItemModel::flags(index);
    else
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

QVariant VBFilmEventsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    if((role == Qt::DisplayRole || role == Qt::EditRole) && filmEvents)
    {
        if(index.column() >= columns.count()) return QString("<###>");

        const vbevent *event = EventAtRow(index.row());
        QString col = columns.at(index.column());

        if(event)
        {
            if(col.compare("Details", Qt::CaseInsensitive)==0)
            {
                QStringList l;
                if(!event->notes.isEmpty())
                    l << QString("Notes: %1").arg(event->notes);
                for(auto attr = event->Attributes().cbegin(),
                    end = event->Attributes().cend();
                    attr != end; ++attr)
                {
                    l << QString("%1: %2").arg(attr->first,attr->second);
                }
                return l.join("\n");
            }
            else
                return event->Attribute(col);
        }
    }

    return QVariant();
}

bool VBFilmEventsTableModel::setData(
    const QModelIndex &index,
    const QVariant &value,
    int role)
{
    if (index.isValid() && role == Qt::EditRole)
    {
        vbevent event = *(EventAtRow(index.row()));
        QString col = columns.at(index.column());
        event.SetAttribute(col, value.toString());
        event.SetAttribute(
            "DateModified",
            QDateTime::currentDateTime().toString(Qt::ISODate));
        UpdateEventAtRow(index.row(), event);
        emit dataChanged(index,index);
        emit FilmEventsTableUpdated();
        return true;
    }
    else
        return false;
}


QVariant VBFilmEventsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if((role == Qt::DisplayRole) && (orientation == Qt::Horizontal))
    {
        if(section < columns.count())
            return columns.at(section);
        else
        {
            qDebug() << "Error getting table header: column out of range";
            return QString("<###>");
        }
    }
    if((role == Qt::DisplayRole) && (orientation == Qt::Vertical))
    {
        return QString("\u205D"); // unicode for the tricolon  ⁝
    }

    return QVariant();
}

void VBFilmEventsTableModel::Clear()
{
    if(!filmEvents) return;

    beginResetModel();
    filmEvents->clear();
    endResetModel();

    emit MultiFrameEventsCleared();
}

void VBFilmEventsTableModel::AddEvent(const vbevent &event)
{
    if(!filmEvents) return;

    if(trash && event.EffectiveConfidence() < confidenceThreshold)
    {
        trash->append(event);
        return;
    }

    // find which row this event will be after it is inserted
    int r(0);
    for (auto i = filmEvents->cbegin(), end = filmEvents->cend(); i != end; ++i)
    {
        if(i.key() < event.Start())
            r += i.value().length();
        else
            break;
    }

    int idx(0);
    if(filmEvents->contains(event.Start()))
    {
        for(idx=0; idx<(*filmEvents)[event.Start()].size(); ++idx, ++r)
        {
            if(event < (*filmEvents)[event.Start()][idx]) break;
        }
    }

    if(inBatchAddEventMode)
        (*filmEvents)[event.Start()].insert(idx,event);
    else
    {
        beginInsertRows(QModelIndex(), r, r);
        (*filmEvents)[event.Start()].insert(idx,event);
        endInsertRows();

        emit FilmEventsTableUpdated();
    }

    if(event.IsMultiFrame())
    {
        emit MultiFrameEventAdded(&((*filmEvents)[event.Start()][idx]));
    }
}

void VBFilmEventsTableModel::BeginBatchAddEvent()
{
    beginResetModel();
    inBatchAddEventMode = true;
}

void VBFilmEventsTableModel::EndBatchAddEvent()
{
    endResetModel();
    inBatchAddEventMode = false;
    emit FilmEventsTableUpdated();
}

void VBFilmEventsTableModel::UpdateEventAtRow(int row, const vbevent &event)
{
    if(trash && event.EffectiveConfidence() < confidenceThreshold)
    {
        trash->append(event);
        DeleteEvent(row);
        return;
    }

    // distingish the "row" of the frame's event list from the row of
    // the master table (all frames)
    int r = row;

    for (auto i = filmEvents->begin(), end = filmEvents->end();
         i != end; ++i)
    {
        if(r < i->length())
        {
            bool wasMulti = (*i)[r].IsMultiFrame();
            bool isMulti = event.IsMultiFrame();

            // if the update changes the frame or the ordering
            // within the frame,
            // delete the old and then add it as new.
            if(
                (i->at(r).Start() == event.Start()) &&
                ((r==0) || i->at(r-1) < event) &&
                ((r==i->length()-1 || event < i->at(r+1)))
                )
            {
                // changed from multi to not multi?
                if(wasMulti && !isMulti)
                    emit MultiFrameEventDeleted(&((*i)[r]));

                (*i)[r] = event;

                // changed to multi?
                if(isMulti && !wasMulti)
                    emit MultiFrameEventAdded(&((*i)[r]));
            }
            else
            {
                if(wasMulti)
                    emit MultiFrameEventDeleted(&((*i)[r]));

                beginRemoveRows(QModelIndex(),row,row);
                i->remove(r);
                if(i->isEmpty())
                    i = filmEvents->erase(i); // clazy:exclude=strict-iterators
                endRemoveRows();

                AddEvent(event); // AddEvent emits multiAdd signal if needed
            }
            break;
        }
        else
            r -= i->length();
    }
    emit FilmEventsTableUpdated();
}

vbevent VBFilmEventsTableModel::TakeEvent(int row)
{
    vbevent event;

    // distingish the "row" of the frame's event list from the row of
    // the master table (all frames)
    int r = row;

    for (auto i = filmEvents->begin(), end = filmEvents->end();
         i != end; ++i)
    {
        if(r < i->length())
        {
            if((*i)[r].IsMultiFrame())
                emit MultiFrameEventDeleted(&((*i)[r]));

            beginRemoveRows(QModelIndex(),row,row);
            event = i->takeAt(r);
            if(i->isEmpty())
                i = filmEvents->erase(i); // clazy:exclude=strict-iterators
            endRemoveRows();
            break;
        }
        else
            r -= i->length();
    }
    emit FilmEventsTableUpdated();

    return event;
}

void VBFilmEventsTableModel::SetColumns(QStringList colList)
{
    beginResetModel();
    columns = colList;
    endResetModel();

    emit FilmEventsColumnsChanged();
}
