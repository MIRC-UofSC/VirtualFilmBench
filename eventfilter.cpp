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

#define EVENT_FILTER_CPP
#include "eventfilter.h"

EventFilterCondition::EventFilterCondition(
    QString attr,
    QString pat,
    EventFilterComp comp,
    bool positive
)
{
    this->attribute = attr;
    this->pattern = pat;
    this->comparison = comp;
    this->isNegated = !positive;
}

EventFilterCondition::EventFilterCondition(EventSet set, bool positive)
{
    this->targetSet = set;
    this->isNegated = !positive;
    this->comparison = EVENT_FILTER_COMP_IN_SET;
}

EventFilterCondition::EventFilterCondition(EventFilterComp comp, bool positive)
{
    Q_ASSERT(
        comp >= EVENT_FILTER_COMP_POS_LEFT &&
        comp <= EVENT_FILTER_COMP_POS_BOTTOM);

    this->comparison = comp;
    this->isNegated = !positive;
}

EventFilterComp EventFilterCondition::Comparison() const
{
    EventFilterComp comp = comparison;
    if(isNegated) comp = EventFilterComp(comp ^ 0x01);
    return comp;
}

bool EventFilterCondition::EventPasses(const vbevent &event) const
{
    bool pass(true);
    bool neg = isNegated;

    switch(comparison)
    {
    case EVENT_FILTER_COMP_NOT_IN_SET:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_IN_SET:
        pass = targetSet.contains(event);
        break;

    case EVENT_FILTER_COMP_ATTR_NOT_EQUAL:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_ATTR_EQUAL:
        pass = (event.Attribute(attribute).
               compare(pattern, Qt::CaseInsensitive)==0);
        break;

    case EVENT_FILTER_COMP_ATTR_DOES_NOT_CONTAIN:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_ATTR_CONTAINS:
        pass = event.Attribute(attribute).
               contains(pattern, Qt::CaseInsensitive);
        break;

    case EVENT_FILTER_COMP_ATTR_DOES_NOT_START_WITH:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_ATTR_STARTS_WITH:
        pass = event.Attribute(attribute).
               startsWith(pattern, Qt::CaseInsensitive);
        break;

    case EVENT_FILTER_COMP_ATTR_DOES_NOT_EXIST:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_ATTR_EXISTS:
        pass = !event.Attribute(attribute).isNull();
        break;

    case EVENT_FILTER_COMP_ATTR_NE:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_ATTR_EQ:
    {
        bool ok;
        double targetValue = pattern.toDouble(&ok);
        if(!ok) return false;
        double value = event.Attribute(attribute).toDouble(&ok);
        if(!ok) return false;
        pass = (value == targetValue);
        break;
    }

    case EVENT_FILTER_COMP_ATTR_GE:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_ATTR_LT:
    {
        bool ok;
        double targetValue = pattern.toDouble(&ok);
        if(!ok) return false;
        double value = event.Attribute(attribute).toDouble(&ok);
        if(!ok) return false;
        pass = (value < targetValue);
        break;
    }

    case EVENT_FILTER_COMP_ATTR_LE:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_ATTR_GT:
    {
        bool ok;
        double targetValue = pattern.toDouble(&ok);
        if(!ok) return false;
        double value = event.Attribute(attribute).toDouble(&ok);
        if(!ok) return false;
        pass = (value > targetValue);
        break;
    }

    case EVENT_FILTER_COMP_POS_LEFT:
        pass = event.BoundsX1() < 0.5;
        break;
    case EVENT_FILTER_COMP_POS_RIGHT:
        pass = event.BoundsX0() > 0.5;
        break;
    case EVENT_FILTER_COMP_POS_NOT_SPAN:
        neg = !neg;
        [[fallthrough]];
    case EVENT_FILTER_COMP_POS_SPAN:
        pass = event.BoundsX0() < 0.5 && event.BoundsX1() > 0.5;
        break;
    case EVENT_FILTER_COMP_POS_TOP:
        pass = event.BoundsY1() < 0.5;
        break;
    case EVENT_FILTER_COMP_POS_BOTTOM:
        pass = event.BoundsY0() > 0.5;
        break;

    default:
        pass = false;
        neg = false;
    }

    return(neg ? !pass : pass);
}

//=============================================================================

EventFilter::EventFilter() :
    matchMode(EVENT_FILTER_MATCH_ALL)
{

}

void EventFilter::AddCondition(EventFilterCondition cond)
{
    conditions.append(cond);
}

void EventFilter::AddCondition(
    QString attr,
    QString pattern
    )
{
    conditions.append(EventFilterCondition(attr,pattern));
}

void EventFilter::AddCondition(
    QString attr,
    QString pattern,
    EventFilterComp comp
    )
{
    conditions.append(EventFilterCondition(attr,pattern,comp));
}

void EventFilter::AddCondition(
    QString attr,
    QString pattern,
    EventFilterComp comp,
    bool positive
    )
{
    conditions.append(EventFilterCondition(attr,pattern,comp,positive));
}

void EventFilter::AddCondition(EventSet set)
{
    conditions.append(EventFilterCondition(set));
}

void EventFilter::AddCondition(EventSet set, bool positive)
{
    conditions.append(EventFilterCondition(set,positive));
}

void EventFilter::AddCondition(EventFilterComp posCond)
{
    conditions.append(EventFilterCondition(posCond));
}

void EventFilter::AddCondition(EventFilterComp posCond, bool positive)
{
    conditions.append(EventFilterCondition(posCond, positive));
}

void EventFilter::RemoveCondition(int pos)
{
    if(pos < conditions.size()) conditions.removeAt(pos);
}

bool EventFilter::EventPasses(const vbevent &event) const
{
    switch(matchMode)
    {
    case EVENT_FILTER_MATCH_ALL:
        for (auto condition = conditions.cbegin(), end = conditions.cend();
             condition != end; ++condition)
        {
            if(!(condition->EventPasses(event))) return false;
        }
        return true;
    case EVENT_FILTER_MATCH_ANY:
        for (auto condition = conditions.cbegin(), end = conditions.cend();
             condition != end; ++condition)
        {
            if(condition->EventPasses(event)) return true;
        }
        return false;
    }
}

const QStringList& EventFilter::ConditionNames()
{
    return EventFilterCompName;
}

const QStringList EventFilter::ConditionNamesSimpleSet()
{
    QStringList list=EventFilterCompName;
    list.removeLast();
    list.removeLast();
    return list;
}

const QStringList& EventFilter::MatchingModeNames()
{
    return EventFilterMatchingModeName;
}
