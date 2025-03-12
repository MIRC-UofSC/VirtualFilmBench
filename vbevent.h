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

#ifndef VBEVENT_H
#define VBEVENT_H

#include <QString>
#include <QList>
#include <QMap>
#include <QAbstractTableModel>
#include <QUuid>

#include <string>
#include <algorithm>

enum EventType {
    VB_EVENT_GENERIC,
    VB_EVENT_FILMSTATE,
    VB_EVENT_DAMAGE,
    VB_EVENT_JOIN,
    VB_EVENT_ARTIFACT,
    VB_EVENT_EDGEMARK,
    VB_EVENT_OTHER };
#ifdef VBEVENT_CPP
static const QStringList EventTypeName = {
    "--",
    "FilmState",
    "Damage",
    "Splice",
    "Artifact",
    "EdgeMark",
    "Other"};
#endif

enum EventBoundsSortType {
    VB_EVENT_BOUNDS_SORT_FULL,
    VB_EVENT_BOUNDS_SORT_SPAN,
    VB_EVENT_BOUNDS_SORT_LEFT,
    VB_EVENT_BOUNDS_SORT_RIGHT
};

typedef QPair< QString, QString > EventAttributePair;
typedef QList< EventAttributePair  > EventAttributeList;
typedef QUuid EventID;
typedef QSet< EventID > EventSet;

class vbevent
{
public:
    vbevent(uint32_t framenum=0, EventType eventType=VB_EVENT_GENERIC);
    vbevent(EventID id);
    ~vbevent();

    QString notes;

    static const QStringList coreEventTypeNames;

    EventBoundsSortType BoundsSortType() const;
    bool operator==(const vbevent &other) const;
    bool operator<(const vbevent &other) const;

private:
    EventID id;
    EventType eventType;
    QString eventTypeOtherName;
    uint32_t frameStart; // zero-indexed frame where the event occurs/begins
    uint32_t frameEnd; // zero-indexed frame number of end
    float  bounds[4]; // x0, x1, y0, y1
    bool isContinuous; // multi-frame: continuous extend? (vs. discrete repeat)
    EventAttributeList attributes;

public:
    EventID ID() const { return id; }
    QString TypeName() const;
    inline EventType Type() const { return eventType; }
    void SetType(EventType eventType);
    void SetType(const QString eventTypeName);
    inline uint32_t Start() const { return frameStart; }
    inline uint32_t End() const { return frameEnd; }
    inline void SetStart(uint32_t s) {
        frameStart=s; if(frameEnd < s) frameEnd = s; }
    void SetEnd(uint32_t e); // use e=0 to set end equal to start
    inline void SetStartAndEnd (uint32_t s, uint32_t e) {
        frameStart = s; frameEnd = std::max(s,e); }

    inline void SetStartAndEnd (uint32_t f) { frameStart = frameEnd = f; }
    inline bool IsMultiFrame() const { return (frameEnd-frameStart >= 2); }

    inline const float *Bounds() const { return bounds; }
    inline float BoundsX0() const { return bounds[0]; }
    inline float BoundsX1() const { return bounds[1]; }
    inline float BoundsY0() const { return bounds[2]; }
    inline float BoundsY1() const { return bounds[3]; }
    void SetBoundsX0X1Y0Y1(float x0, float x1, float y0, float y1);
    inline float BoundsCenterX() const { return (bounds[0]+bounds[1])/2.0f; }
    inline float BoundsCenterY() const { return (bounds[2]+bounds[3])/2.0f; }
    inline float BoundsSizeX() const { return bounds[1]-bounds[0]; }
    inline float BoundsSizeY() const { return bounds[3]-bounds[2]; }
    void SetBoundsCenterAndSize(float cx, float cy, float w, float h);
    bool HasBounds() const;
    bool IsContinuous() const;
    bool SetContinuous(bool c);
    float EffectiveConfidence() const;

    static QString MakeAttributeName(QString str);

    inline const EventAttributeList &Attributes() const { return attributes; }
    void SetAttribute(const QString attribute, const QString value);
    QString Attribute(const QString attribute) const;

    QString SubTypeName() const;
    void SetSubType(QString subType);
    QString SubType() const;

    bool InSet(const EventSet s) const { return s.contains(this->id); }
    operator EventID() const { return id; }
};


#endif // VBEVENT_H
