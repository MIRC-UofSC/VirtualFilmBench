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

#include "propertiesdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QScrollArea>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "FilmScan.h"

PropertiesDialog::PropertiesDialog(
    QWidget *parent, PropertyList properties, bool isNew) :
    QDialog(parent)
{
    if(isNew)
        this->setWindowTitle("New Project");
    else
        this->setWindowTitle("Project Properties");

    QVBoxLayout *layout = new QVBoxLayout(this);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    QWidget *scrollContents = new QWidget;
    form = new QFormLayout(scrollContents);
    for(auto &prop: properties.List())
    {
        QLabel *label = new QLabel(prop.Name());
        label->setObjectName(prop.Name());
        if(prop.Type() == PROPERTY_TYPE_BOOL &&
            (prop.Values().isEmpty() ||
                (prop.Values()[0] == "false" &&
                prop.Values()[1] == "true")
                )
            )
        {
            QCheckBox *checkBox = new QCheckBox;
            checkBox->setChecked(prop.Value() == "true");
            checkBox->setObjectName(prop.Name()+"Value");
            form->addRow(label, checkBox);
        }
        else if(prop.Values().isEmpty())
        {
            QLineEdit *lineEdit = new QLineEdit;
            lineEdit->setObjectName(prop.Name()+"Value");
            lineEdit->setText(prop.Value());
            QFontMetrics fm = lineEdit->fontMetrics();
            int w = fm.averageCharWidth();
            lineEdit->setMinimumWidth(w*40);
            QSizePolicy policy = lineEdit->sizePolicy();
            policy.setHorizontalPolicy(QSizePolicy::Expanding);
            lineEdit->setSizePolicy(policy);

            if(prop.Type() == PROPERTY_SYSTEM_DATE)
            {
                lineEdit->setReadOnly(true);
                lineEdit->setStyleSheet("background: #DDDDDD;");
            }
            else if(prop.IsMandatory())
            {
                lineEdit->setToolTip("This field is mandatory");
                mandatoryFields.append(lineEdit);
                connect(
                    lineEdit, SIGNAL(textChanged(QString)),
                    this, SLOT(CheckTextNonEmpty()));
                CheckTextNonEmpty(lineEdit);
            }
            if(prop.Name() == "FileURL")
            {
                if(isNew)
                {
                    label->setText("Video File");
                    QHBoxLayout *hbox = new QHBoxLayout();
                    hbox->addWidget(lineEdit,1);
                    QPushButton *browse = new QPushButton("Browse");
                    browse->setObjectName(prop.Value()+"Button");
                    hbox->addWidget(browse);
                    connect(
                        browse, &QPushButton::clicked,
                        this, [this,lineEdit](){BrowseForFile(lineEdit);});
                    connect(
                        lineEdit, &QLineEdit::textChanged,
                        this, &PropertiesDialog::CopyFilename);
                    form->addRow(label, hbox);
                }
                else
                {
                    lineEdit->setReadOnly(true);
                    lineEdit->setStyleSheet("background: #DDDDDD;");
                    form->addRow(label, lineEdit);
                }
            }
            else
                form->addRow(label, lineEdit);
        }
        else
        {
            QComboBox *comboBox = new QComboBox;
            comboBox->setObjectName(prop.Name()+"Value");
            comboBox->addItems(prop.Values());
            comboBox->setEditable(false);
            comboBox->setCurrentText(prop.Value());
            form->addRow(label, comboBox);
        }
    }
    scrollArea->setWidget(scrollContents);
    layout->addWidget(scrollArea, 1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox);

    origProperties = properties;
}

PropertiesDialog::~PropertiesDialog()
{

}

PropertyList PropertiesDialog::Properties() const
{
    PropertyList ret = origProperties;
    QLabel *label;
    QString value;
    QWidget *w;
    QLayout *l;
    QLineEdit *lineEdit;
    QComboBox *comboBox;

    for(int r=0; r<form->rowCount(); ++r)
    {
        w = form->itemAt(r, QFormLayout::LabelRole)->widget();
        if(!w) continue;
        label = qobject_cast<QLabel *>(w);
        if(!label) continue;

        w = form->itemAt(r, QFormLayout::FieldRole)->widget();
        if(!w)
        {
            l = form->itemAt(r, QFormLayout::FieldRole)->layout();
            if(l)
                w = l->itemAt(0)->widget();
        }
        if(!w) continue;

        lineEdit = qobject_cast<QLineEdit *>(w);
        if(lineEdit) value = lineEdit->text();
        else
        {
            comboBox = qobject_cast<QComboBox *>(w);
            if(comboBox) value = comboBox->currentText();
            else continue;
        }

        QString name = label->objectName();
        ret.SetValue(name, value);
    }
    return ret;
}

void PropertiesDialog::CheckTextNonEmpty()
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender());
    if(lineEdit) CheckTextNonEmpty(lineEdit);
}

void PropertiesDialog::CheckTextNonEmpty(QLineEdit *lineEdit)
{
    if(lineEdit->text().isEmpty())
        lineEdit->setStyleSheet("background: #FFAAAA;");
    else
        lineEdit->setStyleSheet("");
}

void PropertiesDialog::accept()
{
    for(auto textEdit: mandatoryFields)
    {
        if(textEdit->text().isEmpty())
        {
            QMessageBox::warning(
                this, tr("Missing Mandatory Field"),
                tr("Please enter values for all mandatory fields."));
            return;
        }
    }
    QDialog::accept();
}

void PropertiesDialog::BrowseForFile(QLineEdit *lineEdit)
{
    // Ask for input source

    static QString prevDir = "";
    QString srcDir;

    const struct {
        SourceFormat fileType;
        const char *filter;
    } fileFilterArr[] = {
        { SOURCE_LIBAV,     "Video files (*.mp4 *.mov *.avi)" },
        { SOURCE_TIFF,      "TIFF frames (*.tif *.tiff)" },
        { SOURCE_DPX,       "DPX frames (*.dpx)" },
        { SOURCE_UNKNOWN,   "" }
    };

    QStringList fileTypeFilters;
    for(int ft = 0; fileFilterArr[ft].fileType != SOURCE_UNKNOWN; ++ft)
        fileTypeFilters += tr(fileFilterArr[ft].filter);

    if(prevDir.isEmpty())
    {
        QSettings settings;
        settings.beginGroup("default-folder");
        srcDir = settings.value("source").toString();
        if(srcDir.isEmpty())
        {
            QStringList l;
            l = QStandardPaths::standardLocations(
                QStandardPaths::DocumentsLocation);
            if(l.size()) srcDir = l.at(0);
            else srcDir = "/";
        }
        settings.endGroup();
    }
    else
    {
        srcDir = prevDir;
    }

    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("Film Source"), srcDir,
        fileTypeFilters.join(";;"));

    if(filename.isEmpty()) return;

    prevDir = QFileInfo(filename).absolutePath();
    lineEdit->setText(filename);
}

// copy the filename portion of the fileURL to the InputID field,
// if the InputID field is empty
void PropertiesDialog::CopyFilename(const QString &fileURL)
{
    QLineEdit *fileLineEdit;

    fileLineEdit = this->findChild<QLineEdit *>("InputIDValue");

    if(!fileLineEdit) return;

    if(fileLineEdit->text().isEmpty())
        fileLineEdit->setText(QUrl(fileURL).fileName());
}
