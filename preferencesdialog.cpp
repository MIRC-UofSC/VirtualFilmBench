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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QDebug>
#include <QMessageBox>

preferencesdialog::preferencesdialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::preferencesdialog)
{
	ui->setupUi(this);
    ui->tabWidget->setTabVisible(ui->tabWidget->indexOf(ui->metadataTab),false);
    ui->tabWidget->setCurrentIndex(0);

	settings = new QSettings;

	QStringList l;

	l = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);

	if(l.size()) sysRead = l.at(0);
	else sysRead = "/";

	sysWrite = QStandardPaths::writableLocation(
			QStandardPaths::DocumentsLocation);

	settings->beginGroup("default-folder");
	ui->sourceText->setText(settings->value("source", sysRead).toString());
	ui->projectText->setText(settings->value("project", sysWrite).toString());
	ui->exportText->setText(settings->value("export", sysWrite).toString());
     ui->importText->setText(settings->value("import", sysRead).toString());
	settings->endGroup();

    settings->beginGroup("creator-info");
    ui->creatorIDLineEdit->setText(settings->value("id").toString());
    ui->creatorContextLineEdit->setText(settings->value("context").toString());
    settings->endGroup();

    ui->sourceText->setPlaceholderText(sysRead);
	ui->projectText->setPlaceholderText(sysWrite);
	ui->exportText->setPlaceholderText(sysWrite);

    connect(
        ui->browseForSourceButton, &QPushButton::clicked,
        this, [this](){
            BrowseForFolder(
                ui->sourceText,
                "Default Source Scan Folder",
                sysRead);
        }
        );
    connect(
        ui->browseForProjectButton, &QPushButton::clicked,
        this, [this](){
            BrowseForFolder(
                ui->projectText,
                "Default VFB Project File Folder",
                sysWrite);
        }
        );
    connect(
        ui->browseForExportButton, &QPushButton::clicked,
        this, [this](){
            BrowseForFolder(
                ui->exportText,
                "Default Export Folder",
                sysWrite);
        }
        );
    connect(
        ui->browseForImportButton, &QPushButton::clicked,
        this, [this](){
            BrowseForFolder(
                ui->importText,
                "Default Import Folder",
                sysRead);
        }
        );

    connect(
        ui->sourceText, &QLineEdit::editingFinished,
        this, [this](){ ValidateFolder(sysRead); } );
    connect(
        ui->projectText, &QLineEdit::editingFinished,
        this, [this](){ ValidateFolder(sysWrite); } );
    connect(
        ui->exportText, &QLineEdit::editingFinished,
        this, [this](){ ValidateFolder(sysWrite); } );
    connect(
        ui->importText, &QLineEdit::editingFinished,
        this, [this](){ ValidateFolder(sysRead); } );


    connect(
        ui->discardButton, &QPushButton::clicked,
        this, &preferencesdialog::reject);
    connect(
        ui->saveButton, &QPushButton::clicked,
        this, &preferencesdialog::Save);
}

preferencesdialog::~preferencesdialog()
{
	settings->sync();
	delete settings;

	delete ui;
}

void preferencesdialog::BrowseForFolder(
    QLineEdit *lineEdit, QString title, QString dflt)
{
    QString curdir = lineEdit->text();
    if(curdir.isEmpty()) curdir = dflt;

    QString dir = QFileDialog::getExistingDirectory(this, title, curdir);
    if(dir.isEmpty()) return;

    lineEdit->setText(dir);
}

void preferencesdialog::ValidateFolder(QString dflt)
{
    QLineEdit *w = qobject_cast<QLineEdit *>(sender());
    if(!w) return;

    if(w->text().isEmpty())
    {
        w->setText(dflt);
        return;
    }

    // turn off signals while we handle the message dialogs. This is a
    // workaround for the known QT bug wherein editingFinished is called
    // twice when a messagebox is shown:
    // https://bugreports.qt.io/browse/QTBUG-40
    w->blockSignals(true);

    QString txt = w->text();

    // for preference-file specs, treat all entries as absolute paths
    if(QDir(txt).isRelativePath(txt))
    {
        txt = QDir().rootPath() + txt;
        w->setText(txt);
    }

    if(!QDir(w->text()).exists())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Folder doesn't exist");
        msgBox.setText(QString("The folder '%1' doesn't exist.").arg(txt));
        msgBox.addButton("Create Folder", QMessageBox::AcceptRole);
        msgBox.addButton(QMessageBox::Cancel);
        int ret = msgBox.exec();
        if(ret != QMessageBox::Cancel)
        {
            if(!QDir().mkpath(w->text()))
            {
                QMessageBox::warning(NULL, "Could not create",
                                     "Could not create directory");
                ret = QMessageBox::Cancel;
            }
        }

        if(ret == QMessageBox::Cancel)
        {
            w->setFocus();
            w->selectAll();
        }
    }

    w->blockSignals(false);
}

void preferencesdialog::Save()
{
	settings->beginGroup("default-folder");
	settings->setValue("source",ui->sourceText->text());
	settings->setValue("project",ui->projectText->text());
	settings->setValue("export",ui->exportText->text());
    settings->setValue("import",ui->importText->text());
	settings->endGroup();

    settings->beginGroup("creator-info");
    settings->setValue("id",ui->creatorIDLineEdit->text());
    settings->setValue("context",ui->creatorContextLineEdit->text());
    settings->endGroup();

    settings->beginGroup("audio-metadata");
	settings->setValue("originator", ui->originatorText->text());
	settings->setValue("archive-location", ui->archiveLocationText->text());
	settings->setValue("copyright", ui->copyrightText->text());
	settings->endGroup();

	accept();
	//done(Accepted);
}
