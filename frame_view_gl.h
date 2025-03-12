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

#ifndef FRAME_VIEW_GL_H
#define FRAME_VIEW_GL_H

#include "openglwindow.h"
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QtGui/QGuiApplication>
#include <QtGui/QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QtGui/QScreen>
#include <QtCore/qmath.h>
#include <QOpenGLFunctions_3_0>
#include <QOpenGLTexture>
#include <QSurfaceFormat>
#include <QContextMenuEvent>
#include <QCloseEvent>

#include <QList>
#include <QSet>
#include <QPair>
#include <algorithm>

#include "vbevent.h"
#include "frametexture.h"

class FrameBucketManager {
public:
    struct FrameBucket {
        int frame_buffer_id;
        int frame_number;
    };

    FrameBucketManager(int numberOfBuffers);
    void displayCurrentBuckets() const;
    void addFrameNumberToBuffer(int frame_buffer_id, int frame_number);
      int getcurrent();
    QPair<QList<int>, QList<int>> getNeededFrameNumbers(int frame_number) const;
    QList<int> getBuffersSortedByFrameNumber() const;

private:
    QList<FrameBucket> buffers;
    int bufferCount;
};


typedef void (*FrameWindowCallbackFunction)(void *);

class Frame_Window : public OpenGLWindow
{
Q_OBJECT
public:

    Frame_Window(int width, int height);
	~Frame_Window();

	void initialize() Q_DECL_OVERRIDE;
	void render() Q_DECL_OVERRIDE;

    void closeEvent (QCloseEvent *event) Q_DECL_OVERRIDE;

	void CheckGLError(const char *fn, int line);

	typedef struct  {
		int postion;
		float value;
	} overlap_match;

	typedef struct  {
		GLuint video_output_fbo;
		GLuint video_output_texture;
		int width;
		int height;
		uint8_t * videobuffer;
	} video_output;

	void ParamUpdateCallback(FrameWindowCallbackFunction cb, void *userData);

	//load frame from pointer
	void load_frame_texture(FrameTexture *frame);
    int originalwx;
    int originalwy;
	float GetAverage(GLfloat *, int ) ;
	int GetMinLoc(GLfloat*, int ) ;
	void GetBestMatchFromFloatArray(GLfloat*, int , int ,overlap_match &) ;
	float GetMin(GLfloat* dArray, int iSize) ;
	void PrepareRecording(int numsamples) ;
	void ProcessRecording(int numsamples);
	void DestroyRecording( );
	float **GetRecording() const { return FileRealBuffer; }
    void PrepareVideoOutput(FrameTexture *frame)	;
	float GetMax(GLfloat* dArray, int iSize) ;
	void read_frame_texture(FrameTexture *frame);
	float *GetCalibrationMask();
	void SetCalibrationMask(const float *mask);
	void update_parameters();// update gpu variables and rerender

    void savestripimage(QString filename);

    QList< vbevent* > currentevents;
    void PaintOverlay();

	video_output vo;
	// working data
	float * *FileRealBuffer;
    int VBench_dir;
    int VBench_inc;
    int currentframenumber;
    int currentbufferid;
    QString currentframestring;
	// view parameters
	float WFMzoom; // waveform zoom amount
	float calval;
	float o_color;
	float currstart;
	float rendermode;
	float lift,gamma,gain;
	float threshold,blur;
	float stereo;
	bool thresh;
	int input_w;
	int input_h;
	bool trackonly;
	int bestloc;
	int lowloc;
	bool negative;
	float overlap_target; //0=sound 1= picture 2 = both
	bool desaturate;
	bool is_preload;
	bool is_calc;
	bool is_calculating;
	int samplesperframe;
	int samplesperframe_file;
	overlap_match bestmatch;
	overlap_match currmatch;
	overlap_match * match_array;
	bool cal_enabled;
	int cal_points;
	bool is_caling;
	std::vector <float> sound_prev;
	std::vector <float> sound_curr;
	int channels ;
	float* audio_sample_buffer;
	float* audio_compare_buffer;
    GLfloat loupeview[4];

    GLfloat marqueeBounds[4]; // marquee selection boundary, in click order x1,x2,y1,y2

	GLfloat bounds[4]; // boundry of track area 4 elements x1,x2,y1,y2
    GLfloat splice_bounds[4]; // boundry of splice detection 4 elements x1,x2,y1,y2
    int spliceFrameNum;
    int spliceEventNum;
	GLfloat overlap[4]; // used to compute overlap and hold translation numbers:   y1_start, y_size,  translate_Y,window,Y;

    GLfloat rot_angle; // rotation angle

    float * height_avg;
	float h_avg;
	float * match_avf;
	GLfloat pixbounds[2];
	int match_inc;
	int height_inc;
	bool overlapshow;
    bool spliceshow;
	bool is_rendering;
	bool is_debug;
	bool is_videooutput;
	int overrideOverlap;

	float fps;
	unsigned long duration; // milliseconds
	unsigned int bit_depth;
	unsigned int sampling_rate;

	bool clear_cal;

	QTextStream *logger;
	const char **currentOperation;
    int * VBench_framearray ;

    FrameBucketManager * fbm;

signals:
    void ResizedEventBoundingBox(vbevent *event,
                                 float x0, float x1, float y0, float y1);
    void PlayPause();
    void ShortcutCtrlNum(int num);

private:

    int loupeactive=0;
    int shiftactive=0;
	void mouseEvent(QMouseEvent *mouse);
    void keyEvent(QKeyEvent *key);
	void mouseMoveEvent(QMouseEvent *m) Q_DECL_OVERRIDE { mouseEvent(m); };
	void mousePressEvent(QMouseEvent *m) Q_DECL_OVERRIDE { mouseEvent(m); };
	void mouseReleaseEvent(QMouseEvent *m) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *m) Q_DECL_OVERRIDE { keyEvent(m); };
    void keyReleaseEvent(QKeyEvent *m) Q_DECL_OVERRIDE;
    void wheelEvent(QWheelEvent *event) override;
    float distToBounds(float x, float y, const float *bounds) const;
	FrameWindowCallbackFunction paramUpdateCB;
	void *paramUpdateUserData;

	int samplepointer;
	GLuint loadShader(GLenum type, const char *source);
	void gen_tex_bufs(); //generation of textures and buffers
	bool new_frame; //is a new frame from seq

	void CopyFrameBuffer(GLuint fbo, int width, int height);

	GLenum *audio_draw_buffers;
	GLuint audio_pbo;
	GLuint m_posAttr; //vertex buffer
	GLuint m_texAttr;
	GLuint m_matrixUniform; //sizing matrix currently unused
	GLuint m_inputsize_loc;
	GLuint m_rendermode_loc;
	GLuint m_manipcontrol_loc;  //thresh,threshold amount, blur variable
	GLuint m_show_loc;
	GLuint m_overlap_target_loc;
	GLuint m_neg_loc;
	GLuint m_overlap_loc;
	GLuint stereo_loc;
    GLuint marqueebounds_loc;
	GLuint pix_bounds_loc;
    GLuint splice_bounds_loc;
    GLuint loupeview_loc;
	GLuint dminmax_loc;
	GLuint m_colorcontrol_loc;//test uniform varaiable
	GLuint m_bounds_loc; //location of bounds uniform
    GLuint m_rot_angle; // bind for rotation angle
	GLuint m_calcontrol_loc;
	GLuint m_overlapshow_loc;
    GLuint m_spliceshow_loc;
    GLuint texture_index;
	GLuint frame_texture; //image frame texture used as input
    GLuint frame_texture_loc; //image frame texture used as input
	GLuint adj_frame_fbo; //render fbo writes to adj frame texture

	GLuint adj_frame_texture; //render with pixel/image adjustments
    GLuint adj_frame_texture_loc;
    GLuint prev_adj_frame_tex;
    GLuint prev_adj_frame_tex_loc;
	//audio frame buffer object: audio textures attached as draw buffers.
	GLuint audio_fbo;
	GLuint audio_file_fbo;
	GLuint audio_RGB_texture; //render texture audio rgb for screen display
    GLuint audio_RGB_texture_loc;
    GLuint prev_audio_RGB_texture;
	GLuint overlap_compare_audio_texture;
	GLuint overlaps_audio_texture;
	GLuint output_audio_texture;
	GLuint audio_float_texture; //render texture audio floating point
	GLuint audio_int_texture; //render texture audio integer tbd-16bit or 32bit?
	GLuint cal_audio_texture;

    GLuint prev_audio_RGB_texture_loc;
    GLuint overlap_compare_audio_texture_loc;
    GLuint overlaps_audio_texture_loc;
    GLuint output_audio_texture_loc;
    GLuint audio_float_texture_loc; //render texture audio floating point
    GLuint audio_int_texture_loc; //render texture audio integer tbd-16bit or 32bit?
    GLuint cal_audio_texture_loc;

    GLuint overlay_texture; //render texture audio rgb for screen display
    GLuint overlay_texture_loc;



    int VBench_currentindex=0;
    GLuint VBench_fbo;
    GLuint * VBench_texture;
    GLuint VBench_texture_loc;


    GLuint VBench_Strip_fbo;
    GLuint  VBench_Strip_texture;
    GLuint VBench_Strip_texture_loc;
    int strip_width;
    int strip_height;

    GLfloat *verticesTex;
    GLfloat *verticesTexJitter;
    GLfloat *indices;

    QOpenGLBuffer m_tertexBuffer;
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLVertexArrayObject m_vao;

	QOpenGLShaderProgram *m_program; //gpu opengl executable

	int m_frame;


};

#endif
