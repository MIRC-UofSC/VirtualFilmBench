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

#include "propertylist.h"

#include <QtLogging>
#include <QDebug>

Property::Property(
    QString n,
    QString v,
    PropertyType t,
    QStringList vList)
    :
    name(n),
    propType(t),
    values(vList),
    isMandatory(false)
{
    if(!v.isEmpty()) SetValue(v);
}

void Property::SetValue(const QString& v)
{
    if(propType == PROPERTY_TYPE_BOOL)
    {
        if(values.isEmpty())
        {
            if(v == "true" || v == "false")
                values << "false" << "true";
            else if(v == "yes" || v == "no")
                values << "no" << "yes";
            else
                qFatal() << "Incomplete bool type " << v;
        }
        else
        {
            if(!values.contains(v))
                qFatal() << "Incompatible bool value given: " << v;
        }
    }
    else if(!values.isEmpty() && !values.contains(v))
        values.prepend(v);

    value = v;
}

//============================================================================

PropertyList::PropertyList()
{

}

void PropertyList::Add(Property prop)
{
    for(auto &p : propList)
        if(p.Name() == prop.Name())
            qFatal() << "Property added twice: " << prop.Name();

    propList << prop;
}
void PropertyList::Add(QString name, QString value)
{
    this->Add(Property(name, value));
}

QString PropertyList::Value(QString name) const
{
    for(auto &p : propList)
        if(p.Name() == name) return p.Value();

    return QString();
}

void PropertyList::SetValue(QString name, QString value)
{
    for(auto &p : propList)
        if(p.Name() == name) return p.SetValue(value);

    Add(name, value);
}

void PropertyList::SetMandatory(QString name, bool f)
{
    for(auto &p : propList)
        if(p.Name() == name) return p.SetMandatory(f);

    qFatal() << "SetMandatory references non-existent property " << name;
}
