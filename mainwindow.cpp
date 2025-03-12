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

// PREREQUISITES:
//
// place libraries and include files in system locations like /usr/local/lib
// on unix and osx, or under C:\include and C:\lib under windows, or put
// them or link them to the Virtual Film Bench source directory tree either
// directly or under subdirectories called "release" and "debug"
//
// Or add the appropriate paths to the platform-specific sections of this
// project file
//
// Additional libraries required:
// libav (the ffmpeg libraries)
// dpx
// dspfilters

//-----------------------------------------------------------------------------

#include <algorithm>

#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QtCompilerDetection>
#include <QProcess>
#include <QDebug>
#include <QDialogButtonBox>
#include <QSettings>
#include <QVersionNumber>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QDesktopServices>
#include <QInputDialog>
#include <QSoundEffect>
#include <QFile>
#include <QMouseEvent>
#include <QDateTime>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QScrollArea>
#include <QProgressDialog>
#include <QUrl>
#include <cstdio>
#include <exception>

#include <fcntl.h>
#include <stdlib.h>

#include <csetjmp>
#include <csignal>

#include "eventdialog.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "project.h"
#include "vfbexception.h"
#include <QStandardPaths>
#include "mainwindow.h"
#include "preferencesdialog.h"
#include "propertiesdialog.h"
//#include "table_window.h"

#ifdef USE_MUX_HACK
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
extern "C"
{
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

// The av_err2str(n) macro uses the C99 compound literal construct, which
// MSVC does not understand.
// Ref: https://ffmpeg.zeranoe.com/forum/viewtopic.php?t=4220
// Ref: https://stackoverflow.com/questions/3869963/compound-literals-in-msvc
#ifdef Q_CC_MSVC
#define local_av_err2str(n) "MSVC Cannot Show This Error"
#else
//#define local_av_err2str(n) av_err2str(n)
#define local_av_err2str(errnum) \
    av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), \
    AV_ERROR_MAX_STRING_SIZE, errnum)
#endif


#endif // ifdef USE_MUX_HACK

extern "C" void SegvHandler(int param);
static jmp_buf segvJumpEnv;
static long lastFrameLoad = 0;
static const char *traceCurrentOperation = NULL;
static const char *traceSubroutineOperation = NULL;

#ifndef UMAX
#define UMAX(b) ((1ull<<(b))-1)
#endif
#ifndef SMAX
#define SMAX(b) (UMAX((b)-1))
#endif


QImage createCheckerboard(int width, int height) {
    QImage image(width, height, QImage::Format_RGBX8888);

    QColor gray(128, 128, 128);
    QColor white(255, 255, 255);

    int squareSize = 25; // Size of each square in the checkerboard

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if ((x / squareSize) % 2 == (y / squareSize) % 2) {
                image.setPixel(x, y, white.rgb());
            } else {
                image.setPixel(x, y, gray.rgb());
            }
        }
    }

    return image;
}

//----------------------------------------------------------------------------
ExtractedSound::ExtractedSound() :
    frameIn(0),
    frameOut(0),
    gamma(100),
    gain (100),
    sCurve(0),
    overlap(20),
    lift(0),
    blur(0),
    fpsType(FPS_24),
    useBounds(true),
    usePixBounds(false),
    useSCurve(false),
    makeNegative(false),
    makeGray(false),
    sound(NULL),
    err(0)
{
    bounds[0] = bounds[1] = 0;
    pixBounds[0] = pixBounds[1] = 0;
    framePitch[0] = framePitch[1] = 0;
}

ExtractedSound::~ExtractedSound()
{
    //let the deque handle this: too many copies being made to
    // handle here. We'd need to switch to a smart pointer
#if 0
    if(sound != NULL)
    {
        sound->stop();
        QFile::remove(sound->fileName());
    }
#endif
}

bool ExtractedSound::operator ==(const ExtractedSound &ref) const
{
    if(frameIn != ref.frameIn) return false;
    if(frameOut != ref.frameOut) return false;
    if(framePitch[0] != ref.framePitch[0]) return false;
    if(framePitch[1] != ref.framePitch[1]) return false;
    if(overlap != ref.overlap) return false;

    if(useBounds != ref.useBounds) return false;
    if(usePixBounds != ref.usePixBounds) return false;
    if(useBounds &&
            ((bounds[0] != ref.bounds[0]) || (bounds[1] != ref.bounds[1])))
        return false;
    if(usePixBounds &&
            ((pixBounds[0] != ref.pixBounds[0]) ||
             (pixBounds[1] != ref.pixBounds[1])))
        return false;

    if(gamma != ref.gamma) return false;
    if(gain != ref.gain) return false;
    if(lift != ref.lift) return false;
    if(blur != ref.blur) return false;

    if(useSCurve != ref.useSCurve) return false;
    if(useSCurve && (sCurve != ref.sCurve)) return false;

    if(fpsType != ref.fpsType) return false;

    if(makeNegative != ref.makeNegative) return false;
    if(makeGray != ref.makeGray) return false;

    return true;
}

//----------------------------------------------------------------------------

#define TRANSSLIDER_VALUE 200


QString MainWindow::Version()
{
    // Version number is set in the .pro file
    QString val = QCoreApplication::applicationName().replace("-"," ") +
           QString(" v. ") +
           QCoreApplication::applicationVersion();
    if(!QString(APP_VERSION_QUAL).isEmpty())
        val += QString(" (") + APP_VERSION_QUAL + ")"
           /* + " (" __DATE__ ")"*/;

    return val;
}

void RecursivelyEnable(QLayout *l, bool enable=true)
{
    int i;
    QLayoutItem *item;

    for(i=0; i<l->count(); ++i)
    {
        item = l->itemAt(i);
        if(item)
        {
            if(item->widget()) item->widget()->setEnabled(enable);
            else if(item->layout()) RecursivelyEnable(item->layout(), enable);
        }
    }
}

void RecursivelySetVisible(QLayoutItem *item, bool visible=true)
{
    if(item->widget()) item->widget()->setVisible(visible);
    else if(item->layout())
    {
        for(int i=0; i<item->layout()->count(); ++i)
        {
            QLayoutItem *child = item->layout()->itemAt(i);
            if(child) RecursivelySetVisible(child, visible);
        }
    }
}

// -------------------------------------------------------------------
// Constructor
// -------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowIcon(QIcon(":/virtualbench256.png"));

    this->setWindowTitle(QCoreApplication::applicationName().replace("-"," "));
    #ifdef Q_OS_WINDOWS
        QString fontsize = "18pt";
    #else
        QString fontsize = "20pt";
    #endif

    ui->actionAbout->setMenuRole(QAction::AboutRole);
    ui->actionQuit->setMenuRole(QAction::QuitRole);
    ui->actionPreferences->setMenuRole(QAction::PreferencesRole);

#ifdef AVAILABLE_MAC_OS_X_VERSION_10_11_AND_LATER
    // As of 10.11 (El Capitan), OSX automagically adds "Enter Full Screen"
    // to any menu named "View", so we have to name it something else in
    // order to avoid this undesired behavior.
    // Here we append a zero-width space character.
    ui->menuView->setTitle("View\u200C");
#endif

    ui->appNameLabel->setText(
                QString("<html><head/><body><p><span style=\" font-size:"
                        + fontsize + ";\">")
                + Version() + QString("</span></p></body></html>"));

    // don't switch frames while the user is entering a value
    // to avoid loading frames 4 and 43 when the user types 432
    ui->frame_numberSpinBox->setKeyboardTracking(false);

    // set default values
    ui->showSpliceCheckBox->setChecked(true);
    ui->FramePitchendSlider->setValue(this->scan.overlapThreshold);

    ui->maxFrequencyFrame->setVisible(false);
    ui->tabWidget->setCurrentIndex(0);
    //ui->soundtrackSettingGrid->setRowStretch(0,1);
    frame_window = NULL;
    eventsWindow = NULL;
    paramCopyLock = false;
    samplesPlayed.resize(4);
    currentMeta = NULL;
    currentFrameTexture = NULL;
    outputFrameTexture = NULL;

    // turn off stuff that can't be used until a project is loaded
    ui->saveprojectButton->setEnabled(false);
    ui->actionSave_Settings->setEnabled(false);
    ui->actionShow_Overlap->setEnabled(false);
    ui->actionShow_Soundtrack_Only->setEnabled(false);
    ui->actionWaveform_Zoom->setEnabled(false);
    ui->addEventButton->setEnabled(false);
    ui->tabWidget->setEnabled(false);
    //ui->tabWidget->setTabVisible(2,false);
    RecursivelyEnable(ui->viewOptionsLayout, false);
    RecursivelyEnable(ui->frameNumberLayout, false);
    //RecursivelyEnable(ui->sampleLayout, false);

    // hide unused stuff
    QList< QWidget* > hideWidgets;
    hideWidgets << ui->UseForOverlapSpindownArrow;
    hideWidgets << ui->OverlapSoundtrackCheckBox;
    hideWidgets << ui->OverlapPixCheckBox;
    hideWidgets << ui->searchSizeLabel;
    int nCol = ui->soundtrackSettingGrid->columnCount();
    for(int row=0; row < ui->soundtrackSettingGrid->rowCount(); ++row)
    {
        for(int col=0; col<nCol; ++col)
        {
            QLayoutItem* item =
                ui->soundtrackSettingGrid->itemAtPosition(row,col);
            if(!item) continue;
            if(hideWidgets.contains(item->widget()))
            {
                for(int col=0; col < nCol; ++col)
                {
                    item = ui->soundtrackSettingGrid->itemAtPosition(row,col);
                    if(item) RecursivelySetVisible(item, false);
                }
                break; // whole row is hidden; don't have to check other cols
            }
        }
    }
    ui->CalBtn->setVisible(false);
    ui->CalEnableCB->setVisible(false);
    ui->overlapSlider->setValue(1);

    // skip license agreement if the user has already agreed to this version
    QSettings settings;
    QString ag = settings.value("license","0.1").toString();
    QVersionNumber agv = QVersionNumber::fromString(ag);
    QVersionNumber thisv = QVersionNumber::fromString(APP_VERSION_STR);

    Log() << "Starting project: " << startingProjectFilename << "\n";

    qDebug() << "Version: " << ag << " - " << APP_VERSION_STR;

    if(QVersionNumber::compare(agv, thisv) < 0)
        QTimer::singleShot(0, this, SLOT(LicenseAgreement()));
    else
        QTimer::singleShot(0, this, SLOT(OpenStartingProject()));

    prevProjectDir = "";

    // start log with timestamp
    Log() << QDateTime::currentDateTime().toString() << "\n";

    isVideoMuxingRisky = false;

#ifdef USE_MUX_HACK
    encStartFrame = 0;
    encNumFrames = 0;
    encCurFrame = 0;

    encVideoBufSize = 0;

    encRGBFrame = NULL;
    encS16Frame = NULL;

    encAudioLen = 0;
    encAudioNextPts = 0;
#endif

    playtimer.setTimerType(Qt::PreciseTimer);
    SetPlaybackInterval();
    connect(&playtimer,SIGNAL(timeout()),this,SLOT(playslot()));

    connect(ui->play_btn, SIGNAL(clicked(bool)), this, SLOT(PlaybackStart()));
    connect(ui->slow_play_drop, SIGNAL(currentTextChanged(const QString&)),
        this, SLOT(SetPlaybackInterval()));
    connect(ui->frameRateSpinBox, SIGNAL(valueChanged(int)),
        this, SLOT(SetPlaybackInterval()));

    connect(ui->rotateCheckbox,SIGNAL(stateChanged(int)),
            this, SLOT(UpdateView()));

    connect(ui->addEventButton, SIGNAL(clicked(bool)),
            this, SLOT(OpenEventWindow()));

    connect(ui->syncPitchCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(SyncPitch(int)));

    // Menu Items
    connect(ui->actionProperties, SIGNAL(triggered(bool)),
            this, SLOT(OpenPropertiesWindow()));
}

void MainWindow::UpdateView()
{
    GPU_Params_Update(true);
}

void MainWindow::playslot()
{
    if(playdir<0)
    {
        framebackward();
    }
    else
    {
        frameforward();
    }
}

void MainWindow::frameforward()
{
    if(ui->frame_numberSpinBox->value() < ui->frame_numberSpinBox->maximum())
        ui->frame_numberSpinBox->setValue(ui->frame_numberSpinBox->value()+1);
    qDebug()<<"trigger forward";
}
void MainWindow::framebackward()
{
    if(ui->frame_numberSpinBox->value() > ui->frame_numberSpinBox->minimum())
        ui->frame_numberSpinBox->setValue(ui->frame_numberSpinBox->value()-1);
    qDebug()<<"trigger backward";
}
void MainWindow::TogglePlayPause()
{
    if(playtimer.isActive())
        playtimer.stop();
    else
        playtimer.start();

}
MainWindow::~MainWindow()
{
    DeleteTempSoundFile();
    delete ui;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    if(!frame_window)
    {
        event->accept();
        return;
    }

    QMessageBox::StandardButton resBtn =
        QMessageBox::question( this, APP_NAME,
            tr("Close all windows and quit?\n"),
            QMessageBox::Cancel | QMessageBox::Yes,
            QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        QApplication::quit();
    }
}

void MainWindow::SaveDefaultsSoundtrack()
{
    QSettings settings;

    if(!scan.inFile.IsReady()) return;

    double w = scan.inFile.Width();
    double h = scan.inFile.Height();

    settings.beginGroup("soundtrack");
    settings.setValue("bounds/left", ui->leftSpinBox->value()/w);
    settings.setValue("bounds/right", ui->rightSpinBox->value()/w);
    settings.setValue("bounds/use", ui->OverlapSoundtrackCheckBox->isChecked());
    settings.setValue("pixbounds/left", ui->leftPixSpinBox->value()/w);
    settings.setValue("pixbounds/right", ui->rightPixSpinBox->value()/w);
    settings.setValue("pixbounds/use", ui->OverlapPixCheckBox->isChecked());
    settings.setValue("framepitch/start", ui->framepitchstartSlider->value()/h);
    settings.setValue("framepitch/end", ui->FramePitchendSlider->value()/h);
    settings.setValue("overlap/radius", ui->overlapSlider->value());
    settings.setValue("overlap/lock", ui->HeightCalculateBtn->isChecked());
    settings.setValue("isstereo", ui->monostereo_PD->currentIndex());
    settings.endGroup();
}

void MainWindow::SaveDefaultsImage()
{
    QSettings settings;

    settings.beginGroup("image");
    settings.setValue("lift", ui->liftSlider->value());
    settings.setValue("gamma", ui->gammaSlider->value());
    settings.setValue("gain", ui->gainSlider->value());
    settings.setValue("s-curve", ui->thresholdSlider->value());
    settings.setValue("s-curve-on", ui->threshBox->isChecked());
    settings.setValue("blur-sharp", ui->blurSlider->value());
    settings.setValue("negative", ui->negBox->isChecked());
    settings.setValue("desaturate",ui->desatBox->isChecked());
    settings.endGroup();
}

void MainWindow::SaveDefaultsAudio()
{
    QSettings settings;

    settings.beginGroup("extraction");
    //settings.setValue("sampling-rate", ui->filerate_PD->currentText());
    //settings.setValue("bit-depth", ui->bitDepthComboBox->currentText());
    //settings.setValue("time-code-advance", ui->advance_CB->currentText());
    //settings.setValue("sidecar", ui->xmlSidecarComboBox->currentText());
    settings.endGroup();
}

void MainWindow::LoadDefaults()
{
    QSettings settings;
    int intv;
    double doublev;

    if(!scan.inFile.IsReady()) return;

    int w = scan.inFile.Width();
    int h = scan.inFile.Height();

    // Soundtrack Settings
    settings.beginGroup("soundtrack");

    intv = int(settings.value("bounds/left",0).toDouble() * w + 0.5);
    ui->leftSpinBox->setValue(intv);
    ui->leftSlider->setValue(intv);
    intv = int(settings.value("bounds/right",0).toDouble() * w + 0.5);
    ui->rightSpinBox->setValue(intv);
    ui->rightSlider->setValue(intv);
    ui->OverlapSoundtrackCheckBox->setChecked(
                settings.value("bounds/use",true).toBool());

    intv = int(settings.value("pixbounds/left",0.475).toDouble() * w + 0.5);
    ui->leftPixSpinBox->setValue(intv);
    ui->leftPixSlider->setValue(intv);
    intv = int(settings.value("pixbounds/right",0.525).toDouble() * w + 0.5);
    ui->rightPixSpinBox->setValue(intv);
    ui->rightPixSlider->setValue(intv);
    ui->OverlapPixCheckBox->setChecked(
                settings.value("pixbounds/use", false).toBool());
    on_OverlapPixCheckBox_clicked(ui->OverlapPixCheckBox->isChecked());

    intv = int(settings.value("framepitch/start", 0.1).toDouble() * h + 0.5);
    ui->framepitchstartSlider->setValue(intv);
    intv = int(settings.value("framepitch/end", 0.1).toDouble() * h + 0.5);
    ui->FramePitchendSlider->setValue(intv);

    //intv = settings.value("overlap/radius",5).toInt();
    intv = 1;
    ui->overlapSlider->setValue(intv);
    ui->overlap_label->setText(QString::number(intv/100.f,'f',2));

    ui->HeightCalculateBtn->setChecked(settings.value("overlap/lock").toBool());
    ui->monostereo_PD->setCurrentIndex(settings.value("isstereo",0).toInt());

    settings.endGroup();

    // image processing settings
    settings.beginGroup("image");

    intv = settings.value("lift",0).toInt();
    ui->liftSlider->setValue(intv);
    ui->liftLabel->setText(QString::number(intv/100.f,'f',2));

    intv = settings.value("gamma",100).toInt();
    ui->gammaSlider->setValue(intv);
    ui->gammaLabel->setText(QString::number(intv/100.f,'f',2));

    intv = settings.value("gain", 100).toInt();
    ui->gainSlider->setValue(intv);
    ui->gainLabel->setText(QString::number(intv/100.f,'f',2));

    intv = settings.value("s-curve",300).toInt();
    ui->thresholdSlider->setValue(intv);
    ui->threshLabel->setText(QString::number(intv/100.f,'f',2));

    ui->threshBox->setChecked(settings.value("s-curve-on",false).toBool());

    intv = settings.value("blur-sharp", 0).toInt();
    ui->blurSlider->setValue(intv);
    ui->blurLabel->setText(QString::number(intv/100.f,'f',2));

    ui->negBox->setChecked(settings.value("negative",false).toBool());
    ui->desatBox->setChecked(settings.value("desaturate",false).toBool());
    settings.endGroup();

    // extraction/export parameters

    settings.beginGroup("extraction");
    QString txt;

    //	txt = settings.value("sampling-rate").toString();
    //	intv = ui->filerate_PD->findText(txt, Qt::MatchFixedString);
    //	if(intv != -1) ui->filerate_PD->setCurrentIndex(intv);

    //	txt = settings.value("bit-depth").toString();
    //	intv = ui->bitDepthComboBox->findText(txt, Qt::MatchFixedString);
    //	if(intv != -1) ui->bitDepthComboBox->setCurrentIndex(intv);

    //	txt = settings.value("time-code-advance").toString();
    //	intv = ui->advance_CB->findText(txt, Qt::MatchFixedString);
    //	if(intv != -1) ui->advance_CB->setCurrentIndex(intv);

    //	txt = settings.value("sidecar").toString();
    //	intv = ui->xmlSidecarComboBox->findText(txt, Qt::MatchFixedString);
    //	if(intv != -1) ui->xmlSidecarComboBox->setCurrentIndex(intv);

    settings.endGroup();

}

void MainWindow::LicenseAgreement()
{
    QFile licFile(":/LICENSE.txt");

    if(!licFile.open(QIODevice::ReadOnly))
    {
        exit(1);
    }
    QTextStream licStream(&licFile);
    QString lic = licStream.readAll();
    licFile.close();

    QDialog msg(this);
    QLabel *label;
    msg.setWindowTitle("License Agreement");
    QVBoxLayout vbox(&msg);
    label = new QLabel("<b>License Agreement</b>");
    vbox.addWidget(label,0,Qt::AlignCenter);
    label = new QLabel("Copyright (c) 2024 South Carolina Research Foundation");
    vbox.addWidget(label,0,Qt::AlignLeft);
    label = new QLabel("All Rights Reserved");
    vbox.addWidget(label,0,Qt::AlignLeft);

    QTextEdit *text = new QTextEdit();
    text->setText(lic);
    QFontMetrics fm = text->fontMetrics();
    int w = fm.averageCharWidth();
    text->setMinimumWidth(w*80);
    vbox.addWidget(text,1);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Cancel);
    QPushButton *acceptButton = new QPushButton("Accept License");
    buttonBox->addButton(acceptButton, QDialogButtonBox::AcceptRole);
    QObject::connect(buttonBox, SIGNAL(accepted()), &msg, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), &msg, SLOT(reject()));

    vbox.addWidget(buttonBox);

    int ret = msg.exec();

    if(ret != QDialog::Accepted) exit(0);

    QSettings settings;
    settings.setValue("license",APP_VERSION_STR);

    return;
}

void MainWindow::Render_Frame()
{
    if (frame_window!=NULL)
    {
        GPU_Params_Update(true);
    }
}

ExtractedSound MainWindow::ExtractionParamsFromGUI()
{
    ExtractedSound params;

    params.useBounds = ui->OverlapSoundtrackCheckBox->isChecked();
    params.usePixBounds = ui->OverlapPixCheckBox->isChecked();
    if(!params.usePixBounds) params.useBounds = true;

    if(params.useBounds)
    {
        params.bounds[0] = ui->leftSpinBox->value();
        params.bounds[1] = ui->rightSpinBox->value();
    }
    if(params.usePixBounds)
    {
        params.pixBounds[0] = ui->leftPixSpinBox->value();
        params.pixBounds[1] = ui->rightPixSpinBox->value();
    }

    params.framePitch[0] = ui->framepitchstartSlider->value();
    params.framePitch[1] = ui->FramePitchendSlider->value();

    //	params.frameIn = ui->frameInSpinBox->value();
    //	params.frameOut = ui->frameOutSpinBox->value();

    params.gamma = ui->gammaSlider->value();
    params.gain = ui->gainSlider->value();

    params.useSCurve = ui->threshBox->isChecked();
    if(params.useSCurve) params.sCurve = ui->thresholdSlider->value();

    params.overlap = ui->overlapSlider->value();
    params.lift = ui->liftSlider->value();
    params.blur = ui->blurSlider->value();

//    switch(ui->filmrate_PD->currentIndex())
//    {
//    case 0: params.fpsType = FPS_NTSC; break;
//    case 1: params.fpsType = FPS_24; break;
//    case 2: params.fpsType = FPS_25; break;
//    }

    params.makeNegative = ui->negBox->isChecked();
    params.makeGray = ui->desatBox->isChecked();

    return params;
}

//-----------------------------------------------------------------------------
// ExtractionParametersToGUI
// Copy values from the ExtractedSound params to the GUI controls
void MainWindow::ExtractionParametersToGUI(const ExtractedSound &params)
{
    ui->OverlapSoundtrackCheckBox->setChecked(params.useBounds);
    ui->OverlapPixCheckBox->setChecked(params.usePixBounds);
    on_OverlapPixCheckBox_clicked(ui->OverlapPixCheckBox->isChecked());

    if(params.useBounds)
    {
        ui->leftSpinBox->setValue(params.bounds[0]);
        ui->leftSlider->setValue(params.bounds[0]);
        ui->rightSpinBox->setValue(params.bounds[1]);
        ui->rightSlider->setValue(params.bounds[1]);
    }

    if(params.usePixBounds)
    {
        ui->leftPixSpinBox->setValue(params.pixBounds[0]);
        ui->leftPixSlider->setValue(params.pixBounds[0]);
        ui->rightPixSpinBox->setValue(params.pixBounds[1]);
        ui->rightPixSlider->setValue(params.pixBounds[1]);
    }

    ui->FramePitchendSlider->setValue(params.framePitch[1]);
    ui->framepitchstartSlider->setValue(params.framePitch[0]);

    ui->maxFrequencyLabel->setText(
                QString("%1").arg(float(this->MaxFrequency())/1000.0));

    //	ui->frameInSpinBox->setValue(params.frameIn);
    //	ui->frameOutSpinBox->setValue(params.frameOut);

    ui->gammaSlider->setValue(params.gamma);
    ui->gainSlider->setValue(params.gain);

    ui->threshBox->setChecked(params.useSCurve);
    if(params.useSCurve)
        ui->thresholdSlider->setValue(params.sCurve);

    //ui->overlapSlider->setValue(params.overlap);
    ui->liftSlider->setValue(params.lift);
    ui->blurSlider->setValue(params.blur);

//    switch(params.fpsType)
//    {
//    case FPS_NTSC: ui->filmrate_PD->setCurrentIndex(0); break;
//    case FPS_24: ui->filmrate_PD->setCurrentIndex(1); break;
//    case FPS_25: ui->filmrate_PD->setCurrentIndex(2); break;
//    }

    ui->negBox->setChecked(params.makeNegative);
    ui->desatBox->setChecked(params.makeGray);

    GPU_Params_Update(1);
}

//-----------------------------------------------------------------------------
// GUI_Params_Update
// Copy values from the GPU settings in frame_window to the GUI controls
void MainWindow::GUI_Params_Update()
{
    // prevent a loop-back from GPU_Params_Update
    if(paramCopyLock) return;
    paramCopyLock = true;

    // If the OpenGL window isn't ready with a scan, skip the update
    if (frame_window!=NULL && this->scan.inFile.IsReady())
    {
        ui->leftSpinBox->setValue(
                    frame_window->bounds[0] * this->scan.inFile.Width());
        ui->leftSlider->setValue(
                    frame_window->bounds[0] * this->scan.inFile.Width());
        ui->rightSpinBox->setValue(
                    frame_window->bounds[1] * this->scan.inFile.Width());
        ui->rightSlider->setValue(
                    frame_window->bounds[1] * this->scan.inFile.Width());

        ui->leftPixSpinBox->setValue(
                    frame_window->pixbounds[0] * this->scan.inFile.Width());
        ui->leftPixSlider->setValue(
                    frame_window->pixbounds[0] * this->scan.inFile.Width());
        ui->rightPixSpinBox->setValue(
                    frame_window->pixbounds[1] * this->scan.inFile.Width());
        ui->rightPixSlider->setValue(
                    frame_window->pixbounds[1] * this->scan.inFile.Width());

        int guiStart = frame_window->overlap[3]*1000.0f;
        int guiEnd = frame_window->overlap[2]*1000.0f;
        int dStart = std::abs(ui->framepitchstartSlider->value() - guiStart);
        int dEnd = std::abs(ui->FramePitchendSlider->value() - guiEnd);

        if(ui->syncPitchCheckBox->isChecked())
        {
            if(dStart > dEnd)
            {
                ui->framepitchstartSlider->setValue(guiStart);
                ui->FramePitchendSlider->setValue(guiStart);
                frame_window->overlap[2] = frame_window->overlap[3];
            }
            else if(dEnd > 0)
            {
                ui->framepitchstartSlider->setValue(guiEnd);
                ui->FramePitchendSlider->setValue(guiEnd);
                frame_window->overlap[3] = frame_window->overlap[2];
            }
        }
        else
        {
            if(dStart > 0) ui->framepitchstartSlider->setValue(guiStart);
            if(dEnd > 0) ui->framepitchstartSlider->setValue(guiEnd);
        }

        ui->maxFrequencyLabel->setText(
                    QString("%1").arg(float(this->MaxFrequency())/1000.0));
    }

    // release the lock
    paramCopyLock = false;
}

//-----------------------------------------------------------------------------
// GPU_Params_Update
// Copy values from the GUI controls to the GPU settings in frame_window
void MainWindow::GPU_Params_Update(bool renderyes)
{
    // prevent a loop-back from GUI_Params_Update
    if(paramCopyLock) return;
    paramCopyLock = true;

    // If the OpenGL window isn't ready with a scan, skip the update
    if (frame_window!=NULL && this->scan.inFile.IsReady())
    {
        frame_window->spliceshow = false;
        if(ui->showSpliceCheckBox->checkState())
        {
            int query_idx = ui->frame_numberSpinBox->value();
            qDebug() << "Debug Checking QMap Query Frame Num = " << query_idx;

            frame_window->spliceFrameNum = query_idx;
            frame_window->currentevents = vbscan.FilmEventsForFrame(query_idx);
            frame_window->spliceshow = !frame_window->currentevents.isEmpty();
        }
        else
        {
            frame_window->currentevents.clear();


        }
        frame_window->bounds[0] =
                ui->leftSpinBox->value()/float(this->scan.inFile.Width());
        frame_window->bounds[1] =
                ui->rightSpinBox->value()/float(this->scan.inFile.Width());

        frame_window->pixbounds[0] =
                ui->leftPixSpinBox->value()/float(this->scan.inFile.Width());
        frame_window->pixbounds[1] =
                ui->rightPixSpinBox->value()/float(this->scan.inFile.Width());

        frame_window->overlap[0] = TRANSSLIDER_VALUE/10000.0f;
        frame_window->overlap[1] = ui->overlapSlider->value()/100.0f;
        frame_window->overlap[2] = ui->FramePitchendSlider->value()/1000.0f;
        frame_window->overlap[3] = ui->framepitchstartSlider->value()/1000.0f;
        ui->maxFrequencyLabel->setText(
                    QString("%1").arg(float(this->MaxFrequency())/1000.0));

        frame_window->gamma = ui->gammaSlider->value()/100.0f;
        frame_window->lift = ui->liftSlider->value()/100.0f;
        frame_window->gain = ui->gainSlider->value()/100.0f;
        frame_window->blur = ui->blurSlider->value()/100.0f;
        frame_window->threshold = ui->thresholdSlider->value()/100.0f;
        frame_window->thresh = ui->threshBox->checkState();
        frame_window->trackonly = ui->actionShow_Soundtrack_Only->isChecked();
        frame_window->negative = ui->negBox->checkState();
        frame_window->desaturate = ui->desatBox->checkState();
        frame_window->overlapshow = ui->actionShow_Overlap->isChecked();

        if(ui->rotateCheckbox->isChecked())
            frame_window->rot_angle = ui->degreeSpinBox->value();
        else
            frame_window->rot_angle = 0.0f;

        // Note: if none are checked, we still use the soundtrack (target=1)
        if(ui->OverlapPixCheckBox->isChecked())
        {
            if(ui->OverlapSoundtrackCheckBox->isChecked())
                frame_window->overlap_target = 2.0;
            else
                frame_window->overlap_target = 1.0;
        }
        else
            frame_window->overlap_target = 0.0;

        if (ui->actionWaveform_Zoom->isChecked())
            frame_window->WFMzoom=10.0f;
        else
            frame_window->WFMzoom=1.0f;

        if (renderyes)
            frame_window->renderNow();
    }

    // release the lock
    paramCopyLock = false;
}

//-----------------------------------------------------------------------------
bool MainWindow::Load_Frame_Texture(int frame_num)
{
    if (frame_window==NULL) return false;

    if(!frame_window->isExposed())
    {
        Log() << "Frame window not exposed. Attempting to raise.\n";

        frame_window->raise();
        frame_window->showNormal();
        //frame_window->setWindowState(Qt::WindowActive);
        frame_window->setWindowState(Qt::WindowNoState);
        //qApp->setActiveWindow(frame_window);

        qApp->processEvents();

        while (!frame_window->isExposed())
        {
            Log() << "Frame window still not exposed. Asking for user help.\n";
            int raise = QMessageBox::question(this, "Select image Window",
                                              "The image window is obscured. Please move it to the front.",
                                              QMessageBox::Ok | QMessageBox::Cancel,
                                              QMessageBox::Ok);

            if(raise == QMessageBox::Cancel)
            {
                Log() << "User cancelled (could not raise window?)\n";
                LogClose();
                return false;
            }
        }
    }


    traceCurrentOperation = "Retrieving scan image";

    if (frame_num>=0 && frame_num<scan.inFile.LastFrame())
    {
    currentFrameTexture = this->scan.inFile.GetFrameImage(
                this->scan.inFile.FirstFrame()+frame_num, currentFrameTexture);
    traceCurrentOperation = "Loading scan into texture";
    }
    else
    {

    frame_window->load_frame_texture(&blankframe);

        frame_window->renderNow();
    return true;
    }
 frame_window->load_frame_texture(currentFrameTexture);

    /*
    traceCurrentOperation = "Freeing texture buffer";
    if (frameTex)
        delete frameTex;
    */

    //    struct EventInfo
    //    {
    //        std::string event_name;
    //        unsigned short event_type;
    //        float event_conf;
    //        QList<float> xywh;
    //    };
    //assign splice bounds to frame_view_gl
    //Debug


    traceCurrentOperation = "GL render";
    GPU_Params_Update(true);
    //frame_window->renderNow();
    traceCurrentOperation = "";

    // record frame in static variable for debugging/restarting from error:
    lastFrameLoad = frame_num;

    return true;
}

int MainWindow::MaxFrequency() const
{
    if(!this->frame_window) return -1;
    if(!this->scan.inFile.IsReady()) return -1;

    float fps;
//    switch(this->ui->filmrate_PD->currentIndex())
//    {
//    case 0: fps = 23.976; break;
//    case 1: fps = 24.000; break;
//    case 2: fps = 25.000; break;
//    }
    fps = this->ui->frameRateSpinBox->value();
    int nPitchLines = this->scan.inFile.Height() *
            (1 - frame_window->overlap[3] - frame_window->overlap[2]);

    return int(nPitchLines * fps / 2);
}

void MainWindow::on_sourceButton_clicked()
{
    PropertyList properties = vbscan.Properties();
    QSettings settings;
    settings.beginGroup("creator-info");
    QString id = settings.value("id").toString();
    if(!id.isEmpty())
    properties.SetValue("CreatorID", id);
    QString context = settings.value("context").toString();
    if(!context.isEmpty())
    properties.SetValue("CreatorContext", context);
    settings.endGroup();

    PropertiesDialog dialog(this, properties, true);
    if(dialog.exec() != QDialog::Accepted) return;

    QString filename(dialog.Properties().Value("FileURL"));
    if(filename.isEmpty())
    {
        QMessageBox msgError;
        msgError.setText(
            QString("No Video Source (FileURL) given"));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("No Source");
        msgError.exec();
        return;
    }

    if(this->NewSource(dialog.Properties().Value("FileURL")))
        vbscan.SetProperties(dialog.Properties());
}

bool MainWindow::NewSource(QString filename, SourceFormat ft)
{
    //Set up longjmp to catch SIGSEGV and report what was going on at the
    // time of the segment violation.
    traceCurrentOperation = "";
    traceSubroutineOperation = "";
    int jmp = setjmp(segvJumpEnv);
    if(jmp!=0)
    {
        QMessageBox::critical(NULL,"Critical Failure",
                              QString("Critical failure opening new source.\n%1\n%2").
                              arg(traceCurrentOperation,traceSubroutineOperation));

        std::exit(1);
    }

    void (*prevSegvHandler)(int);
    prevSegvHandler = std::signal(SIGSEGV, SegvHandler);

    try
    {
        traceCurrentOperation = "Opening Source";
        this->scan.SourceScan(filename.toStdString(), ft);
        traceCurrentOperation = "Verifying scan is ready";
        if(this->scan.inFile.IsReady())
        {
            if(frame_window)
            {
                traceCurrentOperation="Closing previous frame window";
                frame_window->currentOperation = &traceSubroutineOperation;
                frame_window->close();
                traceCurrentOperation="Deleting previous frame window";
                delete frame_window;
            }

            traceCurrentOperation = "Creating new frame window";
            frame_window = new Frame_Window(
                        this->scan.inFile.Width(),this->scan.inFile.Height());

            frame_window->currentOperation = &traceSubroutineOperation;
            frame_window->setTitle(filename);
            frame_window->ParamUpdateCallback(&(this->GUI_Params_Update_Static),this);
            connect(frame_window,
                    SIGNAL(ResizedEventBoundingBox(vbevent*,float,float,float,float)),
                    &vbscan,
                    SLOT(UpdateFrameEventBoundingBox(vbevent*,float,float,float,float)));
            connect(frame_window, SIGNAL(PlayPause()),
                    this, SLOT(TogglePlayPause()));
            //****************
            QSurfaceFormat format;
            format.setRenderableType(QSurfaceFormat::OpenGL);
            format.setProfile(QSurfaceFormat::CoreProfile);


            format.setVersion(3,3);
            frame_window->setFormat(format);
            //**********

            Log() << "New frame window\n";
            frame_window->logger = &Log();

            traceCurrentOperation = "Resizing window to 640x640";
            frame_window->setBaseSize(QSize(1280,1024));
            frame_window->resize(1280, 1024);
            frame_window->setSizeIncrement(QSize(10,7));


            traceCurrentOperation = "Showing frame window";
            frame_window->show();
            qApp->processEvents();

            traceCurrentOperation = "Updating GUI controls for new source";
            //			ui->frameInSpinBox->setValue(this->scan.inFile.FirstFrame());
            //			ui->frameInTimeCodeLabel->setText(Compute_Timecode_String(0));
            //			ui->frameOutSpinBox->setValue(this->scan.inFile.LastFrame());
            //			ui->frameOutTimeCodeLabel->setText(
            //						Compute_Timecode_String(this->scan.inFile.NumFrames()-1));

            blankframe.width=(this->scan.inFile.Width());
   blankframe.height=(this->scan.inFile.Height());
            blankframe.nComponents=4;
   blankframe.format=GL_UNSIGNED_INT_8_8_8_8_REV;
            blankframe.buf=(uint8_t*)malloc( blankframe.width* blankframe.height*4);
   memcpy(blankframe.buf,createCheckerboard(blankframe.width,blankframe.height).constBits(),blankframe.width* blankframe.height*4);
            ui->frameNumberTimeCodeLabel->setText(Compute_Timecode_String(0));

            ui->rightSpinBox->setMaximum(this->scan.inFile.Width()-1);
            ui->leftSpinBox->setMaximum(this->scan.inFile.Width()-1);
            ui->rightSlider->setMaximum(this->scan.inFile.Width()-1);
            ui->leftSlider->setMaximum(this->scan.inFile.Width()-1);

            ui->rightPixSpinBox->setMaximum(this->scan.inFile.Width()-1);
            ui->leftPixSpinBox->setMaximum(this->scan.inFile.Width()-1);
            ui->rightPixSlider->setMaximum(this->scan.inFile.Width()-1);
            ui->leftPixSlider->setMaximum(this->scan.inFile.Width()-1);

            ui->leftPixSlider->setValue(int(this->scan.inFile.Width() * 0.475));
            ui->leftPixSpinBox->setValue(int(this->scan.inFile.Width() * 0.475));
            ui->rightPixSlider->setValue(int(this->scan.inFile.Width() * 0.525));
            ui->rightPixSpinBox->setValue(int(this->scan.inFile.Width() * 0.525));

            ui->playSlider->setMaximum(this->scan.inFile.NumFrames()-1);

            // finalize UI with user preferences
            LoadDefaults();

            // enable the rest of the UI that was waiting until a project loaded
            ui->actionSave_Settings->setEnabled(true);
            ui->actionProperties->setEnabled(true);
            ui->actionShow_Overlap->setEnabled(true);
            ui->actionShow_Soundtrack_Only->setEnabled(true);
            ui->actionWaveform_Zoom->setEnabled(true);
            ui->menuView->setEnabled(true);
            ui->saveprojectButton->setEnabled(true);
            ui->addEventButton->setEnabled(true);
            ui->tabWidget->setEnabled(true);
            RecursivelyEnable(ui->viewOptionsLayout, true);
            RecursivelyEnable(ui->frameNumberLayout, true);
            //RecursivelyEnable(ui->sampleLayout, false);
            //			ui->playSampleButton->setEnabled(true);
            //			ui->autoLoadSettingsCheckBox->setEnabled(true);

            traceCurrentOperation = "Updating GPU params";
            GPU_Params_Update(0);
            traceCurrentOperation = "Displaying first frame";

            Load_Frame_Texture(0);

        }

    }
    catch(std::exception &e)
    {
        QMessageBox w;
        QString msg("Error opening source: \n");
        int answer;

        msg += QString(e.what());

        w.setText(msg);
        w.setStandardButtons(QMessageBox::Abort | QMessageBox::Ok);
        w.setDefaultButton(QMessageBox::Ok);
        answer = w.exec();

        if(answer == QMessageBox::Abort) exit(1);
        return false;
    }

    traceCurrentOperation = "";
    traceSubroutineOperation = "";
    this->frame_window->logger = NULL;
    this->frame_window->currentOperation = NULL;

    // restore the previous SEGV handler
    if(prevSegvHandler != SIG_ERR)
        std::signal(SIGSEGV, prevSegvHandler);

    return true;
}

//********************* Project Load and Save *********************************
bool MainWindow::saveproject(QString fn)
{
    QFile file(fn);
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);   // we will serialize the data into the file

    saveproject(out);

    file.close();

    return true;
}

bool MainWindow::saveproject(QTextStream &out)
{
    out<<"Virtual-Bench Project Settings\n";

    // Source Data
    out<<"Source Scan = "<<this->scan.inFile.GetFileName().c_str()<<"\n";
    out<<"Source Format = "<<this->scan.inFile.GetFormatStr()<<"\n";
    //out<<"Frame Rate = "<<ui->filmrate_PD->currentText()<<"\n";
    out<<"Frame Rate = "<<ui->frameRateSpinBox->value()<<"\n";

    // Sondtrack Settings
    out<<"Use Soundtrack = "<<ui->OverlapSoundtrackCheckBox->isChecked()<<"\n";
    out<<"Left Boundary = "<<ui->leftSpinBox->value()<<"\n";
    out<<"Right Boundary = "<<ui->rightSpinBox->value()<<"\n";
    out<<"Soundtrack Type = "<<ui->monostereo_PD->currentText()<<"\n";
    out<<"Use Pix Track = "<<ui->OverlapPixCheckBox->isChecked()<<"\n";
    out<<"Left Pix Boundary = "<<ui->leftPixSpinBox->value()<<"\n";
    out<<"Right Pix Boundary = "<<ui->rightPixSpinBox->value()<<"\n";
    out<<"Frame Pitch Start = "<<ui->framepitchstartSlider->value()<<"\n";
    out<<"Frame Pitch End = "<<ui->FramePitchendSlider->value()<<"\n";
    out<<"Overlap Search Size = "<<ui->overlapSlider->value()<<"\n";
    out<<"Frame Translation = "<< TRANSSLIDER_VALUE <<"\n";

    // Image Processing Settings
    out<<"Lift = "<<ui->liftSlider->value()<<"\n";
    out<<"Gamma = "<<ui->gammaSlider->value()<<"\n";
    out<<"Gain = "<<ui->gainSlider->value()<<"\n";
    out<<"S-Curve Value = "<<ui->thresholdSlider->value()<<"\n";
    out<<"S-Curve On = "<<ui->threshBox->checkState()<<"\n";
    out<<"Blur = "<<ui->blurSlider->value()<<"\n";
    out<<"Negative = "<<ui->negBox->checkState()<<"\n";
    out<<"Desaturate = "<<ui->desatBox->checkState()<<"\n";
    out<<"Calibrate = "<<ui->CalEnableCB->checkState()<<"\n";

#ifdef SAVE_CALIBRATION_MASK_IN_PROJECT
    int numFloats = frame_window->cal_points;
    float *mask = frame_window->GetCalibrationMask();
    uchar *cmask = reinterpret_cast<uchar *>(mask);
    QByteArray bytes = qCompress(cmask, numFloats*sizeof(float), 9);
    out<<"Calibration Mask = "<<(bytes.toBase64())<<"\n";

    cmask=NULL;
    delete [] mask;
    mask = NULL;
#endif

    // Extraction Settings
    //	out<<"Export Frame In = "<<ui->frameInSpinBox->value()<<"\n";
    //	out<<"Export Frame Out = "<<ui->frameOutSpinBox->value()<<"\n";
    //	out<<"Export Sampling Rate = "<<ui->filerate_PD->currentText()<<"\n";
    //	out<<"Export Bit Depth = "<<ui->bitDepthComboBox->currentText()<<"\n";
    //	// This is probably more of a source setting than an export setting:
    //	out<<"Timecode Advance = "<<ui->advance_CB->currentIndex()<<"\n";
    //	out<<"Export XML Sidecar = "<<ui->xmlSidecarComboBox->currentText()<<"\n";

    // View Settings
    //out<<"View Zoom = "<<ui->waveformZoomCheckBox->isChecked()<<"\n";
    out<<"View Overlap = "<<ui->showOverlapCheckBox->isChecked()<<"\n";
    out<<"View Soundtrack Only = "<<
         ui->showSoundtrackOnlyCheckBox->isChecked()<<"\n";
    out<<"View Frame = "<<ui->frame_numberSpinBox->value()<<"\n";

    return true;
}

bool MainWindow::OpenProject(QString fn)
{
    if(!vbscan.load(fn)) return false;

    float overlap_start = vbscan.overlap_framestart;
    float overlap_end = vbscan.overlap_frameend;

    QString videoSource(vbscan.FileURL());
    if(videoSource.isEmpty())
    {
        QMessageBox msgError(this);
        msgError.setText(
            QString("No Video Source (FileURL) given"));
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("No Source");
        msgError.exec();
        return false;
    }
    QFileInfo finfo(videoSource);
    while(!finfo.exists())
    {
        QMessageBox msg(this);
        msg.setText(QString("Video Source File '%1' not found").
                    arg(finfo.fileName()));
        msg.setWindowTitle("Source not found");
        QPushButton *locButton = new QPushButton("Locate");
        msg.addButton(locButton, QMessageBox::AcceptRole);
        QPushButton *retryButton = new QPushButton("Retry");
        msg.addButton(retryButton, QMessageBox::AcceptRole);
        msg.addButton(QMessageBox::Cancel);

        msg.exec();

        if(msg.clickedButton() == retryButton)
        {
            finfo = QFileInfo(videoSource);
            continue;
        }
        if(msg.clickedButton() != locButton) return false;

        videoSource = QFileDialog::getOpenFileName(
            this,
            tr("Video Source"), finfo.absolutePath(),
            QString("%1 files (*.%2);;All files (*)").
            arg(finfo.suffix(), finfo.suffix()));
        if(videoSource.isEmpty()) return false;

        vbscan.SetFileURL(videoSource);
        finfo = QFileInfo(videoSource);
    }

    SourceFormat sf;
    if (vbscan.FileURL().contains(".dpx",Qt::CaseInsensitive))
        sf=SOURCE_DPX;
    else
        sf=SOURCE_LIBAV;
    qDebug()<<"file url: "<<vbscan.FileURL();
    this->NewSource(vbscan.FileURL(), sf);

    ui->framepitchstartSlider->setValue(overlap_start*1000.f);
    ui->FramePitchendSlider->setValue(overlap_end*1000.f);
    frame_window->overlap[3]=vbscan.overlap_framestart = overlap_start;
    frame_window->overlap[2]=vbscan.overlap_frameend = overlap_end;

    this->prevProjectDir = QFileInfo(fn).absolutePath();

    if(!vbscan.FilmEvents().isEmpty())
        OpenEventWindow();

    updateBenchCounter();

    return true;
}

void MainWindow::OpenStartingProject()
{
    Log() << "Opening starting project\n";
    if(!this->startingProjectFilename.isEmpty())
        this->OpenProject(this->startingProjectFilename);
}

bool MainWindow::LoadProjectSettings(QString fn)
{
    QFile file(fn);
    file.open(QIODevice::ReadOnly);
    QTextStream in(&file);   // we will serialize the data into the file

    bool deleteMe = false;
    bool needMask = false;
    bool gotMask = false;

    while(!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(QRegularExpression("\\s*=\\s*"));

        if ((fields[0]).contains("Frame Rate"))
        {
            //int idx = ui->filmrate_PD->findText(fields[1],Qt::MatchExactly);
            //if(idx != -1) ui->filmrate_PD->setCurrentIndex(idx);
            ui->frameRateSpinBox->setValue(fields[1].toDouble());
        }
        if ((fields[0]).contains("Use Soundtrack"))
            ui->OverlapSoundtrackCheckBox->setChecked(fields[1].toInt());
        if ((fields[0]).contains("Left Bound"))
            ui->leftSpinBox->setValue(fields[1].toInt());
        if ((fields[0]).contains("Right Bound"))
            ui->rightSpinBox->setValue(fields[1].toInt());
        if ((fields[0]).contains("Soundtrack Type"))
        {
            int idx = ui->monostereo_PD->findText(fields[1],Qt::MatchExactly);
            if(idx != -1) ui->monostereo_PD->setCurrentIndex(idx);
        }
        if ((fields[0]).contains("Use Pix Track"))
            ui->OverlapPixCheckBox->setChecked(fields[1].toInt());
        if ((fields[0]).contains("Left Pix Bound"))
            ui->leftPixSpinBox->setValue(fields[1].toInt());
        if ((fields[0]).contains("Right Pix Bound"))
            ui->rightPixSpinBox->setValue(fields[1].toInt());
        if ((fields[0]).contains("Frame Pitch Start"))
            ui->framepitchstartSlider->setValue(fields[1].toInt());
        if ((fields[0]).contains("Frame Pitch End"))
            ui->FramePitchendSlider->setValue(fields[1].toInt());
        //if ((fields[0]).contains("Overlap Search Size"))
        //    ui->overlapSlider->setValue(fields[1].toInt());
        if ((fields[0]).contains("Lift"))
            ui->liftSlider->setValue(fields[1].toInt());
        if ((fields[0]).contains("Gamma"))
            ui->gammaSlider->setValue(fields[1].toInt());
        if ((fields[0]).contains("Gain"))
            ui->gainSlider->setValue(fields[1].toInt());
        if ((fields[0]).contains("S-Curve Value"))
            ui->thresholdSlider->setValue(fields[1].toInt());
        if ((fields[0]).contains("S-Curve On"))
            ui->threshBox->setChecked(fields[1].toInt());
        if ((fields[0]).contains("Blur"))
            ui->blurSlider->setValue(fields[1].toInt());
        if ((fields[0]).contains("Negative"))
            ui->negBox->setChecked(fields[1].toInt());
        if ((fields[0]).contains("Desaturate"))
            ui->desatBox->setChecked(fields[1].toInt());

        if ((fields[0]).contains("Calibrate"))
        {
            ui->CalEnableCB->setChecked(fields[1].toInt());
            needMask = true;
        }

#ifdef SAVE_CALIBRATION_MASK_IN_PROJECT
        if((fields[0]).contains("Calibration Mask"))
        {
            QByteArray bytes = qUncompress(fields[1].toUtf8());
            const char *cmask = bytes.constData();
            const float *mask = reinterpret_cast<const float *>(cmask);
            frame_window->SetCalibrationMask(mask);
            gotMask = true;
        }
#endif

        //		// Extraction Settings
        //		if ((fields[0]).contains("Export Frame In"))
        //			ui->frameInSpinBox->setValue(fields[1].toInt());
        //		if ((fields[0]).contains("Export Frame Out"))
        //			ui->frameOutSpinBox->setValue(fields[1].toInt());
        //		if ((fields[0]).contains("Export Sampling Rate"))
        //		{
        //			int idx = ui->filerate_PD->findText(fields[1],Qt::MatchExactly);
        //			if(idx != -1) ui->filerate_PD->setCurrentIndex(idx);
        //		}
        //		if ((fields[0]).contains("Export Bit Depth"))
        //		{
        //			int idx = ui->bitDepthComboBox->
        //					findText(fields[1],Qt::MatchExactly);
        //			if(idx != -1) ui->bitDepthComboBox->setCurrentIndex(idx);
        //		}
        //		// This is probably more of a source setting than an export setting:
        //		if ((fields[0]).contains("Timecode Advance"))
        //			ui->advance_CB->setCurrentIndex(fields[1].toInt());

        //		if ((fields[0]).contains("Export XML Sidecar"))
        //		{
        //			int idx = ui->xmlSidecarComboBox->
        //					findText(fields[1],Qt::MatchExactly);
        //			if(idx != -1) ui->xmlSidecarComboBox->setCurrentIndex(idx);
        //		}

        // View Settings
//        if ((fields[0]).contains("View Zoom"))
//            ui->waveformZoomCheckBox->setChecked(fields[1].toInt());
        if ((fields[0]).contains("View Overlap"))
            ui->showOverlapCheckBox->setChecked(fields[1].toInt());
        if ((fields[0]).contains("View Soundtrack Only"))
            ui->showSoundtrackOnlyCheckBox->setChecked(fields[1].toInt());
        if ((fields[0]).contains("View Frame"))
            ui->frame_numberSpinBox->setValue(fields[1].toInt());

        if((fields[0]).contains("DeleteMe"))
            deleteMe = (fields[1].contains("true"));
    }

    file.close();
    if(deleteMe) file.remove();

    //if(gotMask)
    //	ui->CalEnableCB->setEnabled(true);
    //else if(needMask)
    //	QTimer::singleShot(20, this, SLOT(on_CalBtn_clicked()));

    on_OverlapPixCheckBox_clicked(ui->OverlapPixCheckBox->isChecked());

    return true;
}


void MainWindow::on_saveprojectButton_clicked()
{
    QString savDir;

    if(this->prevProjectDir.isEmpty())
    {
        QSettings settings;
        settings.beginGroup("default-folder");
        savDir = settings.value("project").toString();
        if(savDir.isEmpty())
        {
            savDir = QStandardPaths::writableLocation(
                        QStandardPaths::DocumentsLocation);
        }
        settings.endGroup();
    }
    else
    {
        savDir = this->prevProjectDir;
    }

    QKeySequence shortcutNext = ui->actionNext_Frame->shortcut();
    QKeySequence shortcutPrev = ui->actionPrev_Frame->shortcut();
    ui->actionNext_Frame->setShortcut(QKeySequence());
    ui->actionPrev_Frame->setShortcut(QKeySequence());

    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Project"), savDir,
        tr("VB Project Files (*.vfbp)"));

    ui->actionNext_Frame->setShortcut(shortcutNext);
    ui->actionPrev_Frame->setShortcut(shortcutPrev);

    if(fileName.isEmpty()) return;

    vbscan.save(fileName);

    // saveproject(fileName);
    // this->prevProjectDir = QFileInfo(fileName).absolutePath();
}

void MainWindow::on_loadprojectButton_clicked()
{
    QString prjDir;

    if(this->prevProjectDir.isEmpty())
    {
        QSettings settings;
        settings.beginGroup("default-folder");
        prjDir = settings.value("project").toString();
        if(prjDir.isEmpty())
        {
            prjDir = QStandardPaths::writableLocation(
                        QStandardPaths::DocumentsLocation);
        }
        settings.endGroup();
    }
    else
    {
        prjDir = this->prevProjectDir;
    }

    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Project"), prjDir,
        tr("VB Project Files (*.vfbp)"));
    if(fileName.isEmpty()) return;

    OpenProject(fileName);
}

//*********************Sequence & Track Selection UI **************************
void MainWindow::on_leftPixSlider_sliderMoved(int position)
{
    ui->leftPixSpinBox->setValue(position);
}

void MainWindow::on_rightPixSlider_sliderMoved(int position)
{
    ui->rightPixSpinBox->setValue(position);
}

void MainWindow::on_leftPixSpinBox_valueChanged(int arg1)
{
    ui->leftPixSlider->setValue(arg1);
    GPU_Params_Update(1);
}

void MainWindow::on_rightPixSpinBox_valueChanged(int arg1)
{
    ui->rightPixSlider->setValue(arg1);
    GPU_Params_Update(1);
}

void MainWindow::on_leftSpinBox_valueChanged(int arg1)
{
    ui->leftSlider->setValue(arg1);
    GPU_Params_Update(1);
}

void MainWindow::on_rightSpinBox_valueChanged(int arg1)
{
    ui->rightSlider->setValue(arg1);
    GPU_Params_Update(1);
}
void MainWindow::on_leftSlider_sliderMoved(int position)
{
    ui->leftSpinBox->setValue(position);
}
void MainWindow::on_rightSlider_sliderMoved(int position)
{
    ui->rightSpinBox->setValue(position);
}
void MainWindow:: loadfromrandomaccess(int position)
{
   /* if(position > ui->frame_numberSpinBox->minimum()+2 && position < ui->frame_numberSpinBox->maximum()-2)
    {
        Load_Frame_Texture(position-2);
        Load_Frame_Texture(position-1);
        Load_Frame_Texture(position);
        Load_Frame_Texture(position+1);
        Load_Frame_Texture(position+2);
    }
    //   Load_Frame_Texture(position);
    ui->frame_numberSpinBox->setValue(position);
    ui->frameNumberTimeCodeLabel->setText( Compute_Timecode_String(position));
*/

}

void MainWindow::jumpToFrame(uint32_t frameNum)
{
  //  uint32_t firstFrame = std::max(frameNum,2u)-2u;
  //  uint32_t lastFrame = std::min(frameNum+2,this->scan.lastFrameIndex);

 //   for(uint32_t i=firstFrame; i<= lastFrame; i++)
   //     ui->frame_numberSpinBox->setValue(i);

    ui->frame_numberSpinBox->setValue(frameNum);
   // ui->frameNumberTimeCodeLabel->setText( Compute_Timecode_String(frameNum));
}

float MainWindow::MarqueeX0() const
{
    if(!frame_window) return 0.0;
    return frame_window->marqueeBounds[0];
}
float MainWindow::MarqueeX1() const
{
    if(!frame_window) return 0.0;
    return frame_window->marqueeBounds[1];
}
float MainWindow::MarqueeY0() const
{
    if(!frame_window) return 0.0;
    return frame_window->marqueeBounds[2];
}
float MainWindow::MarqueeY1() const
{
    if(!frame_window) return 0.0;
    return frame_window->marqueeBounds[3];
}

void MainWindow::MarqueeClear()
{
    if(!frame_window) return;
    frame_window->marqueeBounds[0] =
        frame_window->marqueeBounds[1] =
        frame_window->marqueeBounds[2] =
        frame_window->marqueeBounds[3] = 0.0;
}

void MainWindow::on_playSlider_sliderMoved(int position)
{
    FrameTexture *ft;

    ui->playSlider->blockSignals(true);

    ui->frame_numberSpinBox->setValue(position);

    ui->playSlider->blockSignals(false);
    //  loadfromrandomaccess(position);
    //if(position > ui->frame_numberSpinBox->minimum())
    // {
    //     Load_Frame_Texture(position-1);
    // }
    // Load_Frame_Texture(position);

    ui->frameNumberTimeCodeLabel->setText( Compute_Timecode_String(position));

}

//void MainWindow::on_rotateButton_clicked()
//{
//    //	ui->frameOutSpinBox->setValue(
//    //			this->scan.inFile.FirstFrame()+ui->playSlider->value());
//    qDebug() << "Rotate degree " << rot_degree;
//    GPU_Params_Update(1);
//}

void MainWindow::on_frameRateSpinBox_valueChanged(int arg1)
{
    // load the previous frame so that overlap can be computed
    //rot_degree = arg1;
    qDebug() << "See frame rate " << arg1;
    GPU_Params_Update(1);

}

void MainWindow::on_degreeSpinBox_valueChanged(double arg1)
{
    Q_UNUSED(arg1);
    if(ui->rotateCheckbox->isChecked())
        GPU_Params_Update(1);
}

void MainWindow::on_framepitchstartSlider_valueChanged(int value)
{
    bool isBlocking = ui->framepitchstartSlider->blockSignals(true);

    ui->framepitchstartLabel->setText(QString::number(value/1000.f,'f',2));

    // if we're syncing and the other side needs to be updated, let
    // it also handle the GPU_Params_Update call
    if(
        (ui->syncPitchCheckBox->isChecked()) &&
        (ui->FramePitchendSlider->value() != value))
    {
        ui->FramePitchendSlider->setValue(value);
    }
    else
    {
        GPU_Params_Update(1);
    }

    vbscan.overlap_framestart = (value/1000.f);

    ui->framepitchstartSlider->blockSignals(isBlocking);
}

void MainWindow::SyncPitch(int state)
{
    if(state != Qt::Unchecked)
    {
        int start = ui->framepitchstartSlider->value();

        if(start != ui->FramePitchendSlider->value())
            ui->FramePitchendSlider->setValue(start);
    }
}

void MainWindow::on_overlapSlider_valueChanged(int value)
{
    ui->overlap_label->setText(QString::number(value/100.f,'f',2));
    GPU_Params_Update(1);
}

void MainWindow::on_frame_numberSpinBox_valueChanged(int arg1)
{
    if(arg1 <0)
        return;

    // load the previous frame so that overlap can be computed
    if(arg1 > scan.lastFrameIndex){
        playtimer.stop();
        return;
    } //STELL ADDED
    //if(arg1 > ui->frame_numberSpinBox->minimum())
    //Load_Frame_Texture(arg1-1);


    ui->playSlider->setValue(arg1);
    currentframe =arg1;
    ui->frameNumberTimeCodeLabel->setText( Compute_Timecode_String(arg1));
    updateBenchCounter();


    qDebug()<<"This Frame: " <<arg1;
    frame_window->fbm->displayCurrentBuckets();
    QPair<QList<int>, QList<int>> frameloadinfo= frame_window->fbm->getNeededFrameNumbers(arg1);
    qDebug()<<(frameloadinfo.first);
    qDebug()<<(frameloadinfo.second);

    for(int x =0;x< frameloadinfo.first.length(); x++ )
    {

       frame_window->currentframenumber=frameloadinfo.first[x];
       frame_window->currentbufferid=frameloadinfo.second[x];
       Load_Frame_Texture(frameloadinfo.first[x]);
    }


    emit NewFrameLoaded(uint32_t(arg1));
}


void MainWindow::on_HeightCalculateBtn_clicked()
{
    if (frame_window!=NULL && frame_window->is_calc==false)
    {
        frame_window->is_calc=false;

        frame_window->bestloc=frame_window->bestmatch.postion;

        this->ui->FramePitchendSlider->setValue(
                    int( (1.0-(1.0+(frame_window->overlap[3] -
                               frame_window->overlap[0]))) * 1000.0));

        this->ui->overlapSlider->setValue(1);
        this->ui->overlap_label->setText(QString::number(1./100.f,'f',2));
        frame_window->currmatch = frame_window->bestmatch;
        frame_window->currstart = frame_window->overlap[3];
        frame_window->is_calc=true;

        //this->ui->FramePitchendSlider->setEnabled(false);

        this->ui->HeightCalculateBtn->setText("Unlock Height");
    }
    else
    {
        frame_window->is_calc=false;

        this->ui->FramePitchendSlider->setEnabled(true);

        this->ui->HeightCalculateBtn->setText("Lock Height");
    }
}



bool MainWindow::extractGL(QString filename=QString(), bool doTimer=false)
{
    bool batch = true;
    return batch;
}


#ifdef USE_MUX_HACK
//----------------------------------------------------------------------------
//---------------------- MUX.C -----------------------------------------------
//----------------------------------------------------------------------------
/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat API example.
 *
 * Output a media file in any supported libavformat format. The default
 * codecs are used.
 * @example muxing.c
 */

#define STREAM_DURATION   10.0
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define RGB_PIX_FMT       AV_PIX_FMT_RGBA

#define SCALE_FLAGS SWS_BICUBIC

bool MainWindow::EnqueueNextFrame()
{
    if(this->encCurFrame > this->encNumFrames) return false;

    traceCurrentOperation = "Load Texture";

    av_log(NULL, AV_LOG_INFO, "encStartFrame = %ld\n", this->encStartFrame);
    av_log(NULL, AV_LOG_INFO, "encCurFrame = %ld\n", this->encCurFrame);

    long framenum = this->encStartFrame + this->encCurFrame;

    av_log(NULL, AV_LOG_INFO, "framenum = %ld\n", framenum);

    if(framenum > this->scan.inFile.NumFrames()-1)
    {
        // TODO: correct the sound array allocation steps above
        // so that we don't have to load the final frame twice
        av_log(NULL, AV_LOG_INFO, "Load_Frame_Texture(%ld)\n", framenum-1);
        if(!Load_Frame_Texture(framenum - 1)) return false;
    }
    else
    {
        av_log(NULL, AV_LOG_INFO, "Load_Frame_Texture(%ld)\n", framenum);
        if(!Load_Frame_Texture(framenum)) return false;
    }

    // XXX: Warning: this reads to vo.videobuffer instead -- do not change
    // the argument expecting it to work.
    av_log(NULL, AV_LOG_INFO, "this->frame_window->read_frame_texture(this->outputFrameTexture)\n");
    this->frame_window->read_frame_texture(this->outputFrameTexture);

    av_log(NULL, AV_LOG_INFO, "p = new uint8_t[%ld]\n", this->encVideoBufSize);
    uint8_t *p = new uint8_t[this->encVideoBufSize];
    av_log(NULL, AV_LOG_INFO, "memcpy(p, this->frame_window->vo.videobuffer, this->encVideoBufSize)\n");
    memcpy(p, this->frame_window->vo.videobuffer, this->encVideoBufSize);
    av_log(NULL, AV_LOG_INFO, "enqueue p = %p into this->encVideoQueue\n", p);
    this->encVideoQueue.push(p);

    // update audio signal render buffer length
    this->encAudioLen += this->frame_window->samplesperframe_file;
    av_log(NULL, AV_LOG_INFO, "audio render len = %lu\n", (unsigned long)(this->encAudioLen));

    this->encCurFrame++;

    return true;
}

uint8_t *MainWindow::GetVideoFromQueue()
{
    uint8_t *p;

    av_log(NULL, AV_LOG_INFO, "encVideoQueue.empty() = %c\n", encVideoQueue.empty()?'T':'F');
    av_log(NULL, AV_LOG_INFO, "encVideoQueue.size() = %lu\n", encVideoQueue.size());

    if(this->encVideoQueue.empty())
    {
        if(!EnqueueNextFrame()) return NULL;
    }
    p = this->encVideoQueue.front();
    this->encVideoQueue.pop();

    av_log(NULL, AV_LOG_INFO, "return p = %p\n", p);
    return p;
}

AVFrame *MainWindow::GetAudioFromQueue()
{
    const int nbits=16;

    while(this->encS16Frame->pts + this->encS16Frame->nb_samples > this->encAudioLen)
    {
        if(!EnqueueNextFrame()) return NULL;
    }

    int64_t offset = this->encAudioNextPts - this->encAudioPad;
    av_log(NULL, AV_LOG_INFO, "audio render offset = %lld\n", offset);
    av_log(NULL, AV_LOG_INFO, "audio render len = %lld\n", this->encAudioLen);

    if(this->encS16Frame->channels != 2)
        throw vfbexception("sound must be stereo");

    int16_t *samples = (int16_t *)(this->encS16Frame->data[0]);
    int s = 0;

    int16_t v;

    if(offset < 0)
    {
        for(int i = 0; i<this->encS16Frame->nb_samples; ++i)
        {
            for(int c = 0; c < this->encS16Frame->channels; ++c)
            {
                samples[s++] = 0;
            }
        }
        av_log(NULL, AV_LOG_INFO, "Silence written nb_samples = %d x%d\n",
               this->encS16Frame->nb_samples, this->encS16Frame->channels);
    }
    else
    {
        // translate the floating point values to S16:
        float **audio = this->frame_window->FileRealBuffer;
        av_log(NULL, AV_LOG_INFO, "audio = FileRealBuffer = [%p,%p]\n",
               audio[0], audio[1]);
        av_log(NULL, AV_LOG_INFO, "Audio copy nb_samples = %d x%d\n",
               this->encS16Frame->nb_samples, this->encS16Frame->channels);
        for(int i = 0; i<this->encS16Frame->nb_samples; ++i)
        {
            for(int c = 0; c < this->encS16Frame->channels; ++c)
            {
                //av_log(NULL, AV_LOG_INFO, "reading audio[%d][%lld]\n", c, offset+i);
                v = int32_t(
                            (audio[c][offset+i]*UMAX(nbits))-(UMAX(nbits)/2));
                //av_log(NULL, AV_LOG_INFO, "writing samples[%d]\n", s);
                samples[s++] = v;
            }
        }
        av_log(NULL, AV_LOG_INFO, "Audio copied nb_samples = %d x%d\n",
               this->encS16Frame->nb_samples, this->encS16Frame->channels);
    }

    this->encS16Frame->pts = this->encAudioNextPts;
    this->encAudioNextPts += this->encS16Frame->nb_samples;

    return this->encS16Frame;
}

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
#if 0
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
#endif
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
static void add_stream(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    // allocid: MainWindowStream01
    ost->st = avformat_new_stream(oc, *codec);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    av_log(NULL, AV_LOG_INFO,
           "ALLOC new: MainWindowStream01 ost->st->codec = %p\n",
           ost->st->codec);
    ost->st->id = oc->nb_streams-1;
    c = ost->st->codec;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
                    (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;

        if(ost->requestedSamplingRate > 0)
            c->sample_rate = ost->requestedSamplingRate;
        else
            c->sample_rate = 44100;

        /* Ugh. Why would you override your setting as soon as you set it?
         *
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        */

        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        else
            c->channel_layout = AV_CH_LAYOUT_STEREO;

        c->channels = av_get_channel_layout_nb_channels(c->channel_layout);

        ost->st->time_base.num = 1;
        ost->st->time_base.den = c->sample_rate;
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 400000;
        /* Resolution must be a multiple of two. */
        if(ost->requestedWidth > 0)
        {
            c->width = ost->requestedWidth;
            c->height = ost->requestedHeight;
        }
        else
        {
            c->width    = 352;
            c->height   = 288;
        }

        c->bit_rate = c->width * c->height * 4;

        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base.num = 1;
        ost->st->time_base.den = ost->requestedTimeBase;

        c->time_base       = ost->st->time_base;

        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

#ifndef AV_CODEC_FLAG_GLOBAL_HEADER
#define AV_CODEC_FLAG_GLOBAL_HEADER CODEC_FLAG_GLOBAL_HEADER
#endif

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}

static void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->st->codec;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", local_av_err2str(ret));
        exit(1);
    }

    /* init signal generator */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

#ifndef AV_CODEC_CAP_VARIABLE_FRAME_SIZE
#define AV_CODEC_CAP_VARIABLE_FRAME_SIZE CODEC_CAP_VARIABLE_FRAME_SIZE
#endif

    if(c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    // allocid: MainWindowFrame01
    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    av_log(NULL, AV_LOG_INFO, "ALLOC new: MainWindowFrame01 ost->frame = %p\n",
           ost->frame);
    // allocid: MainWindowFrame02
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);
    av_log(NULL, AV_LOG_INFO,
           "ALLOC new: MainWindowFrame02 ost->tmp_frame = %p\n",
           ost->frame);

    /* create resampler context */
    // allocid: MainWindowSWR01
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        exit(1);
    }
    av_log(NULL, AV_LOG_INFO, "ALLOC new: MainWindowSWR01 ost->swr_ctx = %p\n",
           ost->swr_ctx);

    /* set options */
    av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
    }
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
AVFrame *MainWindow::get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];
    AVRational avr_one;
    avr_one.num = avr_one.den = 1;

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->st->codec->time_base,
                      STREAM_DURATION, avr_one) >= 0)
        return NULL;

    av_log(NULL, AV_LOG_INFO, "Audio generate nb_samples = %d x%d\n",
           frame->nb_samples, ost->st->codec->channels);
    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->st->codec->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int MainWindow::write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVPacket pkt; // data and size must be 0;
    pkt.data = NULL;
    pkt.size = 0;
    AVFrame *frame;
    int ret;
    int got_packet = 0;
    int dst_nb_samples;

    av_init_packet(&pkt);
    c = ost->st->codec;

#if 0
    frame = get_audio_frame(ost);
#else
    av_log(NULL, AV_LOG_INFO, "frame = this->GetAudioFromQueue()\n");
    frame = this->GetAudioFromQueue();
    av_log(NULL, AV_LOG_INFO, "frame = %p\n", frame);
#endif

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        av_log(NULL, AV_LOG_INFO, "dst_nb_samples = av_rescale_rnd(...)\n");
        dst_nb_samples = av_rescale_rnd(
                    swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                    c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        av_log(NULL, AV_LOG_INFO, "av_frame_make_writable(ost->frame)\n");
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
        frame = ost->frame;

        AVRational r;
        r.num = 1;
        r.den = c->sample_rate;

        frame->pts = av_rescale_q(ost->samples_count, r, c->time_base);
        ost->samples_count += dst_nb_samples;

        ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
        if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame: %s\n", local_av_err2str(ret));
            exit(1);
        }

        if (got_packet) {
            ret = write_frame(oc, &c->time_base, ost->st, &pkt);
            if (ret < 0) {
                fprintf(stderr, "Error while writing audio frame: %s\n",
                        local_av_err2str(ret));
                exit(1);
            }
        }
    }

    return (frame || got_packet) ? 0 : 1;
}

/**************************************************************/
/* video output */

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}

static void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->st->codec;
    AVDictionary *opt = NULL;

    av_log(NULL, AV_LOG_INFO, "av_dict_copy(&opt, opt_arg, 0)\n");
    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    av_log(NULL, AV_LOG_INFO, "ret = avcodec_open2(c, codec, &opt)\n");
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", local_av_err2str(ret));
        exit(1);
    }

    /* allocate and init a re-usable frame */
    av_log(NULL, AV_LOG_INFO, "ost->frame = alloc_picture(c->pix_fmt, %d, %d)\n",c->width, c->height);
    // allocid: MainWindowFrame03
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    av_log(NULL, AV_LOG_INFO, "ALLOC new: MainWindowFrame03 ost->frame = %p\n",
           ost->frame);

    /* If the output format is not RGBA, then a temporary RGBA
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if(1 || c->pix_fmt != AV_PIX_FMT_RGBA)
    {
        av_log(NULL, AV_LOG_INFO, "ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, %d, %d)\n",c->width, c->height);
        // allocid: MainWindowFrame04
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
        av_log(NULL, AV_LOG_INFO,
               "ALLOC new: MainWindowFrame04 ost->tmp_frame = %p\n",
               ost->tmp_frame);
    }
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i, ret;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally;
     * make sure we do not overwrite it here
     */
    ret = av_frame_make_writable(pict);
    if (ret < 0)
        exit(1);

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

/* Prepare a dummy image. */
static void fill_rgba_image(AVFrame *pict, int frame_index,
                            int width, int height)
{
    int x, y, i, ret;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally;
     * make sure we do not overwrite it here
     */
    av_log(NULL, AV_LOG_INFO, "av_frame_make_writable(pict)\n");
    ret = av_frame_make_writable(pict);
    if (ret < 0)
        exit(1);

    i = frame_index;

    av_log(NULL, AV_LOG_INFO, "pict->channels: %d\n", pict->channels);
    av_log(NULL, AV_LOG_INFO, "pict->data[0]: %p\n", pict->data[0]);
    av_log(NULL, AV_LOG_INFO, "pict->data[1]: %p\n", pict->data[1]);
    av_log(NULL, AV_LOG_INFO, "pict->data[2]: %p\n", pict->data[2]);
    av_log(NULL, AV_LOG_INFO, "pict->data[3]: %p\n", pict->data[3]);

    av_log(NULL, AV_LOG_INFO, "fill RGBA\n");
    uint8_t *p;
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            p = pict->data[0] + (y * pict->linesize[0] + x*4);
            p[0] = x + y + i * 3;
            p[1] = 255;
            p[2] = 0;
            p[3] = 255;
        }
    }
}


AVFrame *MainWindow::get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->st->codec;

#if 0
    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->st->codec->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    if (1 || c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                        "Could not initialize the conversion context\n");
                exit(1);
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx,
                  (const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
                  0, c->height, ost->frame->data, ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }
    //#elif 0
    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->st->codec->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    if (1 || c->pix_fmt != AV_PIX_FMT_RGBA) {
        /* as we only generate a RGBA picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            av_log(NULL, AV_LOG_INFO, "sws_getContext(...)\n");
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          RGB_PIX_FMT,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                        "Could not initialize the conversion context\n");
                exit(1);
            }
        }
        av_log(NULL, AV_LOG_INFO, "fill_rgba_image(encRGBFrame, ost->next_pts, c->width, c->height);\n");
        fill_rgba_image(encRGBFrame, ost->next_pts, c->width, c->height);
        av_log(NULL, AV_LOG_INFO, "sws_scale(...)\n");
        sws_scale(ost->sws_ctx,
                  (const uint8_t * const *)encRGBFrame->data, encRGBFrame->linesize,
                  0, c->height, ost->frame->data, ost->frame->linesize);
    } else {
        fill_rgba_image(ost->frame, ost->next_pts, c->width, c->height);
    }
#else
    /* check if we have more frames */
    uint8_t *buf;

    // discard the first few frames of video in order to sync to audio
    while(this->encVideoSkip >0)
    {
        av_log(NULL, AV_LOG_INFO, "videoSkip: buf = GetVideoFromQueue()\n");
        buf = GetVideoFromQueue();
        if(buf==NULL) return NULL;
        delete [] buf;
        this->encVideoSkip--;
    }

    av_log(NULL, AV_LOG_INFO, "buf = GetVideoFromQueue()\n");
    buf = GetVideoFromQueue();
    if(buf==NULL) return NULL;

    if(!ost->sws_ctx)
    {
        av_log(NULL, AV_LOG_INFO, "sws_getContext(...)\n");
        // allocid: MainWindowSWS01
        ost->sws_ctx = sws_getContext(c->width, c->height,
                                      RGB_PIX_FMT,
                                      c->width, c->height,
                                      c->pix_fmt,
                                      SCALE_FLAGS, NULL, NULL, NULL);
        if(!ost->sws_ctx)
            throw vfbexception("Could not initialize sws_ctx");
        av_log(NULL, AV_LOG_INFO,
               "ALLOC new: MainWindowSWS01 ost->sws_ctx = %p\n",
               ost->sws_ctx);
    }

    uint8_t **bufHandle = &buf;

    av_frame_make_writable(ost->frame);
    av_log(NULL, AV_LOG_INFO, "sws_scale(...)\n");
    sws_scale(ost->sws_ctx,
              (const uint8_t * const *)bufHandle, encRGBFrame->linesize,
              0, c->height, ost->frame->data, ost->frame->linesize);

    delete [] buf;

#endif

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int MainWindow::write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt;
    pkt.data = NULL;
    pkt.size = 0;

    c = ost->st->codec;

    av_log(NULL, AV_LOG_INFO, "frame = get_video_frame(ost)\n");
    frame = get_video_frame(ost);

    av_log(NULL, AV_LOG_INFO, "av_init_packet(&pkt)\n");
    av_init_packet(&pkt);

    /* encode the image */
    av_log(NULL, AV_LOG_INFO, "ret = avcodec_encode_video2(c, &pkt, frame, &got_packet)\n");
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame: %s\n", local_av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        av_log(NULL, AV_LOG_INFO, "ret = write_frame(oc, &c->time_base, ost->st, &pkt)\n");
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    } else {
        ret = 0;
    }

    if (ret < 0) {
        fprintf(stderr, "Error while writing video frame: %s\n", local_av_err2str(ret));
        exit(1);
    }

    return (frame || got_packet) ? 0 : 1;
}

static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    // allocid: MainWindowStream01
    av_log(NULL, AV_LOG_INFO,
           "ALLOC del: MainWindowStream01 ost->st->codec = %p\n",
           ost->st->codec);
    avcodec_close(ost->st->codec);

    // allocid: MainWindowFrame01
    // allocid: MainWindowFrame03
    av_log(NULL, AV_LOG_INFO, "ALLOC del: MainWindowFrame ost->frame = %p\n",
           ost->frame);
    av_frame_free(&ost->frame);

    // allocid: MainWindowFrame02
    // allocid: MainWindowFrame04
    av_log(NULL, AV_LOG_INFO,
           "ALLOC del: MainWindowFrame ost->tmp_frame = %p\n",
           ost->tmp_frame);
    av_frame_free(&ost->tmp_frame);

    // allocid: MainWindowSWS01
    if(ost->sws_ctx)
    {
        av_log(NULL, AV_LOG_INFO, "ALLOC del: MainWindowSWS01 ost->sws_ctx = %p\n",
               ost->sws_ctx);
        sws_freeContext(ost->sws_ctx);
        ost->sws_ctx = NULL;
    }

    // allocid: MainWindowSWR01
    if(ost->swr_ctx)
    {
        av_log(NULL, AV_LOG_INFO, "ALLOC del: MainWindowSWR01 ost->swr_ctx = %p\n",
               ost->swr_ctx);
        swr_free(&ost->swr_ctx);
    }
}

/**************************************************************/
/* media file output */

int MainWindow::MuxMain(const char *fn_arg, long startFrame, long numFrames,
                        long vidFrameOffset, QProgressDialog &progress)
{
    int argc = 2;
    const char *argv[] = { "mux_main", fn_arg };
    OutputStream video_st, audio_st;
    memset(&video_st, 0, sizeof(OutputStream));
    memset(&audio_st, 0, sizeof(OutputStream));
    const char *filename = NULL;
    AVOutputFormat *fmt = NULL;
    AVFormatContext *oc = NULL;
    AVCodec *audio_codec = NULL;
    AVCodec *video_codec = NULL;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;

    // can only do MuxMain once without risking a crash, so mark it now.
    this->isVideoMuxingRisky = true;

    av_log(NULL, AV_LOG_INFO, "MuxMain(\"%s\", %ld, %ld, %ld, progress)\n",
           fn_arg, startFrame, numFrames, vidFrameOffset);


    if (argc < 2) {
        throw vfbexception("Invalid call to MuxMain");
    }

    // Call only once (I think libav can handle additional calls all right,
    // but just in case it doesn't...)
    static bool needRegisterAll = true;
    if(needRegisterAll)
    {
        /* Initialize libavcodec, and register all codecs and formats. */
        av_log(NULL, AV_LOG_INFO, "av_register_all()\n");

        av_register_all();
        needRegisterAll = false;
    }

    filename = argv[1];
    /*
    if (argc > 3 && !strcmp(argv[2], "-flags")) {
        av_dict_set(&opt, argv[2]+1, argv[3], 0);
    }
    */

    /* allocate the output media context */
    av_log(NULL, AV_LOG_INFO, "avformat_alloc_output_context2(&oc, NULL, NULL, filename)\n");
    // allocid: MainWindowOutput01
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    av_log(NULL, AV_LOG_INFO, "ALLOC new: MainWindowOutput01 oc = %p\n", oc);
    if (!oc)
        return 1;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        int fps_timbase =24;
        //if (ui->filmrate_PD->currentIndex()>1) fps_timbase=25;
        fps_timbase = ui->frameRateSpinBox->value();
        video_st.requestedTimeBase = fps_timbase;

        //video_st.requestedWidth = this->scan.inFile.Width();
        //video_st.requestedHeight = this->scan.inFile.Height();
        video_st.requestedWidth = 640;
        video_st.requestedHeight = 480;

        av_log(NULL, AV_LOG_INFO, "add_stream(&video_st, oc, &video_codec, fmt->video_codec)\n");
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        //int samplerate = (this->ui->filerate_PD->currentIndex()+1)*48000;
        int samplerate = 48000;
        int frameratesamples ;
        int fps_timbase =24;

//        if (ui->filmrate_PD->currentIndex()==0)
//            frameratesamples = (int) (samplerate/23.976);
//        else if (ui->filmrate_PD->currentIndex()==1)
//            frameratesamples = (int) (samplerate/24.0);
//        else
//        {
//            frameratesamples =  (int) (samplerate/25.0);
//            fps_timbase=25;
//        }
        fps_timbase = ui->frameRateSpinBox->value();
        frameratesamples = (int) (samplerate/fps_timbase);
        av_log(NULL, AV_LOG_INFO, "Requested sample rate: %d\n", samplerate);
        audio_st.requestedSamplingRate = samplerate;
        audio_st.requestedSamplesPerFrame = frameratesamples;
        audio_st.requestedTimeBase = fps_timbase;

        av_log(NULL, AV_LOG_INFO, "add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec)\n");
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    /*
    av_log(NULL, AV_LOG_INFO, "this->encRGBFrame = alloc_picture(AV_PIX_FMT_RGBA,%d,%d)\n",
            this->scan.inFile.Width(), this->scan.inFile.Height());
    this->encRGBFrame = alloc_picture(RGB_PIX_FMT,
            this->scan.inFile.Width(), this->scan.inFile.Height());
    */
    this->encRGBFrame = av_frame_alloc();
    if (!this->encRGBFrame) {
        av_log(NULL, AV_LOG_INFO, "Could not allocate RGBFrame picture\n");
        throw vfbexception("Could not allocate RGBFrame picture");
    }
    this->encRGBFrame->format = RGB_PIX_FMT;
    this->encRGBFrame->width = 640;
    this->encRGBFrame->height = 480;
    if(av_frame_get_buffer(this->encRGBFrame, 32) < 0)
        throw vfbexception("Could not allocate encRGBFrame buffer");

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

    av_dump_format(oc, 0, filename, 1);

    av_log(NULL, AV_LOG_INFO, "encS16Frame = audio_st.tmp_frame = %p\n", audio_st.tmp_frame);
    this->encS16Frame = audio_st.tmp_frame;

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    local_av_err2str(ret));
            return 1;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                local_av_err2str(ret));
        return 1;
    }

    this->encCurFrame = 2;
    this->encStartFrame = startFrame;
    this->encNumFrames = numFrames;

    if(vidFrameOffset > 0)
    {
        this->encVideoSkip = vidFrameOffset;
        this->encAudioSkip = 0;
        this->encVideoPad = 0;
        this->encAudioPad = 0;
    }
    else
    {
        this->encVideoSkip = 0;
        this->encAudioSkip = -vidFrameOffset * this->frame_window->samplesperframe_file;
        av_log(NULL, AV_LOG_INFO, "encAudioSkip = %ld\n", this->encAudioSkip);
        this->encVideoPad = 0;
        this->encAudioPad = 0;
    }

    this->frame_window->vo.videobuffer = this->encRGBFrame->data[0];

    this->encVideoBufSize = 640 * 480 * 4;

    // queue up some black frames to sync the video:
    uint8_t *blackFrame;
    for(int i=0; i<this->encVideoPad; ++i)
    {
        const uint8_t black[4] = { 0, 0, 0, 0xFF };
        blackFrame = new uint8_t[this->encVideoBufSize];
        for(long p = 0; p<this->encVideoBufSize; p+=4)
            memcpy(blackFrame+p, black, 4);
        this->encVideoQueue.push(blackFrame);
    }

    while (encode_video || encode_audio) {
        /* select the stream to encode */
        if (encode_video &&
                (!encode_audio || av_compare_ts(video_st.next_pts, video_st.st->codec->time_base,
                                                audio_st.next_pts, audio_st.st->codec->time_base) <= 0)) {
            av_log(NULL, AV_LOG_INFO, "encode_video = !write_video_frame(oc, &video_st)\n");
            encode_video = !write_video_frame(oc, &video_st);
        } else {
            av_log(NULL, AV_LOG_INFO, "encode_audio = !write_audio_frame(oc, &audio_st)\n");
            encode_audio = !write_audio_frame(oc, &audio_st);
        }
        progress.setValue(this->encCurFrame);
        if(progress.wasCanceled())
        {
            this->requestCancel = true;
            throw 2;
        }
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if(have_video)
        close_stream(oc, &video_st);

    this->frame_window->vo.videobuffer = NULL;
    av_frame_free(&this->encRGBFrame);

    if(have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the streams */
    // allocid: MainWindowOutput01
    av_log(NULL, AV_LOG_INFO, "ALLOC del: MainWindowOutput01 oc = %p\n", oc);
    avformat_free_context(oc);
    oc = NULL;

    return 0;
}

#undef STREAM_DURATION
#undef STREAM_PIX_FMT
#undef SCALE_FLAGS

//----------------------------------------------------------------------------
//---------------------- END MUX.C -------------------------------------------
//----------------------------------------------------------------------------
#endif

extern "C" void SegvHandler(int param)
{
    longjmp(segvJumpEnv, 1);
}

bool MainWindow::WriteAudioToFile(const char *fn, const char *videoFn,
                                  long firstFrame, long numFrames)
{
    bool ret;
    return ret;
}

//*********************IMAGE PROCESSING UI ************************************
QString MainWindow::Compute_Timecode_String(int position)
{
    unsigned int sec;
    unsigned int frames;
    int fps_timebase=24;
    int h,m,s,f;
//    if(ui->filmrate_PD->currentIndex()>1)
//        fps_timebase=25;
    fps_timebase = ui->frameRateSpinBox->value();

    QStringList TCL =  this->scan.inFile.TimeCode.split(QRegularExpression("[:]"),Qt::SkipEmptyParts);
    sec =(((TCL[0].toInt() * 3600 )+ (TCL[1].toInt()* 60)+TCL[2].toInt()));
    frames = TCL[3].toInt() +position;
    sec+=frames/fps_timebase;
    frames= frames%fps_timebase;

    h = (sec/3600);
    sec -= h*3600;
    m = sec/60;
    sec -=m*60;
    s = sec +  (frames/fps_timebase);
    f= frames%fps_timebase;

    //return  return QString::number(h,)+":"+QString::number(m)+":" +QString::number(s)+":"+QString::number(f);

    return  QString("%1").arg(h, 2, 10, QChar('0'))+":"+
            QString("%1").arg(m, 2, 10, QChar('0'))+":"+
            QString("%1").arg(s, 2, 10, QChar('0'))+":"+
            QString("%1").arg(f, 2, 10, QChar('0'));
}

uint64_t MainWindow::ComputeTimeReference(int position, int samplingRate)
{
    unsigned int sec;
    unsigned int frames;
    int fps_timebase=24;
    int h,m,s,f;
//    if(ui->filmrate_PD->currentIndex()>1)
//        fps_timebase=25;
    fps_timebase = ui->frameRateSpinBox->value();
    uint64_t reference;

    QStringList TCL =  this->scan.inFile.TimeCode.split(
                QRegularExpression("[:]"),Qt::SkipEmptyParts);
    sec =(((TCL[0].toInt() * 3600 )+ (TCL[1].toInt()* 60)+TCL[2].toInt()));
    frames = TCL[3].toInt() +position;

    sec+=frames/fps_timebase;
    frames= frames%fps_timebase;

    reference = sec;
    reference *= samplingRate;
    reference += uint64_t(
                double(samplingRate)*double(frames)/double(fps_timebase));

    return reference;
}

void MainWindow::on_gammaSlider_valueChanged(int value)
{
    ui->gammaLabel->setText(QString::number(value/100.f,'f',2));
    GPU_Params_Update(1);
}

void MainWindow::on_liftSlider_valueChanged(int value)
{
    ui->liftLabel->setText(QString::number(value/100.f,'f',2));
    GPU_Params_Update(1);
}
void MainWindow::on_gainSlider_valueChanged(int value)
{
    ui->gainLabel->setText(QString::number(value/100.f,'f',2));
    GPU_Params_Update(1);
}

void MainWindow::on_thresholdSlider_valueChanged(int value)
{
    ui->threshLabel->setText(QString::number(value/100.f,'f',2));
    GPU_Params_Update(1);
}
void MainWindow::on_blurSlider_valueChanged(int value)
{
    ui->blurLabel->setText(QString::number(value/100.f,'f',2));
    GPU_Params_Update(1);
}

void MainWindow::on_leftPixSlider_valueChanged(int value)
{
    GPU_Params_Update(1);
}

void MainWindow::on_rightPixSlider_valueChanged(int value)
{
    GPU_Params_Update(1);
}

void MainWindow::on_negBox_clicked()
{
    GPU_Params_Update(1);
}

void MainWindow::on_threshBox_clicked()
{
    GPU_Params_Update(1);
}

void MainWindow::on_FramePitchendSlider_valueChanged(int value)
{
    bool isBlocking = ui->FramePitchendSlider->blockSignals(true);

    ui->framePitchLabel->setText(QString::number(value/1000.f,'f',2));

    // if we're syncing and the other side needs to be updated, let
    // it also handle the GPU_Params_Update call
    if(
        (ui->syncPitchCheckBox->isChecked()) &&
        (ui->framepitchstartSlider->value() != value))
    {
        ui->framepitchstartSlider->setValue(value);
    }
    else
    {
        GPU_Params_Update(1);
    }

    vbscan.overlap_frameend = (value/1000.f);

    (void)ui->FramePitchendSlider->blockSignals(isBlocking);
}

void MainWindow::on_lift_resetButton_clicked()
{
    ui->liftSlider->setValue(0);
}

void MainWindow::on_gamma_resetButton_clicked()
{
    ui->gammaSlider->setValue(100);
}

void MainWindow::on_gain_resetButton_clicked()
{
    ui->gainSlider->setValue(100);
}

void MainWindow::on_thresh_resetButton_clicked()
{
    ui->thresholdSlider->setValue(300);
}

void MainWindow::on_blur_resetButton_clicked()
{
    ui->blurSlider->setValue(0);
}

void MainWindow::on_desatBox_clicked()
{
    GPU_Params_Update(1);
}

void MainWindow::on_OverlapSoundtrackCheckBox_stateChanged(int arg1)
{
    GPU_Params_Update(1);
}

void MainWindow::on_OverlapPixCheckBox_stateChanged(int arg1)
{
    GPU_Params_Update(1);
}

void MainWindow::on_MainWindow_destroyed()
{
    if(frame_window)
    {
        frame_window->close();
        delete frame_window;
    }

}

void MainWindow::on_CalBtn_clicked()
{/*
    if(this->frame_window!=NULL)
    {
        this->ui->CalEnableCB->setChecked(false);
        this->frame_window->cal_enabled=false;
        this->frame_window->is_caling=true;

        QProgressDialog progress("Calibrating...", "Cancel", 0, 50);
        progress.setWindowTitle("Calibrating");
        progress.setMinimumDuration(0);
        bool canceled = false;
        progress.setWindowModality(Qt::WindowModal);
        progress.setValue(0);

        this->frame_window->clear_cal=true;
        GPU_Params_Update(1);

        int rnd;
        int floor = 0;
        int ceiling = this->scan.lastFrameIndex-this->scan.firstFrameIndex;
        int range = (ceiling - floor);

        while(this->frame_window->calval<0.5)
        {
            //srand((unsigned)time(0));

            rnd = floor+int(rand()%range);
            Load_Frame_Texture((rnd));

            progress.setValue(100*this->frame_window->calval);
            if(progress.wasCanceled())
            {
                canceled = true;
                break;
            }
        }
        progress.setValue(50); // auto hides the window

        this->frame_window->is_caling=false;
        if(!canceled)
        {
            this->ui->CalEnableCB->setEnabled(true);
            this->ui->CalEnableCB->setChecked(true);
            this->frame_window->cal_enabled=true;
        }
        GPU_Params_Update(1);
    }
    */
}

void MainWindow::on_CalEnableCB_clicked()
{
    if (this->ui->CalEnableCB->isChecked())
    {
        this->frame_window->cal_enabled=true;
    }
    else
    {
        this->frame_window->cal_enabled=false;
    }
    GPU_Params_Update(1);
}

void MainWindow::on_actionAcknowledgements_triggered()
{
    QDesktopServices::openUrl(QUrl("http://www.mirc-rc.usccreate.org"));
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(NULL,
                             QString("Virtual Bench Beta ")+QString(APP_VERSION_STR),
                             "Virtual Bench is an open-source software that enables human and "
                             "machine annotation of scanned motion-picture films. "
                             "\n\n"
                             "Virtual Bench is prdocued at at the University of South Carolina by a team comprised "
                             "of faculty and staff from the University Libraries' Moving "
                             "Image Research Collections (MIRC), Research Computing, and the College of Engineering "
                             "and Computing's Computer Vision Lab, with "
                             "contributions from Thomas Aschenbach (Video & Film Solutions). "
                             "\n\n"
                             "Project funding comes from the Preservation and Access Division "
                             "of the National Endowment for the Humanities. Virtual Bench is "
                             "available through an open-source licensing agreement. The "
                             "complete terms are available in the Virtual Bench Documentation."
                             "\n\n"
                             "This software uses libraries from the FFmpeg project under "
                             "the GPLv2.0."
                             );
}


void MainWindow::DeleteTempSoundFile(void)
{
    try
    {
        ExtractedSound sample;
        ExtractedSound emptySound;
        for(int i=0; i<samplesPlayed.size(); ++i)
        {
            sample = samplesPlayed[i];
            if(sample.sound != NULL)
            {
                sample.sound->stop();
                QFile::remove(sample.sound->source().fileName());//->fileName());
            }
            samplesPlayed[i] = emptySound;
        }
    }
    catch(std::exception &e)
    {
        QMessageBox w;
        QString msg("Error deleting temp file: \n");
        int answer;

        msg += QString(e.what());

        w.setText(msg);
        w.setStandardButtons(QMessageBox::Abort | QMessageBox::Ok);
        w.setDefaultButton(QMessageBox::Ok);
        answer = w.exec();

        if(answer == QMessageBox::Abort) exit(1);
        return;
    }
}

//----------------------------------------------------------------------------
QTextStream &MainWindow::Log()
{
#ifdef Q_OS_WINDOWS
    static QFile logfile(QString("%1/%2").arg(QDir::homePath(),
                                              "VFB-log.txt"));
#else
    static QFile logfile(QString("%1/%2").arg(QDir::homePath(),
                                              ".vfb.log.txt"));
#endif

    if(!log.device() || !log.device()->isOpen())
    {
        logfile.open(QIODevice::WriteOnly);
        log.setDevice(&logfile);
    }

    log.flush();

    return log;
}

void MainWindow::LogSettings()
{
    Log() << "\n----- OpenGL SETTINGS -----\n";
    saveproject(Log());
    Log() << "---------------------------\n";

    if(scan.soundBounds.size() > 0)
    {
        Log() << "\n----- PROJECT SETTINGS -----\n";
        Log() << "FirstFrame: " << scan.firstFrameIndex << "\n";
        Log() << "LastFrame: " << scan.lastFrameIndex << "\n";
        //Log() << "Frames Per Second: " << scan.framesPerSecond << "\n";

        Log() << "Number of soundtracks: " <<
                 this->scan.soundBounds.size() << "\n";

        for(int i=0; i<this->scan.soundBounds.size(); ++i)
            Log() << "Sound bounds: " << (scan.soundBounds[i].Left()) <<
                     " - " << (scan.soundBounds[i].Right()) << "\n";

        Log() << "----------------------------\n";
    }
}

void MainWindow::on_monostereo_PD_currentIndexChanged(int index)
{
    frame_window->stereo = float(index);
}

void MainWindow::on_actionShow_Soundtrack_Only_triggered()
{
    ui->showSoundtrackOnlyCheckBox->setChecked(
                ui->actionShow_Soundtrack_Only->isChecked());
    GPU_Params_Update(1);
}

//void MainWindow::on_actionWaveform_Zoom_triggered()
//{
//    ui->waveformZoomCheckBox->setChecked(ui->actionWaveform_Zoom->isChecked());
//    GPU_Params_Update(1);
//}

void MainWindow::on_actionShow_Overlap_triggered()
{
    ui->showOverlapCheckBox->setChecked(ui->actionShow_Overlap->isChecked());
    GPU_Params_Update(1);
}

//void MainWindow::on_waveformZoomCheckBox_clicked(bool checked)
//{
//    ui->actionWaveform_Zoom->setChecked(checked);
//    GPU_Params_Update(1);
//}

void MainWindow::on_showOverlapCheckBox_clicked(bool checked)
{
    ui->actionShow_Overlap->setChecked(checked);
    GPU_Params_Update(1);
}

void MainWindow::on_showSpliceCheckBox_clicked(bool checked)
{
    //    if (checked)
    //        splice_is_checked=true;
    //    else
    //        splice_is_checked=false;
    //ui->actionShow_Overlap->setChecked(checked);
    GPU_Params_Update(1);
}

void MainWindow::on_showSoundtrackOnlyCheckBox_clicked(bool checked)
{
    ui->actionShow_Soundtrack_Only->setChecked(checked);
    GPU_Params_Update(1);
}

void MainWindow::on_actionOpen_Source_triggered()
{
    on_sourceButton_clicked();
}

void MainWindow::on_actionLoad_Settings_triggered()
{
    on_loadprojectButton_clicked();
}

void MainWindow::on_actionSave_Settings_triggered()
{
    on_saveprojectButton_clicked();
}

void MainWindow::on_actionQuit_triggered()
{
    std::exit(0);
}

void MainWindow::on_frame_numberSpinBox_editingFinished()
{

}


void MainWindow::UpdateQueueWidgets(void)
{

}




void MainWindow::on_soundtrackDefaultsButton_clicked()
{
    SaveDefaultsSoundtrack();
}

void MainWindow::on_imageDefaultsButton_clicked()
{
    SaveDefaultsImage();
}


void MainWindow::on_actionPreferences_triggered()
{
    preferencesdialog *pref = new preferencesdialog(this);
    pref->setWindowTitle("Preferences");
    pref->exec();

    // the dialog itself modifies the app's preferences if accepted,
    // so there's no additional processing to do here.

    delete pref;
}

void MainWindow::on_actionReport_or_track_an_issue_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/MIRC-UofSC/VirtualFilmBench/issues"));
}

void MainWindow::on_OverlapPixCheckBox_clicked(bool checked)
{
    ui->pixLabel->setEnabled(checked);
    ui->leftPixSpinBox->setEnabled(checked);
    ui->rightPixSpinBox->setEnabled(checked);
    ui->leftPixSlider->setEnabled(checked);
    ui->rightPixSlider->setEnabled(checked);
}

void MainWindow::PlaybackStart()
{
    playtimer.start();
}


void MainWindow::on_stop_btn_clicked()
{
    playtimer.stop();
    shuttlespeed=1;
}

void MainWindow::SetPlaybackInterval()
{
    double fps(24);
    double multiplier(1.0);

    const QString& text = ui->slow_play_drop->currentText();

    static  QRegularExpression re("(\\d+)%");
    QRegularExpressionMatch match = re.match(text);

    bool ok(false);
    if(match.hasMatch())
        multiplier = match.captured(1).toInt(&ok) / 100.0;

    if(!ok || multiplier <= 0.0)
    {
        qDebug() << "WARNING: bad playback speed text ignored: " << text;
        multiplier = 1.0;
    }

    fps = ui->frameRateSpinBox->value() * multiplier;
    int interval = int(1000.0/fps);

    playtimer.setInterval(interval);
}

void MainWindow::on_frameR_btn_clicked()
{

    framebackward();
}


void MainWindow::on_frame_F_btn_clicked()
{
    frameforward();
}


void MainWindow::on_frame_shuttleF_btn_clicked()
{
    shuttlespeed=3;
    playtimer.start(44);
}


void MainWindow::on_shuttle_dial_sliderReleased()
{
    ui->shuttle_dial->setValue(0);
    playtimer.stop();
    shuttlespeed=1;
    jumpToFrame(ui->frame_numberSpinBox->value());
    playdir=1;
}


void MainWindow::on_shuttle_dial_sliderMoved(int position)
{

    if (position<5&&  position >-5)
    {
        shuttlespeed=1;
        if (position<0)
            playdir = -1;
        else
            playdir=1;
        if(!playtimer.isActive())
            playtimer.start(400/abs(position));
        else
            playtimer.setInterval(400/abs(position));
    }
    else
    {
        shuttlespeed=position/2;
        if(!playtimer.isActive())
            playtimer.start(44);
        else
            playtimer.setInterval(44);
    }


}


void MainWindow::on_showSoundtrackOnlyCheckBox_stateChanged(int arg1)
{

}


void MainWindow::on_showSoundtrackOnlyCheckBox_clicked()
{

}


void MainWindow::on_exportstrip_btn_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                               "/home/jana/untitled.png",
                               tr("Images (*.png *.xpm *.jpg *.tif)"));
    frame_window->savestripimage(fileName);

}


void MainWindow::OpenEventWindow()
{
    if(!eventsWindow)
    {
        eventsWindow = new eventdialog(this);
        connect(
            eventsWindow,SIGNAL(jump(uint32_t)),
            this,SLOT(jumpToFrame(uint32_t)));
        connect(
            this, SIGNAL(NewFrameLoaded(uint32_t)),
            eventsWindow, SLOT(ScrollToFrame(uint32_t)));
        connect(
            frame_window, SIGNAL(ShortcutCtrlNum(int)),
            eventsWindow, SLOT(ShortcutEventKey(int)));
    }

    eventsWindow->show();
    eventsWindow->raise();
    eventsWindow->activateWindow();
}

void MainWindow::OpenPropertiesWindow()
{
    PropertiesDialog dialog(this, vbscan.Properties());
    if(dialog.exec() != QDialog::Accepted) return;
    vbscan.SetProperties(dialog.Properties());
}

//#############################################################################

MainWindow *MainWindowAncestor(QObject *obj, bool reporting)
{
    MainWindow *mainwindow;

    // Try to find the mainwindow among the ancestors of this dialog
    for(
        QObject *p = obj;
        !(mainwindow = qobject_cast<MainWindow *>(p)) && p->parent();
        p = p->parent())
    {
        ;
    }

    if(!mainwindow && reporting)
    {
        QMessageBox msgError;
        msgError.setText("Internal Error: dialog's parent is not the main window");
        msgError.setIcon(QMessageBox::Critical);
        msgError.setWindowTitle("Internal Error");
        msgError.exec();
    }

    return mainwindow;
}

void MainWindow::on_actionPlay_Stop_triggered()
{
    if(!playtimer.isActive())
        playtimer.start();
    else

    {
        playtimer.stop();
        shuttlespeed=1;
    }
}


void MainWindow::on_play_btn_clicked()
{

}


void MainWindow::on_setZero_button_clicked()
{
    vbscan.zeroframe = currentframe;
    updateBenchCounter();
}

void MainWindow::updateBenchCounter()
{
    int f = currentframe - vbscan.zeroframe;
    QString sign = "";

    if(f<0)
    {
        sign = "-";
        f = -f;
    }

    if (ui->bench_counter_comboBox->currentIndex()==0)
    {
        int framesperfoot = vbscan.FramesPerFoot();

        if((sign=="-"))
        {
            ui->bench_counter_display->setText(
                "-(" +
                QString::number(f/framesperfoot)+
                "+"+
                QString::number(f%framesperfoot).rightJustified(2,'0')+
                ")");

            frame_window->currentframestring=
                "-(" +
                QString::number(f/framesperfoot)+
                "+"+
                QString::number(f%framesperfoot).rightJustified(2,'0')+
                                               ")";
        }
        else
        {
            ui->bench_counter_display->setText(
                QString::number(f/framesperfoot)+
                "+"+
                QString::number(f%framesperfoot).rightJustified(2,'0'));

            frame_window->currentframestring=
                QString::number(f/framesperfoot)+
                "+"+
                                               QString::number(f%framesperfoot).rightJustified(2,'0');
        }

    }

    if (ui->bench_counter_comboBox->currentIndex()==1)
    {
        ui->bench_counter_display->setText(sign+QString::number(f));
        frame_window->currentframestring=sign+QString::number(f);
    }

    if (ui->bench_counter_comboBox->currentIndex()==2)
    {
        ui->bench_counter_display->setText(sign+Compute_Timecode_String(f));
        frame_window->currentframestring=sign+Compute_Timecode_String(f);
    }


}

void MainWindow::on_bench_counter_comboBox_currentIndexChanged(int index)
{
    updateBenchCounter();
}
