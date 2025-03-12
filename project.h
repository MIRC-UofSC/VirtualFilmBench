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

#ifndef PROJECT_H
#define PROJECT_H

#include <iostream>
#include <string>
#include <vector>

#include <QTextStream>

#include "FilmScan.h"
#include "overlap.h"

class FrameRegion {
private:
	unsigned int left;
	unsigned int right;
public:
	FrameRegion() {};
	FrameRegion(int l, int r) : left(l), right(r) {} ;

	int Left() const { return left; }
	int Right() const { return right; }
	int Width() const { return right - left + 1; }
};

typedef std::vector< double > LampMask;

class Project {
public:
	std::string filename; // file name of this project file

	FilmScan inFile;
	unsigned int firstFrameIndex; // = 0
	unsigned int lastFrameIndex; // = 0

	std::vector<FrameRegion> soundBounds;

	unsigned int overlapThreshold; // default = 20

public:
	Project();
	Project(std::string filename);
	~Project();

	void Initialize();

	bool SourceScan(std::string filename, SourceFormat fmt=SOURCE_UNKNOWN);

};


#endif
