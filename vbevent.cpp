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

#define VBEVENT_CPP
#include "vbevent.h"

#include <QDateTime>

#include <stdexcept>
#include <algorithm>

vbevent::vbevent(uint32_t framenum, EventType type)
{
    id = QUuid::createUuid();
    SetStartAndEnd(framenum);
    SetType(type);
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
    isContinuous = false;
}

vbevent::vbevent(EventID id) : vbevent()
{
    if(!id.isNull())
        this->id = id;
}

vbevent::~vbevent()
{

}

EventBoundsSortType vbevent::BoundsSortType() const
{
    if(!this->HasBounds()) return VB_EVENT_BOUNDS_SORT_FULL;

    if(this->BoundsX0() < 0.5)
    {
        if(this->BoundsX1() >= 0.5) return VB_EVENT_BOUNDS_SORT_SPAN;
        else return VB_EVENT_BOUNDS_SORT_LEFT;
    }
    return VB_EVENT_BOUNDS_SORT_RIGHT;
}

bool vbevent::operator==(const vbevent &other) const
{
    // equal if everything other than id and timstamps are equal

    if(eventType != other.eventType) return false;
    if(eventTypeOtherName.compare(other.eventTypeOtherName,Qt::CaseInsensitive)!=0)
        return false;
    if(frameStart != other.frameStart) return false;
    if(frameEnd != other.frameEnd) return false;
    if(bounds[0] != other.bounds[0]) return false;
    if(bounds[1] != other.bounds[1]) return false;
    if(bounds[2] != other.bounds[2]) return false;
    if(bounds[3] != other.bounds[3]) return false;
    if(IsContinuous() != other.IsContinuous()) return false;

    // check in both directions in case one of the lists has the same
    // element twice; to ensure no false positives

    // check this's attributes against that
    for(auto &attr : attributes)
    {
        if(attr.first.compare("DateCreated")==0) continue;
        if(attr.first.compare("DateModified")==0) continue;

        if(attr.second.compare(other.Attribute(attr.first), Qt::CaseInsensitive)!=0)
            return false;
    }

    // check that's attributes against this
    for(auto &attr : other.attributes)
    {
        if(attr.first.compare("DateCreated")==0) continue;
        if(attr.first.compare("DateModified")==0) continue;

        if(attr.second.compare(Attribute(attr.first), Qt::CaseInsensitive)!=0)
            return false;
    }
    return true;
}

bool vbevent::operator<(const vbevent &other) const
{
    if(this->frameStart < other.frameStart) return true;
    if(this->frameStart > other.frameStart) return false;

    if(this->BoundsSortType() < other.BoundsSortType()) return true;
    if(this->BoundsSortType() > other.BoundsSortType()) return false;

    // same class, the one with the higher top (smaller Y) wins.
    return(this->BoundsY0() < other.BoundsY0());
}

const QStringList vbevent::coreEventTypeNames = EventTypeName;

QString vbevent::TypeName() const
{
    switch(eventType)
    {
    case VB_EVENT_OTHER:
        return eventTypeOtherName;
    default:
        return EventTypeName[eventType];
    }
}

void vbevent::SetType(EventType type)
{
    if(eventType == type) return;

    eventType = type;

    if(type != VB_EVENT_OTHER)
        eventTypeOtherName=QString();
    else
        eventTypeOtherName=QString("Other");
}

void vbevent::SetType(const QString eventTypeName)
{
    int i;

    for(i=0; i<EventTypeName.length(); ++i)
    {
        if(eventTypeName.compare(EventTypeName[i],Qt::CaseInsensitive) == 0 )
        {
            SetType(EventType(i));
            return;
        }
    }

    eventType = VB_EVENT_OTHER;
    eventTypeOtherName = eventTypeName;
}

void vbevent::SetEnd(uint32_t e)
{
    if(e==0)
        frameEnd = frameStart;
    else
    {
        frameEnd=e;
        if(frameStart > e)
            frameStart = e;
    }
}

void vbevent::SetBoundsX0X1Y0Y1(float x0, float x1, float y0, float y1)
{
    if(x0>x1)
    {
        bounds[0] = x1;
        bounds[1] = x0;
    }
    else
    {
        bounds[0] = x0;
        bounds[1] = x1;
    }

    if(y0>y1)
    {
        bounds[2] = y1;
        bounds[3] = y0;
    }
    else
    {
        bounds[2] = y0;
        bounds[3] = y1;
    }
}


void vbevent::SetBoundsCenterAndSize(float cx, float cy, float w, float h)
{
    bounds[0] = cx-(0.5f*w);
    bounds[1] = cx+(0.5f*w);
    bounds[2] = cy-(0.5f*h);
    bounds[3] = cy+(0.5f*h);
}

bool vbevent::HasBounds() const
{
    return bounds[0] || bounds[1] || bounds[2] || bounds[3];
}

bool vbevent::IsContinuous() const
{
    return isContinuous && (frameStart < frameEnd);
}

bool vbevent::SetContinuous(bool c)
{
    // allow continuous to be set before or after setting frameEnd
    // so as not to allow stealthy bugs to creep in. The setting will
    // be forgotten for single-frame events on event export and import,
    // since the IsContinuous() method would be used as the bridge.
    isContinuous = c;
    return IsContinuous();
}

float vbevent::EffectiveConfidence() const
{
    QString confStr = Attribute("Confidence");
    if(confStr.isEmpty())
        return 1.0f;
    else
    {
        bool ok;
        float conf = confStr.toFloat(&ok);
        if(!ok) return 1.0f;
        else return conf;
    }
}

// MakeAttributeName
// =================
// static function to make a name conform to the conventions
// Virtual Bench uses for Attributes: CamelCase.
//
// The given string wil have spaces and underscores removed, with the
// character following the removed space/underscore mapped to uppercase.
// The initial character will also be mapped to uppercase.
//
// two_words -> TwoWords
// twowords -> Twowords
// The quick brown fox -> TheQuickBrownFox
// user_id -> UserId
// User ID -> UserID
QString vbevent::MakeAttributeName(QString str)
{
    QStringList words = str.replace("_"," ").simplified().split(' ');
    QString name;
    for(QString word : words)
        name += word.replace(0, 1, word[0].toUpper());
    return name;
}

void vbevent::SetAttribute(const QString attribute, const QString value)
{
    QString attr = MakeAttributeName(attribute);

    if((attr.compare("Start",Qt::CaseInsensitive)==0) ||
        (attr.compare("Frame",Qt::CaseInsensitive)==0))
    {
        bool ok;
        int val = value.toInt(&ok);
        if(ok) SetStart(val);
        return;
    }

    if(attr.compare("End",Qt::CaseInsensitive)==0)
    {
        bool ok;
        int val = value.toInt(&ok);
        if(ok) SetEnd(val);
        return;
    }

    if((attr.compare("EventType",Qt::CaseInsensitive)==0) ||
        (attr.compare("Type",Qt::CaseInsensitive)==0))
    {
        SetType(value);
        return;
    }

    if(attr.compare("SubType",Qt::CaseInsensitive)==0)
    {
        SetSubType(value);
        return;
    }

    if(attr.compare("Notes",Qt::CaseInsensitive)==0)
    {
        notes = value;
        return;
    }

    for(auto i = attributes.begin(), end = attributes.end(); i != end; ++i)
    {
        if(i->first.compare(attr, Qt::CaseInsensitive)==0)
        {
            i->second = value;
            return;
        }
    }

    attributes.append(EventAttributePair(attr,value));
}

QString vbevent::Attribute(const QString attribute) const
{
    QString attr = MakeAttributeName(attribute);
    for(auto i = attributes.cbegin(), end = attributes.cend(); i != end; ++i)
    {
        if(i->first.compare(attr, Qt::CaseInsensitive)==0)
            return i->second;
    }
    if((attr.compare("Start",Qt::CaseInsensitive)==0) ||
        (attr.compare("Frame",Qt::CaseInsensitive)==0))
        return QVariant(this->Start()).toString();
    if(attr.compare("End",Qt::CaseInsensitive)==0)
        return QVariant(this->End()).toString();
    if((attr.compare("EventType",Qt::CaseInsensitive)==0) ||
        (attr.compare("Type",Qt::CaseInsensitive)==0))
        return this->TypeName();
    if(attr.compare("SubType",Qt::CaseInsensitive)==0)
        return this->SubType();
    if(attr.compare("Notes",Qt::CaseInsensitive)==0)
        return this->notes;

    return QString();
}

QString vbevent::SubTypeName() const
{
    QString subtag = MakeAttributeName(TypeName());
    subtag.append("Type");
    return subtag;
}

void vbevent::SetSubType(QString subType)
{
    SetAttribute(SubTypeName(),subType);
}

QString vbevent::SubType() const
{
    return Attribute(SubTypeName());
}
