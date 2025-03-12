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

#ifndef FILMGAUGE_H
#define FILMGAUGE_H

#include <QMap>
#include <QString>

class FilmGauge
{
public:
    FilmGauge(QString g="35mm");

    inline static const QStringList gaugeList{
        "35mm",
        "16mm",
        "8mm",
        "S8mm",
        "9.5mm",
        "17.5mm",
        "22mm",
        "28mm"
    };

    inline static const QMap<QString, int> gaugeFPF{
        {"35mm",16}, // 4-perf
        {"16mm",40},
        {"8mm",80},
        {"s8mm",72 }
    };

private:
    QString gauge;

public:
    int FramesPerFoot() const { return gaugeFPF.value(gauge.toLower(),80); }
    operator QString() const { return gauge; }
    void operator=(const QString s) { gauge = s; }
};

#endif // FILMGAUGE_H
