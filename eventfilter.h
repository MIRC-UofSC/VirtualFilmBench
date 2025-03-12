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

#ifndef EVENTFILTER_H
#define EVENTFILTER_H

#include "vbevent.h"

enum EventFilterComp : int
{
    // IMPORTANT: each filter must be paired with its
    // negation in such a way that their int values
    // are the same except for the LSB being flipped.
    EVENT_FILTER_COMP_ATTR_EQUAL,
    EVENT_FILTER_COMP_ATTR_NOT_EQUAL,
    EVENT_FILTER_COMP_ATTR_CONTAINS,
    EVENT_FILTER_COMP_ATTR_DOES_NOT_CONTAIN,
    EVENT_FILTER_COMP_ATTR_STARTS_WITH,
    EVENT_FILTER_COMP_ATTR_DOES_NOT_START_WITH,
    EVENT_FILTER_COMP_ATTR_EXISTS,
    EVENT_FILTER_COMP_ATTR_DOES_NOT_EXIST,

    // compare numerically. Non-numerics always fail test
    EVENT_FILTER_COMP_ATTR_EQ,
    EVENT_FILTER_COMP_ATTR_NE,
    EVENT_FILTER_COMP_ATTR_LT,
    EVENT_FILTER_COMP_ATTR_GE,
    EVENT_FILTER_COMP_ATTR_GT,
    EVENT_FILTER_COMP_ATTR_LE,

    // position of boundingbox
    EVENT_FILTER_COMP_POS_LEFT,
    EVENT_FILTER_COMP_POS_RIGHT,
    EVENT_FILTER_COMP_POS_SPAN,
    EVENT_FILTER_COMP_POS_NOT_SPAN,
    EVENT_FILTER_COMP_POS_TOP,
    EVENT_FILTER_COMP_POS_BOTTOM,

    // set comparison
    EVENT_FILTER_COMP_IN_SET,
    EVENT_FILTER_COMP_NOT_IN_SET
};

inline bool EventFilterCompIsAttr(EventFilterComp c)
    { return (c <= EVENT_FILTER_COMP_ATTR_LE); }
inline bool EventFilterCompIsNumeric(EventFilterComp c)
{
    return (
        c >= EVENT_FILTER_COMP_ATTR_EQ &&
        c <= EVENT_FILTER_COMP_ATTR_LE);
}
inline bool EventFilterCompIsPos(EventFilterComp c)
{
    return (
        c >= EVENT_FILTER_COMP_POS_LEFT &&
        c <= EVENT_FILTER_COMP_POS_BOTTOM);
}
inline bool EventFilterCompIsSet(EventFilterComp c)
{
    return (
        c >= EVENT_FILTER_COMP_IN_SET &&
        c <= EVENT_FILTER_COMP_NOT_IN_SET);
}

enum EventFilterMatchingMode
{
    EVENT_FILTER_MATCH_ANY,
    EVENT_FILTER_MATCH_ALL
};

#ifdef EVENT_FILTER_CPP
static const QStringList EventFilterCompName = {
    "Is",
    "Is not",
    "Contains",
    "Does not contain",
    "Starts with",
    "Does not start with",
    "Is set (any value)",
    "Is not set",
    "=",
    "\u2260",
    "<",
    "\u2265",
    ">",
    "\u2264",
    "Left side",
    "Right side",
    "Spans",
    "Does not span",
    "Top",
    "Bottom",
    "In",
    "Not in"
};

static const QStringList EventFilterMatchingModeName = {
    "Match Any",
    "Match All"
};
#endif

class EventFilterCondition
{

private:
    EventFilterComp comparison;
    bool isNegated;
    QString attribute;
    QString pattern;
    EventSet targetSet;

public:
    EventFilterCondition(
        QString attr,
        QString pattern,
        EventFilterComp comp = EVENT_FILTER_COMP_ATTR_EQUAL,
        bool positive=true
    );
    EventFilterCondition(EventSet set, bool positive=true);
    EventFilterCondition(EventFilterComp locationComp, bool positive=true);

    EventFilterComp Comparison() const;
    QString Attribute() const { return attribute; }
    QString Pattern() const { return pattern; }

    void SetCondition(
        QString attr,
        QString pattern,
        EventFilterComp comp = EVENT_FILTER_COMP_ATTR_EQUAL,
        bool positive=true
    );
    void SetCondition(EventSet set, bool positive=true);
    void SetCondition(EventFilterComp locationComp, bool positive=true);

    bool IsAttr() { return EventFilterCompIsAttr(comparison); }
    bool IsNumeric() { return EventFilterCompIsNumeric(comparison); }
    bool IsPos() { return EventFilterCompIsPos(comparison); }
    bool IsSet() { return EventFilterCompIsSet(comparison); }

    bool EventPasses(const vbevent &event) const;
};

class EventFilter
{

private:
    QList< EventFilterCondition > conditions;
    EventFilterMatchingMode matchMode;

public:
    EventFilter();

    const QList<EventFilterCondition> &Conditions() const { return conditions; }
    void AddCondition(EventFilterCondition cond);
    void AddCondition(
        QString attr,
        QString pattern
    );
    void AddCondition(
        QString attr,
        QString pattern,
        EventFilterComp comp
    );
    void AddCondition(
        QString attr,
        QString pattern,
        EventFilterComp comp,
        bool positive
    );
    void AddCondition(EventSet set);
    void AddCondition(EventSet set, bool positive);
    void AddCondition(EventFilterComp posCond);
    void AddCondition(EventFilterComp posCond, bool positive);
    void SetMatchMode(EventFilterMatchingMode mode) {matchMode = mode;}
    EventFilterMatchingMode MatchMode() const { return matchMode; }
    void RemoveCondition(int pos);
    EventFilterCondition Condition(int pos) const;
    void ReplaceCondition(int pos, EventFilterCondition cond);
    bool EventPasses(const vbevent &event) const;

    static const QStringList& ConditionNames();
    static const QStringList ConditionNamesSimpleSet();
    static const QStringList& MatchingModeNames();
};

#endif // EVENTFILTER_H
