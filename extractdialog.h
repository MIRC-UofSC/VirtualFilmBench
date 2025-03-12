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

#ifndef EXTRACTDIALOG_H
#define EXTRACTDIALOG_H

#include <QDialog>
#include "metadata.h"

namespace Ui {
class ExtractDialog;
}

class ExtractDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ExtractDialog(QWidget *parent, MetaData &metadata,
			const QString &dir);
	~ExtractDialog();

private:
	Ui::ExtractDialog *ui;
	uint16_t version;
	MetaData *meta;
	QString defaultDir;
	bool requestRestart;
	bool videoIsRisky;

public:
	void MarkVideoAsRisky(void);
	bool RequestedRestart(void) {return requestRestart; };

	QString GetFilename(void);
	QString GetVideoFilename(void);
	QString GetOriginator(void);
	QString GetOriginatorReference(void);
	QString GetDescription(void);
	uint16_t GetVersion(void);
	QByteArray GetUMID(void);
	QString GetCodingHistory(void);
	QString GetArchiveLocation(void);
	QString GetSubjectInfo(void);
	QString GetCopyrightInfo(void);

	void SetFilename(const QString &filename);
	void SetOriginator(const QString &originator);
	void SetOriginatorReference(const QString &originatorReference);
	void SetDescription(const QString &description);
	void SetOriginationDateStr(const QString &datestr);
	void SetVersion(uint16_t version);
	void SetTimeReferenceStr(const QString &timecode);
	void SetCodingHistory(const QString &codingHistory);
	void SetArchiveLocation(const QString &archiveLocation);
	void SetSubjectInfo(const QString &subjectInfo);
	void SetCopyrightInfo(const QString &copyrightInfo);

private slots:
	void on_cancelButton_clicked();
	void on_okButton_clicked();
	void on_fileBrowseButton_clicked();
	void on_muxVideoCheckbox_clicked(bool checked);
	void on_fileVideoBrowseButton_clicked();
};

#endif // EXTRACTDIALOG_H
