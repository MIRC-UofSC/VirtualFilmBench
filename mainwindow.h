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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>

#include <QDate>
#include <QMap>
#include <QCloseEvent>
#include <QProgressDialog>
#include <vector>
#include <QtXml>
#include <queue>
#include "QtCore/qtimer.h"
#include "frame_view_gl.h"
#include "eventdialog.h"
#include "project.h"
#include "metadata.h"
#include <QSoundEffect>
#include "vbproject.h"
#define USE_MUX_HACK
#include <QImage>
#include <QColor>


namespace Ui {
class MainWindow;
}

#define FPS_NTSC 0
#define FPS_24 1
#define FPS_25 2
#define FPS_FILM FPS_24
#define FPS_PAL FPS_25


#define EXTRACT_TIMER 0x01
#define EXTRACT_LOG 0x02
#define EXTRACT_NOTIFY 0x04

//struct EventInfo
//{
//    std::string event_name;
//    unsigned short event_type;
//    float event_conf;
//    QList<float> xywh;

//};

class ExtractedSound
{
public:
	uint32_t frameIn;
	uint32_t frameOut;
	uint16_t bounds[2]; // left, right pixel columns
	uint16_t pixBounds[2]; // left, right pixel columns
	uint16_t framePitch[2]; // top, bottom pixel rows
	uint16_t gamma; // [0,500]
	uint16_t gain; // [0,500]
	uint16_t sCurve; // [0,600]
	uint8_t overlap; // as a percentage of height [0,100]
	int8_t lift; // [-100,100]
	int8_t blur; // [-100,100]
	uint8_t fpsType;
	bool useBounds;
	bool usePixBounds;
	bool useSCurve;
	bool makeNegative;
	bool makeGray;

    QSoundEffect *sound;
	int err;

	ExtractedSound();
	~ExtractedSound();
	bool operator ==(const ExtractedSound &ref) const;
	operator bool() const { return (err==0); };
};

class ExtractTask
{
public:
	ExtractedSound params;
	QString source;
	SourceFormat srcFormat;
	QString output;
	MetaData meta;
};

#ifdef USE_MUX_HACK
//----------------------------------------------------------------------------
//---------------------- MUX.C -----------------------------------------------
//----------------------------------------------------------------------------

// a wrapper around a single output AVStream
class OutputStream {
public:
	int requestedWidth;
	int requestedHeight;
	int requestedSamplingRate;
	int requestedSamplesPerFrame;
	int requestedTimeBase;

	AVStream *st;

	/* pts of the next frame that will be generated */
	int64_t next_pts;
	int samples_count;

	AVFrame *frame;
	AVFrame *tmp_frame;

	float t, tincr, tincr2;

	struct SwsContext *sws_ctx;
	struct SwrContext *swr_ctx;
};

#endif

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
	void Render_Frame();
	Project scan;
    vbproject vbscan;
    FrameTexture blankframe;
    int leadingframe;
    int trailingframe;
    int framemmovedirection; // 1 fwd,-1rev,0switch
    int shuttlespeed=1;
    int playdir=1;
    QTimer playtimer;
	std::vector< ExtractedSound > samplesPlayed;
	std::vector< ExtractTask > extractQueue;
	int adminWidth;

	void GUI_Params_Update();
	static void GUI_Params_Update_Static(void *userData)
	{ static_cast<MainWindow *>(userData)->GUI_Params_Update(); }

	ExtractedSound ExtractionParamsFromGUI();
	void ExtractionParametersToGUI(const ExtractedSound &params);
	void PlaySample(int index);

signals:
    void NewFrameLoaded(uint32_t frameNum);
public  slots:
    void jumpToFrame(uint32_t frameNum);
    void frameforward();
    void framebackward();
    void TogglePlayPause();
    void LicenseAgreement();
    void OpenStartingProject();

public:
    void RequestOpenProject(QString fn) { OpenProject(fn); }

private:
	bool WriteAudioToFile(const char *fn, const char *videoFn,
			long firstFrame, long numFrames);
	void DeleteTempSoundFile(void);
	bool saveproject(QString);
	bool saveproject(QTextStream &stream);

	void SaveDefaultsSoundtrack();
	void SaveDefaultsImage();
	void SaveDefaultsAudio();
	void LoadDefaults();

	bool LoadProjectSource(QString fn);
	bool LoadProjectSettings(QString fn);
	bool OpenProject(QString fn);
	bool NewSource(QString fn, SourceFormat ft=SOURCE_UNKNOWN);
	bool Load_Frame_Texture(int);
	void GPU_Params_Update(bool renderyes);
	void UpdateQueueWidgets(void);
	QString Compute_Timecode_String(int position);
	uint64_t ComputeTimeReference(int position, int samplingRate);

private slots:
    void UpdateView();
    void on_sourceButton_clicked();
    //void on_rotateButton_clicked();
    void on_degreeSpinBox_valueChanged(double arg1);
    void updateBenchCounter();
    void on_frameRateSpinBox_valueChanged(int arg1);

	void on_leftSpinBox_valueChanged(int arg1);
	void on_rightSpinBox_valueChanged(int arg1);
	void on_MainWindow_destroyed();
	void on_leftSlider_sliderMoved(int position);
	void on_rightSlider_sliderMoved(int position);
	void on_playSlider_sliderMoved(int position);

	void on_gammaSlider_valueChanged(int value);
	void on_liftSlider_valueChanged(int value);
	void on_gainSlider_valueChanged(int value);
	void on_lift_resetButton_clicked();
	void on_gamma_resetButton_clicked();
	void on_gain_resetButton_clicked();
	void on_thresh_resetButton_clicked();
	void on_blur_resetButton_clicked();
	void on_thresholdSlider_valueChanged(int value);
	void on_blurSlider_valueChanged(int value);
	void on_negBox_clicked();
	void on_threshBox_clicked();
	void on_desatBox_clicked();
	void on_overlapSlider_valueChanged(int value);
	void on_framepitchstartSlider_valueChanged(int value);
    void SyncPitch(int state=Qt::Checked);
	void on_frame_numberSpinBox_valueChanged(int arg1);

private:
    bool extractGL(QString filename, bool doTimer);
    bool extractGL(bool doTimer) { return extractGL(QString(), doTimer); }
//	ExtractedSound Extract(QString filename, QString videoFilename,
//			long firstFrame, long numFrames, uint8_t flags=0);

private slots:
#ifdef USE_SERIAL
	void on_extractSerialButton_clicked();
	void extractSerial(bool doTimer);
#endif // USE_SERIAL
    void playslot();
	void on_HeightCalculateBtn_clicked();
	void on_FramePitchendSlider_valueChanged(int value);
	void on_CalBtn_clicked();
	void on_CalEnableCB_clicked();
	void on_actionAcknowledgements_triggered();
	void on_actionAbout_triggered();
	void on_saveprojectButton_clicked();
	void on_loadprojectButton_clicked();
	void on_leftPixSlider_valueChanged(int value);
	void on_rightPixSlider_valueChanged(int value);
	void on_leftPixSlider_sliderMoved(int position);
	void on_rightPixSlider_sliderMoved(int position);
	void on_leftPixSpinBox_valueChanged(int arg1);
	void on_rightPixSpinBox_valueChanged(int arg1);
	void on_monostereo_PD_currentIndexChanged(int index);
	void on_OverlapSoundtrackCheckBox_stateChanged(int arg1);
	void on_OverlapPixCheckBox_stateChanged(int arg1);
	void on_actionShow_Soundtrack_Only_triggered();
    //void on_actionWaveform_Zoom_triggered();
	void on_actionShow_Overlap_triggered();
    //void on_waveformZoomCheckBox_clicked(bool checked);
	void on_showOverlapCheckBox_clicked(bool checked);
    void on_showSpliceCheckBox_clicked(bool checked);
	void on_showSoundtrackOnlyCheckBox_clicked(bool checked);
	void on_actionOpen_Source_triggered();
	void on_actionLoad_Settings_triggered();
	void on_actionSave_Settings_triggered();
	void on_actionQuit_triggered();
	void on_frame_numberSpinBox_editingFinished();

	void on_soundtrackDefaultsButton_clicked();

	void on_imageDefaultsButton_clicked();

	void on_actionPreferences_triggered();

	void on_actionReport_or_track_an_issue_triggered();

	void on_OverlapPixCheckBox_clicked(bool checked);

    void PlaybackStart();

    void on_stop_btn_clicked();

    void on_frameR_btn_clicked();

    void on_frame_F_btn_clicked();

    void on_frame_shuttleF_btn_clicked();

    void SetPlaybackInterval();

    void on_shuttle_dial_sliderReleased();

    void on_shuttle_dial_sliderMoved(int position);

    void on_showSoundtrackOnlyCheckBox_stateChanged(int arg1);

    void on_showSoundtrackOnlyCheckBox_clicked();

    void on_exportstrip_btn_clicked();

    void OpenEventWindow();
    void OpenPropertiesWindow();

    void on_actionPlay_Stop_triggered();

    void on_play_btn_clicked();

    void on_setZero_button_clicked();

    void on_bench_counter_comboBox_currentIndexChanged(int index);

private:
	QString startingProjectFilename;
	Ui::MainWindow *ui;
    Frame_Window * frame_window;
    eventdialog *eventsWindow;
	QTextStream log;
	bool paramCopyLock;
	bool requestCancel;
	QString prevProjectDir;
	QString prevExportDir;
	MetaData *currentMeta;
	FrameTexture *currentFrameTexture;
	FrameTexture *outputFrameTexture;
	bool isVideoMuxingRisky;
    int currentframe = 0;
    //QDomDocument xml;
    //QMap<int, EventInfo> map;
    //bool splice_is_checked;

public:
    inline const QString ProjectDir() const { return prevProjectDir; }
    Frame_Window *GLWindow() { return frame_window; }
	void SetStartingProject(QString fn) { startingProjectFilename = fn; };
	int MaxFrequency() const;
	ExtractedSound ExtractionParameters();
	QString Version();
	QTextStream &Log();
	void LogSettings();
    void loadfromrandomaccess(int position);
    uint32_t CurrentFrame() const { return currentframe; }
    float MarqueeX0() const;
    float MarqueeX1() const;
    float MarqueeY0() const;
    float MarqueeY1() const;
    void MarqueeClear();

	void LogClose()
		{ if(log.device() && log.device()->isOpen()) log.device()->close(); }

#ifdef USE_MUX_HACK
private:

	long encStartFrame;
	long encNumFrames;
	long encCurFrame;
	long encVideoSkip; // discard scanned video frames from start
	long encAudioSkip; // discard extracted audio frame snippets from start
	long encVideoPad;  // add synthetic (black) video frames at start
	long encAudioPad;  // add synthetic (silent) audio frame snippets at start

	long encVideoBufSize;

	AVFrame *encRGBFrame;
	AVFrame *encS16Frame;

	std::queue< uint8_t* > encVideoQueue;
	int64_t encAudioLen; // total number of samples rendered so far
	int64_t encAudioNextPts;

	bool EnqueueNextFrame();
	uint8_t *GetVideoFromQueue();
	AVFrame *GetAudioFromQueue();

	AVFrame *get_audio_frame(OutputStream *ost);
	int write_audio_frame(AVFormatContext *oc, OutputStream *ost);

	AVFrame *get_video_frame(OutputStream *ost);
	int write_video_frame(AVFormatContext *oc, OutputStream *ost);

	int MuxMain(const char *fn_arg, long firstFrame, long numFrames,
			long vidFrameOffset, QProgressDialog &progress);

#endif

};

MainWindow *MainWindowAncestor(QObject *obj, bool reporting=true);

#endif // MAINWINDOW_H
