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

#ifndef PROPERTYLIST_H
#define PROPERTYLIST_H

#include <QMap>
#include <QString>
#include <QStringList>

enum PropertyType {
    PROPERTY_TYPE_TEXT,
    PROPERTY_TYPE_BOOL,
    PROPERTY_SYSTEM_DATE
};

class Property
{
public:
    Property(
        QString name = QString(),
        QString value = QString(),
        PropertyType t = PROPERTY_TYPE_TEXT,
        QStringList valueList = QStringList());

private:
    QString name;
    QString value;
    PropertyType propType;
    QStringList values;
    bool isMandatory;

public:
    const QString& Name() const { return name; }
    PropertyType Type() const { return propType; }
    const QString& Value() const { return value; }
    void SetValue(const QString& v);
    const QStringList& Values() const { return values; }
    void SetMandatory(bool f=true) { isMandatory = f; }
    bool IsMandatory() const { return isMandatory; }
};

class PropertyList
{
public:
    PropertyList();

private:
    QList<Property> propList;

public:
    const QList<Property>& List() const { return propList; }
    void Add(Property p);
    void Add(QString name, QString value=QString());
    QString Value(QString name) const;
    void SetValue(QString name, QString value);
    void SetMandatory(QString name, bool f=true);
};

#endif // PROPERTYLIST_H
