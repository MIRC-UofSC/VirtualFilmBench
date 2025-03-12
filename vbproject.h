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

#ifndef VBPROJECT_H
#define VBPROJECT_H

#include <QObject>
#include <vbevent.h>

#include "propertylist.h"

typedef QList<vbevent> VBFrameEvents; // list of all the events that occur (or start) on the same frame as each other
typedef QMap<uint32_t, VBFrameEvents> VBFilmEvents;

typedef QMap<QString, QStringList> VBFilmEventAttributeValues;

class VBFilmEventsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    VBFilmEventsTableModel(
        QObject *parent=nullptr,
        VBFilmEvents *filmEvents = nullptr,
        QList<vbevent> *trash = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    const vbevent *EventAtRow(int row) const;
    int RowOfEvent(const vbevent *event) const;
    int RowAtFrame(uint32_t frame) const;
    void Clear();
    void AddEvent(const vbevent &event);
    void BeginBatchAddEvent();
    void EndBatchAddEvent();
    void UpdateEventAtRow(int row, const vbevent &event);
    vbevent TakeEvent(int row);
    void DeleteEvent(int row) { (void)TakeEvent(row); }

    void SetConfidenceThreshold(float t) { confidenceThreshold = t; }
    QStringList Columns() const { return columns; }
    void SetColumns(QStringList colList);

    const VBFilmEvents *FilmEvents() const { return filmEvents; }

signals:
    void FilmEventsTableUpdated();
    void FilmEventsColumnsChanged();
    void MultiFrameEventAdded(vbevent *);
    void MultiFrameEventDeleted(vbevent *);
    void MultiFrameEventsCleared();

private:
    VBFilmEvents *filmEvents;
    QList<vbevent> *trash;
    float confidenceThreshold;
    QStringList columns;
    bool inBatchAddEventMode;
};

class vbproject : public QObject
{
    Q_OBJECT
public:
    vbproject();

    const PropertyList Properties() const { return properties; }
    void SetProperties(PropertyList l);
    QString FileURL() const { return properties.Value("FileURL"); }
    void SetFileURL(QString s) { properties.SetValue("FileURL",s); }
    QString InputID() const { return properties.Value("InputID"); }
    void SetInputID(QString s) { properties.SetValue("InputID",s); }
    QString FilmGauge() const { return properties.Value("Gauge"); }
    int FramesPerFoot() const;
    void SetFilmGauge(QString s) { properties.SetValue("Gauge", s); }

    int zeroframe;
    float overlap_framestart;
    float overlap_frameend;

    VBFilmEvents::size_type NumFilmEvents();

    QString Title() const { return properties.Value("Title"); }
    void SetTitle(QString s) { properties.SetValue("Title", s); }
    QString RollID() const { return properties.Value("ReelID"); }
    void SetRollID(QString s) { properties.SetValue("ReelID", s); }
    QString FilmAssetID() const { return properties.Value("FilmAssetID"); }
    void SetFilmAssetID(QString s) { properties.SetValue("FilmAssetID", s); }
    QString Notes() const { return properties.Value("Notes"); }
    void SetNotes(QString s) { properties.SetValue("Notes", s); }
    QString CreationDate() const { return properties.Value("CreationDate"); }
    void SetCreationDate(QString s) { properties.SetValue("CreationDate", s); }
    QString LastModifiedDate() const
        { return properties.Value("ModificationDate"); }
    void SetLastModificationDate(QString s)
        { properties.SetValue("ModificationDate", s); }

    bool save(QString filename);
    bool load(QString filename);
    EventSet ImportEvents(const QString filename);
    void ExportEvents(
        const QString filename,
        std::function<bool(int)> include = [](int a){(void)a; return true;}
        ) const;

    inline const VBFilmEvents &FilmEvents() const { return filmEvents; }
    inline VBFilmEvents &FilmEvents() { return filmEvents; }
    QList<vbevent *> FilmEventsForFrame(uint32_t frame);
    VBFilmEventsTableModel *FilmEventsTableModel();

    QString FilmNotes() const { return filmNotes; }
    void SetFilmNotes(QString s) { filmNotes = s; }

    float ConfidenceThreshold() const { return confidenceThreshold; }
    bool ConfidenceThresholdIsEnabled() const
        { return confidenceThresholdIsEnabled; }
    int NumEventsThresholded() const { return lowConfidenceEvents.size(); }

    void SetConfidenceThreshold(float threshold, bool enabled=true);
    void EnableConfidenceThreshold(bool enable);

    QStringList AttributesInUse() const;

public slots:
    void UpdateFrameEventBoundingBox(vbevent *event,
                                     float x0, float x1, float y0, float y1);
    void UpdateFrameEventBoundingBox(int frame, int eventNum,
                                     float x0, float x1, float y0, float y1);
    void MultiFrameEventAdd(vbevent *e);
    void MultiFrameEventDelete(vbevent *e);
    void MultiFrameEventDeleteAll();

private:
    PropertyList properties;
    VBFilmEvents filmEvents;
    VBFilmEventsTableModel *filmEventsTableModel;
    QList<vbevent*> multiFrameEvents;

    QString filmNotes;

    float confidenceThreshold;
    bool confidenceThresholdIsEnabled;
    QList<vbevent> lowConfidenceEvents;

    inline static const QStringList reservedAttributes = {
        "EventType",
        "SubType",
        "CreatorID",
        "CreatorContext",
        "Start",
        "End",
        "BoundingBoxW",
        "BoundingBoxH",
        "BoundingBoxX",
        "BoundingBoxY",
        "BoundingBoxX0",
        "BoundingBoxX1",
        "BoundingBoxY0",
        "BoundingBoxY1",
        "IsContinuous",
        "Confidence",
        "Notes",
        "Details",
        "DateCreated",
        "DateModified"
    };

    VBFilmEventAttributeValues defaultAttributeValues;

public:
    inline static bool IsReservedAttribute(const QString &s)
    {
        return reservedAttributes.contains(s, Qt::CaseInsensitive);
    }

    inline QStringList DefaultAttributes() const
    {
        return defaultAttributeValues.keys();
    }

    QStringList DefaultAttributeValues(QString attr) const;

    void LoadDefaultAttributeValues(QString filename);
};

#endif // VBPROJECT_H
