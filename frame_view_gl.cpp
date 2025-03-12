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

#include "frame_view_gl.h"
#include "QtOpenGL/qopenglpaintdevice.h"

#include <iostream>
#include <fstream>
#include <string>
#include <QDebug>
#include <QString>
#include <QResource>
#include <QMessageBox>
#include <QApplication>
#include <QWidget>
#include <QMouseEvent>
#include <QCursor>
#include <QContextMenuEvent>
//#include <qopengl.h>
#include <math.h>
#include <stdlib.h>
#include <QPainter>
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunsequenced"
#endif

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

#include "vfbexception.h"

#define PI 3.14159265358979323846
#define VBench_numbuckets 5
#define FRAME_Y_RATIO (0.75)

const char *gluErrorString(GLenum glerror)
{
    switch(glerror)
    {
    case GL_NO_ERROR: return "No error";
    case GL_INVALID_ENUM: return "Invalid Enumeration";
    case GL_INVALID_VALUE: return "Invalid Value";
    case GL_INVALID_OPERATION: return "Invalid Operation";
    case GL_STACK_OVERFLOW: return "Stack Overflow";
    case GL_STACK_UNDERFLOW: return "Stack Underflow";
    case GL_OUT_OF_MEMORY: return "Out of Memory";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "Invalid Framebuffer Operation";
    default: return "Unknown GL Error";
    }
}
void drawCenteredText(QPainter &painter, const QString &text, const QFont &font, int x, int y) {
    painter.setFont(font);

    // Calculate text dimensions
    QFontMetrics metrics(font);
    QRect textRect = metrics.boundingRect(text);

    // Calculate centered position
    int textX = x - textRect.width() / 2;
    int textY = y - textRect.height() / 2;

    // Draw text
    painter.drawText(textX, textY + metrics.ascent(), text);
}
FrameBucketManager::FrameBucketManager(int numberOfBuffers) : bufferCount(numberOfBuffers) {
    for (int i = 0; i < numberOfBuffers; ++i) {
        buffers.append({i, -10}); // Initialize with default frame_number = 0
    }
}

void FrameBucketManager::addFrameNumberToBuffer(int frame_buffer_id, int frame_number) {
    if (frame_buffer_id >= 0 && frame_buffer_id < bufferCount) {
        buffers[frame_buffer_id].frame_number = frame_number;
    }
    // Error handling if frame_buffer_id is out of range
}
int FrameBucketManager::getcurrent() {
    QList<FrameBucket> sortedBuffers = buffers;
    std::sort(sortedBuffers.begin(), sortedBuffers.end(), [](const FrameBucket &a, const FrameBucket &b) {
        return a.frame_number < b.frame_number;
    });

    return sortedBuffers[sortedBuffers.length()/2].frame_number;
}
void FrameBucketManager::displayCurrentBuckets() const {
    qDebug() << "Current Buckets State:";
    for (const auto &bucket : buffers) {
        qDebug() << "Buffer ID:" << bucket.frame_buffer_id << ", Frame Number:" << bucket.frame_number;
    }
}
QPair<QList<int>, QList<int>> FrameBucketManager::getNeededFrameNumbers(int frame_number) const {
    QList<int> neededFrameNumbers;
    QList<int> availableBufferIDs;
    int halfB = bufferCount / 2;

    // Generate a set of all possible frame numbers in the range
    QSet<int> allPossibleFrameNumbers;
    for (int offset = -halfB; offset <= halfB; ++offset) {
        //if (offset == 0) continue; // Skip the current frame number
        allPossibleFrameNumbers.insert(frame_number + offset);
    }

    // Check each possible frame number if it's present in any buffer
    for (int possibleFrame : allPossibleFrameNumbers) {
        bool isPresent = std::any_of(buffers.begin(), buffers.end(),
                                     [possibleFrame](const FrameBucket& buffer) {
                                         return buffer.frame_number == possibleFrame;
                                     });

        if (!isPresent) {
            neededFrameNumbers.append(possibleFrame);
        }
    }

    // Sort neededFrameNumbers in ascending order
    std::sort(neededFrameNumbers.begin(), neededFrameNumbers.end());

    // Find buffer IDs for frames not in the needed set
    for (const auto &buffer : buffers) {
        if (!allPossibleFrameNumbers.contains(buffer.frame_number)) {
            availableBufferIDs.append(buffer.frame_buffer_id);
        }
    }

    return qMakePair(neededFrameNumbers, availableBufferIDs);
}


QList<int> FrameBucketManager::getBuffersSortedByFrameNumber() const {
    QList<FrameBucket> sortedBuffers = buffers;
    std::sort(sortedBuffers.begin(), sortedBuffers.end(), [](const FrameBucket &a, const FrameBucket &b) {
        return a.frame_number < b.frame_number;
    });

    QList<int> sortedBufferIDs;
    for (const auto &buffer : sortedBuffers) {
        sortedBufferIDs.append(buffer.frame_buffer_id);
    }
    return sortedBufferIDs;
}

void Frame_Window::CheckGLError(const char *fn, int line)
{
    QMessageBox w;
    GLenum glerror;
    QString msg("");
    int answer;
    static bool show = true;

    #ifndef Q_OS_WINDOWS
    while((glerror = glGetError()) != GL_NO_ERROR)
    {
        msg += QString("GL Error in %1 at line %2:\n%3\n").arg(
                    QString(fn), QString::number(line),
                    QString(gluErrorString(glerror)));
    }

    if(msg.length() > 0)
    {
        if(this->logger)
        {
            (*(this->logger)) << msg << "\n";
        }


        #ifdef QT_DEBUG

        if(show)
        {
            msg += "\nContinue?\n";
            w.setText(msg);
            w.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll |
                                 QMessageBox::No);

            w.setDefaultButton(QMessageBox::Yes);
            answer = w.exec();

            if(answer == QMessageBox::No) exit(1);
            if(answer == QMessageBox::YesToAll) show = false;
        }

        #endif

    }
    #endif
}

#ifdef QT_DEBUG
#define CHECK_GL_ERROR(f,l) CheckGLError(f,l)
#else
#define CHECK_GL_ERROR(f,l) {}
#endif

const char *gluFBOString(GLenum fbos)
{
    switch(fbos)
    {
    case(GL_FRAMEBUFFER_COMPLETE): return "frambuffer complete.";
    case(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT):
        return "framebuffer attachment points are framebuffer incomplete.";
    case(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT):
        return "framebuffer does not have at least one image attached to it.";
    case(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER):
        return "incomplete draw buffer.";
    case(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER):
        return "incomplete read buffer.";
    case(GL_FRAMEBUFFER_UNSUPPORTED):
        return "framebuffer unsupported.";
    case(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE):
        return "framebuffer incomplete multisample.";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "framebuffer incomplete layer targets.";
    default: return "Unknown GL framebuffer Error";
    }
}

void CheckFrameBuffer(GLenum fbo, const char *fn, int line)
{
    QMessageBox w;
    GLenum glerror;
    QString msg("");
    int answer;
    static bool show = true;

    GLenum fboErr;

#ifdef __APPLE__
    if((fboErr = glCheckFramebufferStatus(fbo)) != GL_FRAMEBUFFER_COMPLETE)
    {
        msg += QString("GL FBO Error in %1 at line %2:\n%3\n").arg(
                    QString(fn), QString::number(line),
                    QString(gluFBOString(fboErr)));

    }

    if(msg.length() > 0)
    {
        /*
        if(this->logger)
        {
            (*(this->logger)) << msg << "\n";
        }
        */

        if(show)
        {
            msg += "\nContinue?\n";
            w.setText(msg);
            w.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll |
                                 QMessageBox::No);
            w.setDefaultButton(QMessageBox::Yes);
            answer = w.exec();

            if(answer == QMessageBox::No) exit(1);
            if(answer == QMessageBox::YesToAll) show = false;
        }
    }
#endif
}

#ifdef QT_DEBUG
#define CHECK_GL_FBO(fbo,f,l) CheckFrameBuffer(fbo,f,l)
#else
#define CHECK_GL_FBO(fbo,f,l) {}
#endif

#define CUR_OP(s) { if(currentOperation) (*currentOperation) = s; }

Frame_Window::Frame_Window(int w,int h)
    : m_program(0)
    , m_frame(0)

{
    // working data
    FileRealBuffer = NULL;

    // view parameters
    WFMzoom=1.0f;

    calval=0;

    o_color=0;
    currstart=0;
    rendermode=0;

    lift=0;
    gamma=1.0f;
    gain=1.0f;
    threshold = 0.5;
    blur = 0;
    stereo = 0;
    thresh=false;

    input_w=w;
    input_h=h;

    trackonly = false;

    bestloc = 0;
    lowloc = 0;

    negative=false;

    overlap_target = 2; // use both sound and picture

    desaturate = false;

    is_preload = false;

    is_calc = false;
    is_calculating=false;

    samplesperframe=2000;
    samplesperframe_file =2000;

    bestmatch.postion = 0;
    bestmatch.value = 0;
    currmatch.postion = 0;
    currmatch.value = 0;
    match_array = new overlap_match[5];

    cal_enabled=false;
    cal_points=2000;
    is_caling=false;

    sound_prev.resize(2000);
    sound_curr.resize(2000);

    channels =2;
    audio_sample_buffer= new float [2*4095*channels];
    audio_compare_buffer= new float [2*samplesperframe*8];

    marqueeBounds[0] = marqueeBounds[1] = marqueeBounds[2] = marqueeBounds[3] = 0;
    splice_bounds [0] = splice_bounds[1] = splice_bounds[2] = splice_bounds[3] = 0;
    loupeview [0] =0.5;
    loupeview[1] =0;
    loupeview[2] =0;
        loupeview[3] = 0;

    bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0;
    overlap[0] = overlap[1] = overlap[2] = overlap[3] = 0;

    height_avg = new float[50];
    h_avg = 0;
    match_avf= new float[5];

    pixbounds[0] = pixbounds[1] = 0;

    rot_angle = 0.0;

    match_inc=0;
    height_inc= 0;

    spliceshow=false;

    overlapshow=false;
    is_rendering =false;
    is_debug = false;
    overrideOverlap = 0;

    fps = 24.0;
    duration = 0; // milliseconds
    bit_depth = 16;
    sampling_rate = 48000;

    clear_cal = false;

    logger = NULL;
    currentOperation = NULL;

    // private:
    paramUpdateCB = NULL;
    paramUpdateUserData = NULL;

    samplepointer = 0;

    new_frame = false;

    audio_draw_buffers = NULL;
    audio_pbo = 0;

    m_posAttr = 0;
    m_texAttr = 0;
    m_matrixUniform = 0;
    m_inputsize_loc = 0;
    m_rendermode_loc = 0;
    m_manipcontrol_loc = 0;
    m_show_loc = 0;
    m_overlap_target_loc = 0;
    m_neg_loc = 0;
    m_overlap_loc = 0;
    stereo_loc = 0;
    marqueebounds_loc = 0;
    pix_bounds_loc = 0;

    splice_bounds_loc = 0;
    dminmax_loc = 0;
    m_colorcontrol_loc = 0;
    m_bounds_loc = 0;
    m_rot_angle = 0;
    m_calcontrol_loc = 0;
    m_overlapshow_loc = 0;
    m_spliceshow_loc = 0;

    frame_texture = 0;
    adj_frame_fbo = 0;
    adj_frame_texture = 0;
    prev_adj_frame_tex = 0;
    audio_fbo = 0;
    audio_file_fbo = 0;
    audio_RGB_texture = 0;
    prev_audio_RGB_texture = 0;
    overlap_compare_audio_texture = 0;
    overlaps_audio_texture = 0;
    output_audio_texture = 0;
    audio_float_texture = 0;
    audio_int_texture = 0;
    cal_audio_texture = 0;
    vo.videobuffer=NULL;
    is_videooutput=0;
    fbm = new FrameBucketManager(VBench_numbuckets);

}

Frame_Window::~Frame_Window()
{
    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("Deleting frame_texture");
    glDeleteTextures(1,&frame_texture);

    CUR_OP("Deleting adj_frame_fbo");
    //glIsFramebuffer returns true, but glDeleteFrameBuffers crashes.
    // Commenting this out to avoid the crash. It seems to not consume the
    // buffer -- the same value (1) is re-used for adj_frame_fbo on my
    // system (OSX), so itmust be getting deleted somewhere else.
    //if(glIsFramebuffer(adj_frame_fbo))
    //		glDeleteFramebuffers( 1, &adj_frame_fbo);

    CUR_OP("Deleting prev_adj_frame_tex");
    glDeleteTextures(1,&prev_adj_frame_tex);

    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("Deleting audio_fbo");
    //glDeleteFramebuffers( 1, &audio_fbo);
    CUR_OP("Deleting audio_file_fbo");
    //glDeleteFramebuffers(1,&audio_file_fbo);

    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("Deleting audio_RGB_texture");
    glDeleteTextures(1,&audio_RGB_texture);
    //glDeleteTextures(1,&audio_float_texture);
    CUR_OP("Deleting prev_audio_RGB_texture");
    glDeleteTextures(1,&prev_audio_RGB_texture);
    //glDeleteTextures(1,&audio_int_texture);
    CUR_OP("Deleting overlaps_audio_texture");
    glDeleteTextures(1,&overlaps_audio_texture);
    CUR_OP("Deleting overlap_compare_audio_texture");
    glDeleteTextures(1,&overlap_compare_audio_texture);

    CUR_OP("Deleting cal_audio_texture");
    glDeleteTextures(1,&cal_audio_texture);
    CUR_OP("Deleting output_audio_texture");
    glDeleteTextures(1,&output_audio_texture);

    CHECK_GL_ERROR(__FILE__,__LINE__);

    if(audio_sample_buffer)
    {
        CUR_OP("Deleting audio_sample_buffer");
        delete [] audio_sample_buffer;
    }

    if(audio_compare_buffer)
    {
        CUR_OP("Deleting audio_compare_buffer");
        delete [] audio_compare_buffer;
    }

    if(height_avg)
    {
        CUR_OP("Deleting height_avg");
        delete [] height_avg;
    }

    if(match_avf)
    {
        CUR_OP("Deleting match_avf");
        delete [] match_avf;
    }

    if(match_array)
    {
        CUR_OP("Deleting match_array");
        delete [] match_array;
    }

    if(audio_draw_buffers)
    {
        CUR_OP("audio_draw_buffers");
        delete [] audio_draw_buffers;
    }

    if(m_program)
    {
        CUR_OP("Deleting m_program");
        delete m_program;
    }
    CUR_OP("");
}

void Frame_Window::closeEvent (QCloseEvent *event)
{
    // Try to find the main window and close it instead
    // It will verify "close all windows?" and exit
    const QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget *widget : topLevelWidgets)
    {
        if(widget->isWindow() && !widget->parent())
        {
            event->ignore();
            widget->close();
            return;
        }
    }
    event->accept();
}

GLuint Frame_Window::loadShader(GLenum type, const char *source)
{
    CHECK_GL_ERROR(__FILE__,__LINE__);
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    CHECK_GL_ERROR(__FILE__,__LINE__);
    return shader;
}

void Frame_Window::initialize()
{
	initializeOpenGLFunctions();
	m_program = new QOpenGLShaderProgram(this);
	m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,
			":/Shaders/vert_shader.vert");
	m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,
			":/Shaders/frag_shader.frag");


    VBench_framearray = new int[VBench_numbuckets];
    m_program->link();
    m_overlapshow_loc = m_program->uniformLocation("overlapshow");
    m_spliceshow_loc = m_program->uniformLocation("spliceshow");
    pix_bounds_loc = m_program->uniformLocation("pix_boundry");
    marqueebounds_loc = m_program->uniformLocation("marquee_boundary");
    splice_bounds_loc = m_program->uniformLocation("splice_boundry");

    m_colorcontrol_loc = m_program->uniformLocation("color_controls");
    m_manipcontrol_loc = m_program->uniformLocation("manip_controls");
    m_calcontrol_loc = m_program->uniformLocation("cal_controls");
    m_inputsize_loc = m_program->uniformLocation("inputsize");
    m_overlap_target_loc = m_program->uniformLocation("overlap_target");
    m_show_loc = m_program->uniformLocation("show_mode");
    m_posAttr = m_program->attributeLocation("posAttr");
    m_texAttr = m_program->attributeLocation("texCoord");
    m_matrixUniform = m_program->uniformLocation("matrix");
    m_bounds_loc = m_program->uniformLocation("bounds");

    m_rot_angle= m_program->uniformLocation("rot_angle");

    m_neg_loc = m_program->uniformLocation("negative");
    stereo_loc=  m_program->uniformLocation("isstereo");
    dminmax_loc = m_program->uniformLocation("dminmax");
    m_rendermode_loc = m_program->uniformLocation("render_mode");
    m_overlap_loc = m_program->uniformLocation("overlap");

    loupeview_loc = m_program->uniformLocation("loupeview");

    CHECK_GL_ERROR(__FILE__,__LINE__);

    gen_tex_bufs(); //call for all textures and buffers to be created
    overlap[0]=0;
    overlap[1]=0;

    audio_draw_buffers = new GLenum[2];
    audio_draw_buffers[0] = GL_COLOR_ATTACHMENT0;
    audio_draw_buffers[1] = GL_COLOR_ATTACHMENT1;



	m_program->setUniformValue("frame_tex",0);
	m_program->setUniformValue("adj_frame_tex",1);
	m_program->setUniformValue("prev_frame_tex",2);
	m_program->setUniformValue("audio_tex",3);
	m_program->setUniformValue("prev_audio_tex",4);

    originalwx=width();
    originalwy=height();


}


void Frame_Window::gen_tex_bufs()
{
    //textures *********************************

    texture_index=0;

    CHECK_GL_ERROR(__FILE__,__LINE__);
    glGenTextures(1,&frame_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    glBindTexture(GL_TEXTURE_2D, frame_texture);
    frame_texture_loc=texture_index;
    texture_index++;
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);


    glGenTextures(1,&adj_frame_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    adj_frame_texture_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, adj_frame_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&prev_adj_frame_tex);
    glActiveTexture(GL_TEXTURE0+texture_index);
    prev_adj_frame_tex_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, prev_adj_frame_tex);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&audio_RGB_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    audio_RGB_texture_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, audio_RGB_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&prev_audio_RGB_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    prev_audio_RGB_texture_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, prev_audio_RGB_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&overlap_compare_audio_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    overlap_compare_audio_texture_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, overlap_compare_audio_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&overlaps_audio_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    overlaps_audio_texture_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, overlaps_audio_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&cal_audio_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    cal_audio_texture_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, cal_audio_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&output_audio_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    output_audio_texture_loc=texture_index;
    texture_index++;
    glBindTexture(GL_TEXTURE_2D, output_audio_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);






    //******************************************Virtual Bench
    VBench_texture = new GLuint[8];
    VBench_texture_loc=texture_index;

    glGenTextures(8,VBench_texture);
    for (int i = 0; i<8; i++)
    {
        glActiveTexture(GL_TEXTURE0+texture_index);
        glBindTexture(GL_TEXTURE_2D, VBench_texture[i]);
        texture_index++;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECK_GL_ERROR(__FILE__,__LINE__);


    }

    glGenTextures(1,&VBench_Strip_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    glBindTexture(GL_TEXTURE_2D, VBench_Strip_texture);
    texture_index++;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenTextures(1,&overlay_texture);
    glActiveTexture(GL_TEXTURE0+texture_index);
    glBindTexture(GL_TEXTURE_2D, overlay_texture);
    overlay_texture_loc=texture_index;
    texture_index++;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);


    //*************************************



    glActiveTexture(GL_TEXTURE0);

    GLint progactive;
    glGetIntegerv(GL_CURRENT_PROGRAM, &progactive);
    qDebug() << "Active program = " << progactive;
    GLuint texLoc = m_program->uniformLocation("frame_tex");
    qDebug() << "frame_tex loc = " << texLoc;
    CHECK_GL_ERROR(__FILE__,__LINE__);
    glUniform1i(texLoc, frame_texture_loc);  // GL Error: invalid operation
    CHECK_GL_ERROR(__FILE__,__LINE__);
    texLoc = m_program->uniformLocation("adj_frame_tex");
    qDebug() << "adj_frame_tex loc = " << texLoc;
    glUniform1i(texLoc, adj_frame_texture_loc); // GL Error: invalid operation
    CHECK_GL_ERROR(__FILE__,__LINE__);
    //glGetUniformLocation()
    texLoc = m_program->uniformLocation("prev_frame_tex");
    qDebug() << "prev_frame_tex loc = " << texLoc;
    glUniform1i(texLoc,prev_adj_frame_tex_loc);
    texLoc = m_program->uniformLocation("audio_tex");
    glUniform1i(texLoc, audio_RGB_texture_loc);
    texLoc = m_program->uniformLocation("prev_audio_tex");
    glUniform1i(texLoc, prev_audio_RGB_texture_loc);
    texLoc = m_program->uniformLocation("overlap_audio_tex");
    glUniform1i(texLoc, overlap_compare_audio_texture_loc);
    texLoc = m_program->uniformLocation("overlapcompute_audio_tex");
    glUniform1i(texLoc, overlaps_audio_texture_loc);
    texLoc = m_program->uniformLocation("overlay_tex");
    glUniform1i(texLoc, overlay_texture_loc);
    texLoc =m_program->uniformLocation("cal_audio_tex");
    glUniform1i(texLoc, cal_audio_texture_loc); // GL Error: invalid operation


    CHECK_GL_ERROR(__FILE__,__LINE__);

    //fbos *********************************

    glGenFramebuffers(1,&adj_frame_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,adj_frame_fbo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,adj_frame_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,input_w,input_h,0,
                 GL_RGBA,GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,
                           adj_frame_texture,0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,prev_adj_frame_tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,input_w,input_h,0,GL_RGBA,
                 GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,
                           prev_adj_frame_tex,0);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glBindFramebuffer(GL_FRAMEBUFFER,0);

    glGenFramebuffers(1,&audio_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,audio_RGB_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,2,samplesperframe,0,GL_RGBA,
                 GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,
                           audio_RGB_texture,0);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D,prev_audio_RGB_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,2,samplesperframe,0,GL_RGBA,
                 GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT1,GL_TEXTURE_2D,
                           prev_audio_RGB_texture,0);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D,overlap_compare_audio_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,2,samplesperframe,0,GL_RGBA,
                 GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT2,GL_TEXTURE_2D,
                           overlap_compare_audio_texture,0);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D,overlaps_audio_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,2,samplesperframe,0,GL_RGBA,
                 GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT3,GL_TEXTURE_2D,
                           overlaps_audio_texture,0);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, cal_audio_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,2,cal_points,0,GL_RGBA,
                 GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT4,GL_TEXTURE_2D,
                           cal_audio_texture,0);

    glGenFramebuffers(1,&audio_file_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,audio_file_fbo);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, output_audio_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,2,4095,0,GL_RGBA,GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,
                           output_audio_texture,0);


    glGenFramebuffers(1,&VBench_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,VBench_fbo);



    for (int i = 0; i<8; i++)
    {

        glActiveTexture(VBench_texture_loc+i);
        glBindTexture(GL_TEXTURE_2D,VBench_texture[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,input_w,input_h,0,
                     GL_RGBA,GL_UNSIGNED_INT,NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,GL_TEXTURE_2D,
                               VBench_texture[i],0);

    }

    glGenFramebuffers(1,&VBench_Strip_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,VBench_Strip_fbo);
    glActiveTexture(VBench_Strip_texture_loc);
    glBindTexture(GL_TEXTURE_2D,VBench_Strip_texture );
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,input_h*5,input_w,0,GL_RGBA,GL_UNSIGNED_INT,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,
                           VBench_Strip_texture,0);

        strip_width=input_h*5;
        strip_height = input_w;


    texLoc = m_program->uniformLocation("VBench_P0");
    glUniform1i(texLoc, VBench_texture_loc);
    texLoc = m_program->uniformLocation("VBench_P1");
    glUniform1i(texLoc, VBench_texture_loc+1);
    texLoc = m_program->uniformLocation("VBench_P2");
    glUniform1i(texLoc, VBench_texture_loc+2);
    texLoc = m_program->uniformLocation("VBench_P3");
    glUniform1i(texLoc, VBench_texture_loc+3);
    texLoc = m_program->uniformLocation("VBench_P4");
    glUniform1i(texLoc, VBench_texture_loc+4);
    texLoc = m_program->uniformLocation("VBench_P5");
    glUniform1i(texLoc, VBench_texture_loc+5);
    texLoc = m_program->uniformLocation("VBench_P6");
    glUniform1i(texLoc, VBench_texture_loc+6);
    texLoc = m_program->uniformLocation("VBench_P7");
    glUniform1i(texLoc, VBench_texture_loc+7);


    glActiveTexture(GL_TEXTURE0);

    CHECK_GL_ERROR(__FILE__,__LINE__);
    glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);



    uint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


    CUR_OP("set vertex arrays");


    //matrix.rotate(45, 0, 1, 0);

    GLfloat verticesPix[] ={
        -1.0,  1.0, 0, // bottom left corner
        1.0,  1.0, 0, // top left corner
        -1.0,  -1.0, 0, // top right corner
        1.0, -1.0, 0  // bottom right corner
    };
    indices=new GLfloat [12] {
            0,1,2, // first triangle (bottom left - top left - top right)
            0,2,3  // second triangle (bottom left - top right - bottom right)
};
    /*
     verticesTex=new GLfloat [12]{
        0, 0, 0, // bottom left corner
        0, 1, 0, // top left corner
        1, 1, 0, // top right corner
        1, 0, 0  // bottom right corner
    };
*/


    verticesTex =new GLfloat [12] {
            0, 1,  // bottom left corner
            1, 1,  // top left corner
            0, 0,  // top right corner
            1,0  // bottom right corner
};
    int jitter;
    int floor = 0, ceiling = 10, range = (ceiling - floor);

    jitter =(ceiling/2)- (floor+int(rand()%range) );
    float jitteroffset = (1.0/input_h) * jitter;
    GLfloat verticesTexJitter[] ={
        0, 0+jitteroffset, 0, // bottom left corner
        0, 1+jitteroffset, 0, // top left corner
        1, 1+jitteroffset, 0, // top right corner
        1, 0+jitteroffset, 0  // bottom right corner
    };


    m_vertexBuffer.create();

    m_vertexBuffer.setUsagePattern( QOpenGLBuffer::StaticDraw );
    m_vertexBuffer.bind();

    m_vertexBuffer.allocate( verticesPix, 3 * 4 * sizeof( GLfloat ) );
    m_program->setAttributeBuffer( "vertex", GL_FLOAT, 0, 4 );

    m_program->enableAttributeArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);


    m_tertexBuffer.create();
    m_tertexBuffer.setUsagePattern( QOpenGLBuffer::DynamicDraw );
    m_tertexBuffer.bind();
    m_tertexBuffer.allocate( verticesTex, 2* 4 * sizeof( GLfloat ) );

    m_program->setAttributeBuffer( "texCoord", GL_FLOAT, 0, 4);
    //m_tertexBuffer.
    m_program->enableAttributeArray( 1 );
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,0);


}
void Frame_Window::savestripimage(QString filename)

{
    float ia_used=              1.0+(overlap[3] - overlap[0])-       overlap[3] ;



    uint8_t * stripbuf;
    stripbuf= new uint8_t [strip_height*strip_width*4];

    glBindFramebuffer(GL_FRAMEBUFFER,VBench_Strip_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, strip_width*ia_used, strip_height,GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV,stripbuf);


    QImage stripimage(stripbuf,strip_width*ia_used,strip_height,QImage::Format::Format_RGBX8888);
    stripimage.mirror(false,true);
    stripimage.save(filename);
    delete []stripbuf;
}

void Frame_Window::load_frame_texture(FrameTexture *frame)
{
    GLenum componentformat;
    CHECK_GL_ERROR(__FILE__,__LINE__);
    glActiveTexture(GL_TEXTURE0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, frame->isNonNativeEndianess) ;
    glBindTexture(GL_TEXTURE_2D,frame_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    switch(frame->nComponents)
    {
    case 4: componentformat = GL_RGBA; break;
    case 3:	componentformat = GL_RGB; break;
    case 1:	componentformat = GL_LUMINANCE; break;
    default: throw vfbexception("Invalid num_components");
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, frame->width, frame->height, 0,
                 componentformat, frame->format, frame->buf);

    CHECK_GL_ERROR(__FILE__,__LINE__);
    new_frame=true;
}

void Frame_Window::update_parameters()
{


    m_program->setUniformValue(m_colorcontrol_loc, lift,gamma,gain,
                               float(!desaturate));
    m_program->setUniformValue(m_manipcontrol_loc, float(thresh),threshold,
                               blur);
    m_program->setUniformValue(m_bounds_loc,
                               bounds[0],bounds[1],bounds[2],bounds[3]);
    m_program->setUniformValue(m_rot_angle,
                                   rot_angle);
    m_program->setUniformValue(marqueebounds_loc,
                               std::min(marqueeBounds[0],marqueeBounds[1]),
                               std::max(marqueeBounds[0],marqueeBounds[1]),
                               std::min(marqueeBounds[2],marqueeBounds[3]),
                               std::max(marqueeBounds[2],marqueeBounds[3]));
    m_program->setUniformValue(pix_bounds_loc,pixbounds[0],pixbounds[1]);
    m_program->setUniformValue(splice_bounds_loc,splice_bounds[0],splice_bounds[1],splice_bounds[2],splice_bounds[3]);
    m_program->setUniformValue(m_overlap_loc,
                               overlap[0],overlap[1],overlap[2],overlap[3]);
    m_program->setUniformValue(m_show_loc, float(trackonly));
    m_program->setUniformValue(stereo_loc, stereo);
    m_program->setUniformValue(m_neg_loc, float(negative));
    m_program->setUniformValue(m_calcontrol_loc, float(cal_enabled),
                               float(is_caling),0,0);
    m_program->setUniformValue(m_overlapshow_loc, float(overlapshow));
    m_program->setUniformValue(m_spliceshow_loc, float(spliceshow));

    m_program->setUniformValue(m_inputsize_loc, float(input_w), float(input_h));
    m_program->setUniformValue(m_overlap_target_loc, overlap_target);
     m_program->setUniformValue(loupeview_loc,loupeview[0],loupeview[1],loupeview[2],loupeview[3]);
   // qDebug()<<loupeview[0]<<","<<loupeview[1]<<","<<loupeview[2]<<","<<loupeview[3];
}

void Frame_Window::CopyFrameBuffer(GLuint fbo, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glClear(GL_COLOR_BUFFER_BIT);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    {
        glBlitFramebuffer(
                    0,0,width,height,
                    0,0,width,height,
                    GL_COLOR_BUFFER_BIT,GL_NEAREST);
    }

    glReadBuffer(0);
    glDrawBuffer(0);
}

void Frame_Window::render()
{
    #ifdef Q_OS_WINDOWS
    originalwx=width();
    originalwy=height();
    #endif

    //  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


    GLfloat * fullarray ;
    CUR_OP("setUniformValues");
    m_program->bind();
    m_program->setUniformValue("frame_tex",frame_texture_loc);
    m_program->setUniformValue("adj_frame_tex",adj_frame_texture_loc);
    m_program->setUniformValue("prev_frame_tex",prev_adj_frame_tex_loc);
    m_program->setUniformValue("audio_tex",audio_RGB_texture_loc);
    m_program->setUniformValue("prev_audio_tex",prev_audio_RGB_texture_loc);
    m_program->setUniformValue("overlap_audio_tex",overlap_compare_audio_texture_loc);
    m_program->setUniformValue("overlapcompute_audio_tex",overlaps_audio_texture_loc);
    m_program->setUniformValue("cal_audio_tex",cal_audio_texture_loc);
    const qreal retinaScale = devicePixelRatio();

    CUR_OP("set matrix to identity");

    QMatrix4x4 matrix;
    // matrix.perspective(60.0f, 4.0f/3.0f, 0.1f, 100.0f);
    // matrix.translate(0, 0, -2);
    matrix.setToIdentity();
    GLfloat verticesPix[] ={
        -1.0,  -1.0, 0, // bottom left corner
        1.0,  -1.0, 0, // top left corner
        -1.0,  1.0, 0, // top right corner
        1.0, 1.0, 0  // bottom right corner
    };

    GLfloat verticesTRO[] = {
        bounds[0], 1.0,  // bottom left corner
        bounds[1], 1.0,  // top left corner
        bounds[0], 0,  // top right corner
        bounds[1],0  // bottom right corner
    };

    if(new_frame)
    {
        CUR_OP("binding adj_frame_fbo");
        CopyFrameBuffer(adj_frame_fbo, input_w, input_h);
        CHECK_GL_ERROR(__FILE__,__LINE__);

        CUR_OP("binding to audio_fbo");
        CopyFrameBuffer(audio_fbo, 2, samplesperframe);
        CHECK_GL_ERROR(__FILE__,__LINE__);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // GL Error: invalid operation
    CHECK_GL_ERROR(__FILE__,__LINE__);

    //************************Adjustment Render********************************
    // Input Textures: frame_tex (original from file)
    // Renders to: adj_frame_tex
    // Description: applies color and density correction to image

    m_tertexBuffer.write(0,verticesTex, 2 * 4 * sizeof( GLfloat ) );// verticesTex, 3 * 4 * sizeof( GLfloat ) );
    m_vertexBuffer.write(0,verticesPix, 3 * 4 * sizeof( GLfloat ) );
    bool jitteractive = false;

    CUR_OP("adjustment render (mode 0)");
    m_program->setUniformValue(m_rendermode_loc, 0.0f);

    CUR_OP("new frame vertext attrib pointed to verticesTex");
    /*
    if(!new_frame || !jitteractive)
        glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0, verticesTex);
    else
        glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0,
                verticesTexJitter);

*/
    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("binding to adj_frame_fbo for new frame");
    glBindFramebuffer(GL_FRAMEBUFFER,adj_frame_fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0,0, input_w, input_h);
    glClear(GL_COLOR_BUFFER_BIT);
    CUR_OP("drawTriangles for adj_frame_fbo new frame");
    //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0,4); //GL ERROR
    CHECK_GL_ERROR(__FILE__,__LINE__);


    //********************************Audio RENDER*****************************
    // Input Textures: adj_frame_texture (adjusted image texture)
    // Renders to: audio_RGB_texture
    // Description: steps through each line within x boundary and computes
    //   value for display
    m_tertexBuffer.write(0,verticesTex, 2 * 4 * sizeof( GLfloat ) );// verticesTex, 3 * 4 * sizeof( GLfloat ) );

    CUR_OP("audio render (mode 1)");
    m_program->setUniformValue(m_rendermode_loc, 1.0f);
    CUR_OP("setting vertexSttribPointer for audio render");
    //	glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0, verticesTex);
    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("binding to audio_fbo in mode 1");
    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0,0, 2, samplesperframe);

    glClear(GL_COLOR_BUFFER_BIT);

    CUR_OP("drawElements for audio_fbo in mode 1");
    glDrawArrays( GL_TRIANGLE_STRIP, 0,4); //GL ERROR
    //	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
    m_tertexBuffer.write(0,verticesTex, 3 * 4 * sizeof( GLfloat ) );// verticesTex, 3 * 4 * sizeof( GLfloat ) );
    m_vertexBuffer.write(0,verticesPix, 3 * 4 * sizeof( GLfloat ) );

    CHECK_GL_ERROR(__FILE__,__LINE__);
    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0); //GL ERROR
    CHECK_GL_ERROR(__FILE__,__LINE__);

    //copy float buffer out
    CUR_OP("copy float buffer out of audio_fbo in mode 1");
    glReadPixels(0,0,2,samplesperframe,GL_RED, GL_FLOAT,audio_compare_buffer);
    fullarray = (static_cast<GLfloat*>(audio_compare_buffer));

    CUR_OP("getting dmin and dmax from audio_fbo in mode 1");
    GLfloat* subdminarray = &fullarray[samplesperframe-samplesperframe/4];
    float dmin =0.0;// GetMin(subdminarray,samplesperframe/4);
    float dmax =1.0;// GetMax(subdminarray,samplesperframe/4);
    m_program->setUniformValue(dminmax_loc, dmin,dmax);
    m_tertexBuffer.write(0,verticesTex, 2 * 4 * sizeof( GLfloat ) );// verticesTex, 3 * 4 * sizeof( GLfloat ) );

    //**********************************Cal RENDER*****************************
    // Input Textures: adj_frame_texture (adjusted image texture)
    // Renders to: cal_audio_texture
    // Description: averages lines with alpha 0.005 200 frames
    if(is_caling)
    {
        CUR_OP("Cal Render");

        m_program->setUniformValue(m_rendermode_loc, 1.0f);
        m_tertexBuffer.write(0,verticesTRO, 2 * 4 * sizeof( GLfloat ) );
        CHECK_GL_ERROR(__FILE__,__LINE__);
        glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT4);
        glViewport(0,0, 2, cal_points);
        if(clear_cal)
        {
            glClear(GL_COLOR_BUFFER_BIT);
            clear_cal=false;
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        //  glBlendFunc (GL_SRC_ALPHA, GL_SRC_ALPHA);
        glEnable( GL_BLEND );
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        glDisable( GL_BLEND );
        glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT4);
        glReadPixels(0, 0, 1, cal_points,GL_RED, GL_FLOAT,audio_compare_buffer);
        CHECK_GL_ERROR(__FILE__,__LINE__);
        fullarray = (static_cast<GLfloat*>(audio_compare_buffer));
        calval=GetAverage(fullarray,cal_points);

        //glClear(GL_COLOR_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        CHECK_GL_ERROR(__FILE__,__LINE__);
    }
    //**************** RENDER Audio & Pix for Overlap for computations*********
    // x0 = curr *** x1 =prev
    // Input Textures: adj_frame_texture (adjusted image texture)
    //   and prev_adj_frame_texture
    // Renders to: overlap_compare_audio_texture
    // Description: computes 1d waveform for current and previous adjusted
    //   frames. pixel column 0 is current and column 1 is previous
    m_tertexBuffer.write(0,verticesTex, 3 * 4 * sizeof( GLfloat ) );// verticesTex, 3 * 4 * sizeof( GLfloat ) );

    CUR_OP("Audio overlap render (mode 4)");
    m_program->setUniformValue(m_rendermode_loc, 4.0f);
    CUR_OP("Set VertextAttribPointer for Audio overlap render (mode 4)");
    //	glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0, verticesTex);
    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("Binding audio_fbo for Audio overlap render (mode 4)");
    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT2);
    glViewport(0,0, 2, samplesperframe);
    glClear(GL_COLOR_BUFFER_BIT);
    CUR_OP("Drawing elements for Audio overlap render (mode 4)");
    glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
    //	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
    CHECK_GL_ERROR(__FILE__,__LINE__);
    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);

    //****************************overlap renders *****************************
    // Input Textures: overlap_compare_audio_texture
    // Renders to: overlaps_audio_texture
    // Description: slides curr and previous 1d arrays over each other and
    // takes the absolute value difference
    //  location is 2 * tex coord

    CUR_OP("Drawing overlaps (mode 5)");
    m_program->setUniformValue(m_rendermode_loc, 5.0f);
    CUR_OP("Set vertexAttribPointer for Drawing overlaps (mode 5)");
    //	glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0, verticesTex);
    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("binding audio_fbo for Drawing overlaps (mode 5)");
    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);

    glDrawBuffer(GL_COLOR_ATTACHMENT3);

    glViewport(0,0, 2, samplesperframe);

    glClear(GL_COLOR_BUFFER_BIT);

    //  glBindTexture(GL_TEXTURE_2D,adj_frame_texture);

    CUR_OP("drawing elements for Drawing overlaps (mode 5)");
    glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
    //	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT3);
    CUR_OP("reading pixels for audio_compare_buffer");
    glReadPixels(0,0,1,samplesperframe,GL_RED, GL_FLOAT,audio_compare_buffer);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    //***********************Find best overlap match***************************
    float low=20.0;
    float temp;
    lowloc=0;
    // for (int i = 4; i<2*(samplesperframe * overlap[1]); i++)

    int start ;
    int end ;
    int pitchline;
    int lowest;
    fullarray = (static_cast<GLfloat*>(audio_compare_buffer));

    CUR_OP("setting search parameters for finding best overlap");
    // end = std::max(end,start);
    bool outsidefind = false;
    start = (overlap[2]+overlap[3]) * samplesperframe -
            (overlap[1]*0.5*samplesperframe) ;
    end = (overlap[2]+overlap[3]) * samplesperframe +
            (overlap[1]*0.5*samplesperframe) ;
    pitchline = (overlap[2]+overlap[3]) * samplesperframe;

    start = std::max(4,start);
    end = std::min(end,1998);
    end=std::max(end,start);
    pitchline = (end+start)/2;

    GLfloat* subarray = &fullarray[samplesperframe-end];

    CUR_OP("getting best match from subarray in finding best overlap");
    GetBestMatchFromFloatArray(subarray,(end-start),end, bestmatch);

    int s_start,s_end;
    int s_size = end-start;
    int s_mid = start + (s_size/2);
    int s_i_size;
    int best=0;

    for (int i = 1; i<6; i++)
    {
        s_i_size =  ((s_size/2)/5);
        if(i==1)
        {
            s_start = s_mid -(4);
            s_end   = s_mid +(4);

        }
        else
        {
            s_start = s_mid -(s_i_size*i);
            s_end   = s_mid +(s_i_size*i);
        }

        s_start= std::max(4,s_start);
        s_end = std::min(s_end,1998);

        subarray = &fullarray[samplesperframe-s_end];

        CUR_OP("getting best match 2 from subarray in finding best overlap");
        GetBestMatchFromFloatArray(subarray,(s_end-s_start),s_end,
                                   match_array[i-1]) ;
    }
    if(is_calc)
    {
        bestmatch= match_array[0];
    }

    else
        bestmatch= match_array[4];

    CUR_OP("recording best overlap");
    overlap[0] = (float)(bestmatch.postion)/2000.0;

    bool usegl=true;

    if(overrideOverlap > 0)
    {
        lowloc = overrideOverlap;
        usegl = false;
    }

    CHECK_GL_ERROR(__FILE__,__LINE__);

    CUR_OP("logging results of overlap computation");
    if(logger)
        (*logger) <<
                     " OpenGL overlap "<< bestmatch.postion <<
                     " Using " << (overrideOverlap?"Override ":"OpenGL ") <<
                     lowloc <<
                     " FrameStart " << overlap[3] <<
                     " FrameStop " << 1.0+(overlap[3] - overlap[0]) <<
                                                                       " start search " << start <<
                                                                       " end search " << end <<
                                                                       "   " << outsidefind <<
                                                                       "\n";

    qDebug() << "jitter: " << "smid: " << s_mid <<
                " Opengl overlap " << bestmatch.postion << " Using OpenGL: " <<
                usegl << " Override: " << overrideOverlap << " FrameStart   " <<
                overlap[3] << " frameStop " << (1.0 - overlap[0] + overlap[3]) <<
                                                                                  " start search" << start << " end search" << end << "   " <<
                                                                                  outsidefind;

    qDebug()<<"MA[0] "<< match_array[0].postion<<" , "<<match_array[0].value;
    qDebug()<<"MA[1] "<< match_array[1].postion<<" , "<<match_array[1].value;
    qDebug()<<"MA[2] "<< match_array[2].postion<<" , "<<match_array[2].value;
    qDebug()<<"MA[3] "<< match_array[3].postion<<" , "<<match_array[3].value;
    qDebug()<<"MA[4] "<< match_array[4].postion<<" , "<<match_array[4].value;

    CUR_OP("calling update_parameters() in overlap computation");
    update_parameters();

    //*************************************************************************
    // Overlap Compute with new coordinates
    CUR_OP("overlap computer with new coordinates");
    float bestvalue = 1.0f+(overlap[3] - ((float)(bestloc)/2000.0));
    float bestvalueoffset =
            1.0f+(overlap[3] - ((float)(bestmatch.postion)/2000.0));

    GLfloat verticesTRO_ForFile[] ={
        bounds[0], bestvalueoffset, // bottom left corner
        bounds[1], bestvalueoffset,  // top left corner
        bounds[0], overlap[3],// top right corner
        bounds[1], overlap[3]// bottom right corner
    };
    GLfloat verticesTex_VBench[] ={
        0, bestvalueoffset, // bottom left corner
        1.0, bestvalueoffset,  // top left corner
        0, overlap[3],// top right corner
        1.0, overlap[3]// bottom right corner
    };
    /*
    GLfloat verticesTRO[] = {
       bounds[0], 1.0,  // bottom left corner
       bounds[1], 1.0,  // top left corner
       bounds[0], 0,  // top right corner
       bounds[1],0  // bottom right corner
   };
   */
    //**********************Video output render*******************************
    // Input Textures: picture textures
    // Renders to: vo fbo
    // Description: draw for video ouput

    if(is_videooutput)
    {
        CUR_OP("screen render (mode 2)");
        m_program->setUniformValue(m_rendermode_loc, 0.0f);
        CUR_OP("setting vertexAttribPointer for screen render (mode 2)");


        // glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0,verticesTex);

        CUR_OP("binding frambuffer for screen render (mode 2)");
        glBindFramebuffer(GL_FRAMEBUFFER,vo.video_output_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        CHECK_GL_ERROR(__FILE__,__LINE__);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0,0,vo.width,vo.height);

        CUR_OP("drawing elements for screen render (mode 2)");
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);

        CHECK_GL_ERROR(__FILE__,__LINE__);

    }


    //**********************Virtual Bench render*******************************
    // Input Textures: Current textures
    // Renders to: VBench fbo
    // Description: Render to VBench Bucket and reassign samplers
    if(new_frame)
    {
        CUR_OP("screen render (mode 2)");
        m_program->setUniformValue(m_rendermode_loc, 0.0f);
        CUR_OP("setting vertexAttribPointer for screen render (mode 2)");

        // glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0,verticesTex);


        fbm->addFrameNumberToBuffer(currentbufferid,currentframenumber);

        CUR_OP("binding frambuffer for screen render (mode 2)");
        glBindFramebuffer(GL_FRAMEBUFFER,VBench_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0+currentbufferid);

        // glDrawBuffer(GL_COLOR_ATTACHMENT1);
        m_tertexBuffer.write(0,verticesTex, 2 * 4 * sizeof( GLfloat ) );// verticesTex, 3 * 4 * sizeof( GLfloat ) );


        // reassign sample pointers to slots sequential call
        m_program->setUniformValue("VBench_P7",VBench_texture_loc+7);
        m_program->setUniformValue("VBench_P6",VBench_texture_loc+6);
        m_program->setUniformValue("VBench_P5",VBench_texture_loc+5);


        QList<int> blist =fbm->getBuffersSortedByFrameNumber();
            m_program->setUniformValue("VBench_P0",VBench_texture_loc+blist[0]);
            m_program->setUniformValue("VBench_P1",VBench_texture_loc+blist[1]);
            m_program->setUniformValue("VBench_P2",VBench_texture_loc+blist[2]);
            m_program->setUniformValue("VBench_P3",VBench_texture_loc+blist[3]);
            m_program->setUniformValue("VBench_P4",VBench_texture_loc+blist[4]);






        CHECK_GL_ERROR(__FILE__,__LINE__);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0,0,input_w,input_h);

        CUR_OP("drawing elements for screen render (mode 2)");
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        CHECK_GL_ERROR(__FILE__,__LINE__);



    }


    //**********************Pix to screen render*******************************
    // Input Textures: picture textures
    // Renders to: screen back buffer
    // Description: display picture
  PaintOverlay();
    if(!is_calculating)
    {
        CUR_OP("screen render (mode 2)");
        m_program->setUniformValue(m_rendermode_loc, 2.0f);
        CUR_OP("setting vertexAttribPointer for screen render (mode 2)");
        CUR_OP("binding frambuffer for screen render (mode 2)");
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glDrawBuffer(GL_BACK);
        CHECK_GL_ERROR(__FILE__,__LINE__);

        glClear(GL_COLOR_BUFFER_BIT);
        if(trackonly)

            m_tertexBuffer.write(0,verticesTRO, 2 * 4 * sizeof( GLfloat ) );// verticesTex, 3 * 4 * sizeof( GLfloat ) );
        else
            m_tertexBuffer.write(0,verticesTex, 2* 4 * sizeof( GLfloat ));
        glViewport(int(float(width()*0.15)*retinaScale),
                   int(float(height()*0.3)*retinaScale),
                   int(float(width()*0.7)*retinaScale),
                   int(float(height()*0.7)*retinaScale));

        m_tertexBuffer.write(0,verticesTex, 2* 4 * sizeof( GLfloat ));
		glViewport(0,
                int(float(originalwy/2.0)*retinaScale),
                int(float(originalwx/2.0)*retinaScale),
                int(float(originalwy/2.0)*retinaScale));


		CUR_OP("drawing elements for screen render (mode 2)");
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        CHECK_GL_ERROR(__FILE__,__LINE__);
        //**********************Strip  render for file *******************************
        float ia_used=              1.0+(overlap[3] - overlap[0])-       overlap[3] ;

        m_program->setUniformValue(m_rendermode_loc, 3.0f);

        glBindFramebuffer(GL_FRAMEBUFFER,VBench_Strip_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0,0,int((strip_width)*ia_used),strip_height);

        CUR_OP("drawing elements for screen render (mode 2)");
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        CHECK_GL_ERROR(__FILE__,__LINE__);
        //*************Strip Render to Screen****************
        // Input Textures:
        // Renders to: screen back buffer
        // Description: display strip

       // PaintStripOverlay();
        CUR_OP("soundwaveform render (mode 3)");
        m_program->setUniformValue(m_rendermode_loc, 3.0f);

        CUR_OP("setting vertexAttribPointer for soundwaveform render (mode 3)");

        //		glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0, verticesTex);
        CHECK_GL_ERROR(__FILE__,__LINE__);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glDrawBuffer(GL_BACK);


        glViewport(0,0, (originalwx * retinaScale), (originalwy *retinaScale*(1.0-0.5))-(originalwy*0.1));

        CUR_OP("drawing elements for soundwaveform render (mode 3)");
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        CHECK_GL_ERROR(__FILE__,__LINE__);



        //*************Loupe Render to Screen****************
        // Input Textures:
        // Renders to: screen back buffer
        // Description: display Loupe



        CUR_OP("loupe render (mode 11)");
        m_program->setUniformValue(m_rendermode_loc, 80.0f);

        CUR_OP("setting vertexAttribPointer for lope render (mode 80)");
        //		glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0, verticesTex);

        CHECK_GL_ERROR(__FILE__,__LINE__);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glDrawBuffer(GL_BACK);

        m_tertexBuffer.write(0,verticesTex, 2* 4 * sizeof( GLfloat ));
                glViewport(int(float(originalwx/2.0)*retinaScale)+originalwx*0.01,
                        int(float(originalwy/2.0)*retinaScale),
                        int(float(originalwx/2.0)*retinaScale),
                        int(float(originalwy/2.0)*retinaScale));

        CUR_OP("drawing elements for soundwaveform render (mode 3)");
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        CHECK_GL_ERROR(__FILE__,__LINE__);

        //*************Overlay Render to Screen****************
        // Input Textures:
        // Renders to: screen back buffer
        // Description: display Loupe



        CUR_OP("Overlay render (mode 99)");
        m_program->setUniformValue(m_rendermode_loc, 99.0f);

        CUR_OP("setting vertexAttribPointer for lope render (mode 80)");
        //		glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0, verticesTex);

        CHECK_GL_ERROR(__FILE__,__LINE__);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glDrawBuffer(GL_BACK);

        m_tertexBuffer.write(0,verticesTex, 2* 4 * sizeof( GLfloat ));
        glViewport(0,0,
                   int(float(originalwx)*retinaScale),
                   int(float(originalwy)*retinaScale));

        CUR_OP("drawing elements for soundwaveform render (mode 3)");
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        //  glBlendFunc (GL_SRC_ALPHA, GL_SRC_ALPHA);
        glEnable( GL_BLEND );
        glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        glDisable( GL_BLEND );
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
        CHECK_GL_ERROR(__FILE__,__LINE__);

    }

    //***********************Audio RENDER for file*****************************
    // Input Textures: prev_adj_frame_texture
    // Renders to: output_audio_texture
    // Description: computes audio from prev texture between x and y
    //   calculated space.

    CUR_OP("audio render for file (mode 1.5)");
    m_program->setUniformValue(m_rendermode_loc, 1.5f);
    CUR_OP("set vertexAttribPointer  for audio render for file (mode 1.5)");

    m_tertexBuffer.write(0,verticesTRO_ForFile, 2 * 4 * sizeof( GLfloat ) );
    //	glVertexAttribPointer(m_texAttr, 3, GL_FLOAT, GL_FALSE, 0,

    //		verticesTRO_ForFile);
    CHECK_GL_ERROR(__FILE__,__LINE__);
    CUR_OP("binding audio_file_fbo for audio render for file (mode 1.5)");
    glBindFramebuffer(GL_FRAMEBUFFER,audio_file_fbo);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0,0, 2, samplesperframe_file);

    glClear(GL_COLOR_BUFFER_BIT);

    //  glBindTexture(GL_TEXTURE_2D,adj_frame_texture);

    CUR_OP("drawing elements for audio render for file (mode 1.5)");
    glDrawArrays( GL_TRIANGLE_STRIP, 0,4);
    //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
    CHECK_GL_ERROR(__FILE__,__LINE__);
    m_tertexBuffer.write(0,verticesTex, 2 * 4 * sizeof( GLfloat ) );

    glBindFramebuffer(GL_FRAMEBUFFER,audio_file_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // is_rendering = recording to filebuffer
    // new_frame indicates a frame texture was loaded
    if (is_rendering && new_frame )
    {
        CUR_OP("reading left channel for audio render for file (mode 1.5)");
        //copy float buffer out for file left channel
        glReadPixels(0, 0, 1, samplesperframe_file,GL_RED, GL_FLOAT,
                     &FileRealBuffer[0][samplepointer]);


        CUR_OP("reading right channel for audio render for file (mode 1.5)");
        //copy float buffer out for file right channel
        glReadPixels(1, 0, 1, samplesperframe_file,GL_RED, GL_FLOAT,
                     &FileRealBuffer[1][samplepointer]);


        samplepointer+=samplesperframe_file;
    }
    CHECK_GL_ERROR(__FILE__,__LINE__);

    CUR_OP("binding fbo 0 for audio render for file (mode 1.5)");
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    CUR_OP("disable vertex array");
    //	glDisableVertexAttribArray(1);

    //glDisableVertexAttribArray(0);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    CUR_OP("release m_program");
    m_program->release();

    CUR_OP("increment frame counter");
    ++m_frame;

    CUR_OP("");



    new_frame=false;
}

void Frame_Window::PrepareRecording(int numsamples)
{
    FileRealBuffer= new float* [2];

    FileRealBuffer[0] = new float[numsamples];
    FileRealBuffer[1] = new float[numsamples];

    if(logger)
    {
        (*logger) << "FileRealBuffer = [" << FileRealBuffer[0] <<
                     "," << FileRealBuffer[1] << "] (2x" << numsamples << "\n";

    }
    fprintf(stderr, "FileRealBuffer = [%p,%p]\n",
            FileRealBuffer[0], FileRealBuffer[1]);
    fflush(stderr);

    samplepointer=0;
}

void Frame_Window::PaintOverlay() {
    const qreal retinaScale = devicePixelRatio();

    QImage image(int((width()) * retinaScale), int((height()) * retinaScale),
                 QImage::Format_RGBA8888);
    image.fill(Qt::transparent);

    int eventimage_w = image.width() / 2;
    int eventimage_h = image.height() / 2;
    int stripbottom = image.height() * 0.55;
    int striptop = image.height() * 0.5;
    int stripcenter = striptop + ((stripbottom - striptop) / 2);
    QPainter painter(&image);

    // painter.setPen(Qt::yellow);
    // painter.setBrush(Qt::blue);
    // painter.drawRect(10, 10, 100, 50);
    QPen borderPen(Qt::cyan);
    borderPen.setWidth(5);
    int borderhalf = 3;
    painter.setPen(borderPen);
    QFont font = painter.font();
    int pointSize = 20;
    font.setPointSize(pointSize);

    /* set the modified font to the painter */
    painter.setFont(font);
    if (1) // show splice
    {
        foreach (vbevent *event, currentevents) {
            QPoint textp;
            QString text(event->TypeName());

            if (!event->SubType().isEmpty())
              text += " - " + event->SubType();

            // Multiframe events: append "n of N" count
            if (event->Start() < event->End())
              text += " [" +
                      QVariant(spliceFrameNum - event->Start() + 1).toString() +
                      "/" +
                      QVariant(event->End() - event->Start() + 1).toString() +
                      "]";

            int left = event->BoundsX0() * eventimage_w;
            int right = event->BoundsX1() * eventimage_w;
            int top = event->BoundsY0() * eventimage_h;
            int bottom = event->BoundsY1() * eventimage_h;

            if (event->IsContinuous()) {
              // draw box top only on first frame,
              // otherwise extend box to top of image
              if (event->Start() == uint32_t(spliceFrameNum))
                painter.drawLine(left, top, right, top);
              else
                top = 0;

              // draw box bottom only on last frame,
              // otherwise extend box to bottom of image
              if (event->End() == uint32_t(spliceFrameNum))
                painter.drawLine(left, bottom, right, bottom);
              else
                bottom = eventimage_h;

              // draw the (now extended) sides of the box
              painter.drawLine(left, top, left, bottom);
              painter.drawLine(right, top, right, bottom);
            } else {
              QPoint tl(left, top);
              QSize sz(event->BoundsSizeX() * eventimage_w,
                       event->BoundsSizeY() * eventimage_h);

              QRectF ddd(tl, sz);
              painter.drawRect(ddd);
            }

            // make sure the text fits in under the top of the image
            textp.setX(left);
            textp.setY(std::max(top - 20, pointSize));
            painter.drawText(textp, text);
        }
    }
    font.setPointSize(36);
    painter.setFont(font);
    painter.setBrush(QBrush(QColor(100, 0, 0, 100)));
    painter.setPen(Qt::red);

    QPolygon polygon;
    polygon << QPoint(image.width() * 0.4, striptop + borderhalf)
            << QPoint(image.width() * 0.6, striptop + borderhalf)
            << QPoint(image.width() * 0.5 + (image.width() * 0.05),
                      stripbottom - borderhalf)
            << QPoint(image.width() * 0.5 - (image.width() * 0.05),
                      stripbottom - borderhalf);
    painter.drawPolygon(polygon);

    painter.setPen(Qt::white);
    drawCenteredText(painter, currentframestring, font, eventimage_w,
                     stripcenter);

    painter.drawLine(image.width() * 0.2, stripcenter, image.width() * 0.2,
                     stripbottom);

    painter.drawLine(image.width() * 0.4, stripcenter, image.width() * 0.4,
                     stripbottom);
    painter.drawLine(image.width() * 0.6, stripcenter, image.width() * 0.6,
                     stripbottom);
    painter.drawLine(image.width() * 0.8, stripcenter, image.width() * 0.8,
                     stripbottom);
    painter.end(); // Ensure to end the painter to flush the drawings
    glActiveTexture(overlay_texture_loc);
    glBindTexture(GL_TEXTURE_2D, overlay_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
}
void Frame_Window::DestroyRecording()
{
    delete [] FileRealBuffer[1];
    delete [] FileRealBuffer[0];

    delete[] FileRealBuffer ;
    FileRealBuffer = NULL;
    samplepointer=0;
}

void Frame_Window::ProcessRecording(int numsamples)
{

}
void Frame_Window::PrepareVideoOutput(FrameTexture * frame)
{
    if(vo.video_output_fbo!=0)
    {
        glDeleteFramebuffers(1,&vo.video_output_fbo);
        glDeleteTextures(1,&vo.video_output_texture);
    }
    if(vo.videobuffer!=NULL)
        delete[] vo.videobuffer;


    vo.videobuffer=frame->buf;

    vo.height=frame->height;
    vo.width= frame->width;
    glGenTextures(1,&vo.video_output_texture);
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, vo.video_output_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glGenFramebuffers(1,&vo.video_output_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER,vo.video_output_fbo);

    glBindTexture(GL_TEXTURE_2D, vo.video_output_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,frame->width,frame->height,0,
                 GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,
                           vo.video_output_texture,0);


    glActiveTexture(GL_TEXTURE0);
}

void Frame_Window::read_frame_texture(FrameTexture * frame)
{
    if(vo.videobuffer == NULL) return;

    glBindBuffer(GL_PIXEL_PACK_BUFFER,0);

    //  glPixelStorei(GL_UNPACK_SWAP_BYTES,0);

    glBindFramebuffer(GL_FRAMEBUFFER,vo.video_output_fbo);

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // Reference Point: LSJ-20170519-1322
    // See mainwindow.cpp:LSJ-20170519-1322
    glReadPixels(0, 0, frame->width, frame->height,GL_RGBA,
                 GL_UNSIGNED_INT_8_8_8_8_REV, vo.videobuffer); //copy buffer out to

}

float *Frame_Window::GetCalibrationMask()
{
    /*
    float *buf = new float[cal_points];

    glBindFramebuffer(GL_FRAMEBUFFER,audio_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT4);
    glReadPixels(0, 0, 1, cal_points,GL_RED, GL_FLOAT,buf);
    */

    float *buf2 = new float[cal_points*2];
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, cal_audio_texture);
    glGetTexImage(GL_TEXTURE_2D,0,GL_RED,GL_FLOAT,buf2);

    return buf2;
}

void Frame_Window::SetCalibrationMask(const float *mask)
{
    CHECK_GL_ERROR(__FILE__,__LINE__);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, cal_audio_texture);
    CHECK_GL_ERROR(__FILE__,__LINE__);

    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,2,cal_points,0,GL_LUMINANCE,
                 GL_FLOAT,mask);


    CHECK_GL_ERROR(__FILE__,__LINE__);

    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT4,GL_TEXTURE_2D,
                           cal_audio_texture,0);

    CHECK_GL_ERROR(__FILE__,__LINE__);
}


float Frame_Window::GetAverage(GLfloat* dArray, int iSize)
{
    double dSum = dArray[0];
    for (int i = 1; i < iSize; ++i)
    {
        dSum += dArray[i];
    }
    return float (dSum/iSize);
}

void Frame_Window::GetBestMatchFromFloatArray(GLfloat* dArray, int iSize,
                                              int start , overlap_match &bmatch)

{
    int iCurrMin = 0;
    float iCurrMinValue = 0;

    for (int i = 1; i < iSize; ++i)
    {
        if (dArray[iCurrMin] > dArray[i])
        {
            iCurrMin =i;
            bmatch.postion= start-  i;
            bmatch.value = dArray[i];
        }
    }
}

int Frame_Window::GetMinLoc(GLfloat* dArray, int iSize)
{
    int iCurrMin = 0;

    for (int i = 1; i < iSize; ++i)
    {
        if (dArray[iCurrMin] > dArray[i])
        {
            iCurrMin = i;
        }
    }
    return iCurrMin;
}

float Frame_Window::GetMin(GLfloat* dArray, int iSize)
{
    int iCurrMin = 0;

    for (int i = 1; i < iSize; ++i)
    {
        if (dArray[iCurrMin] > dArray[i])
        {
            iCurrMin = i;
        }
    }
    return dArray[iCurrMin];
}

float Frame_Window::GetMax(GLfloat* dArray, int iSize)
{
    int iCurrMax= 0;

    for (int i = 1; i < iSize; ++i)
    {
        if (dArray[iCurrMax] < dArray[i])
        {
            iCurrMax = i;
        }
    }
    return dArray[iCurrMax];
}

void Frame_Window::ParamUpdateCallback(FrameWindowCallbackFunction cb,
                                       void *userData)

{
    this->paramUpdateCB = cb;
    this->paramUpdateUserData = userData;
}

void Frame_Window::mouseReleaseEvent(QMouseEvent *mouse)
{
    if(mouse->button() == Qt::LeftButton) mouseEvent(mouse);
    else mouse->ignore();
}

void Frame_Window::keyReleaseEvent(QKeyEvent * k)
{
    switch(k->key())
    {
    case Qt::Key_L:
        loupeactive=0;
        qDebug()<<loupeactive;
        break;

    case Qt::Key_Shift:
        shiftactive=0;
        break;

    default:
        k->ignore();
    }
}
void Frame_Window::keyEvent(QKeyEvent *key)
{
    if(key->key() == Qt::Key_Space)
    {
        emit PlayPause();
    }
    else if(key->key()== Qt::Key_L)
    {
        loupeactive=1;
        qDebug()<<loupeactive;
    }
    else if(key->modifiers()  == Qt::ShiftModifier)
    {
        qDebug()<<"shift";
        shiftactive=1;
    }
    else if(key->key() == Qt::Key_Up && shiftactive==1 )
    {
        loupeview[0]+=0.01  ;
        this->update_parameters();
        qDebug()<<"up";
        this->renderNow();
    }
    else if(key->key() == Qt::Key_Down && shiftactive==1 )
    {
        loupeview[0]-=0.01   ;
        this->update_parameters();
        qDebug()<<"down";
        this->renderNow();
    }
    // Check for CTRL-NUM, including from the Num Keypad,
    // and emit signal if found
    else if(
        ((key->modifiers()&(~Qt::KeypadModifier)) == Qt::ControlModifier) &&
        (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9))
    {
        int num = key->key() - Qt::Key_0;
        emit ShortcutCtrlNum(num);
    }
    else
    {
        key->ignore();
    }
}

void Frame_Window::wheelEvent(QWheelEvent *ev)
{
    qDebug()<<"Wheel Event";
    if(ev->angleDelta().y() > 0) // up Wheel
    {
        loupeview[0]+=0.01;
        qDebug()<<"Wheel up";
    }
    else if(ev->angleDelta().y() < 0) //down Wheel
    {
        loupeview[0]-=0.01;
        qDebug()<<"Wheel down";
    }
    else
    {
        ev->ignore();
        return;
    }

    this->update_parameters();
    this->renderNow();
    ev->accept();
}

void Frame_Window::mouseEvent(QMouseEvent *mouse)
{
    // ignore user clicking/releasing middle or right buttons
    // this routine only cares about mousedrag and left clicks
    if(
            (mouse->type() != QEvent::MouseMove) &&
            !(mouse->button() == Qt::LeftButton &&
                (mouse->type() == QEvent::MouseButtonPress ||
                 mouse->type() == QEvent::MouseButtonRelease)
            )
        )
    {
        mouse->ignore();
        return;
    }

    static bool grabIsActive = false;

    static float x,y, fx,fy,fny;
    static int grab = -1; // the index of the thing being grabbed
    static int hover = -1; // the index of the thing being hovered over

    static const struct {
        GLfloat *targ;
        float *src;
        float maxval;
        enum Qt::CursorShape cursor;
    } grabArr[] = {
        /*
        { &(this->bounds[0]), &x, 1.0, Qt::SplitHCursor },
        { &(this->bounds[1]), &x, 1.0, Qt::SplitHCursor },
        { &(this->pixbounds[0]), &x, 1.0, Qt::SplitHCursor },
        { &(this->pixbounds[1]), &x, 1.0, Qt::SplitHCursor },
        */
        { &(this->overlap[2]), &fny, 0.35, Qt::SplitVCursor },
        { &(this->overlap[3]), &fy, 0.35, Qt::SplitVCursor },
        { NULL, NULL, 0, Qt::ArrowCursor }
    };

    // -1 for no grab, 0 for min, 1 for max
    static int spliceGrabX = -1;
    static int spliceGrabY = -1;
    static int spliceHoverX = -1;
    static int spliceHoverY = -1;
    static int marqueeGrab = -1;

    static vbevent *eventGrab(nullptr);

    /*
    static const struct {
        GLfloat *targPoint[2];
        float *srcPoint[2];
        enum Qt::CursorShape cursor;
    } grabBoundsArr[] = {
        {
            {&(this->splice_bounds[0]), &(this->splice_bounds[2]) },
            {&x, &y},
            Qt::SizeFDiagCursor
        },
        {
            {&(this->splice_bounds[0]), &(this->splice_bounds[3]) },
            {&x, &y},
            Qt::SizeBDiagCursor
        },
    };
    */
    int i;

    // horizontal boundary markers are recorded as ratio of image width

    x = float(mouse->position().x())/float(this->width());

    // frame pitch markers are recorded as ratio of frame height
    y = float(mouse->position().y())/float(this->height());


    if(y > 1.0)
    {
        mouse->ignore();
        return;
    }

    if(x<0.5 && y < 0.5)
    {
        fx = fmin(x*2.0f,1.0f);
        fy = fmin(y*2.0f,1.0f);
        fny = 1.0f - fy;
    }
    else
    {
        fx = fy = fny = -1.0f;
    }

    //new_frame=false;
    // this->renderNow();
    if (loupeactive==1)
    {
        if (x<0.5 && y<0.5)
        {
            loupeview[2]=((x-0.25)*2.0);
            loupeview[3]=(y-0.25)*2.0;
            loupeview[1]=0.0;
            this->update_parameters();
            qDebug()<<"mouse pointer positionx "<<x<<"  "<<y;
            this->renderNow();
        }
        else if (y>0.5)
        {
            loupeview[1]=1.0;
            loupeview[2]=((x-0.5)*2.0);
            loupeview[3]=(y-0.75)*2.0;

            this->update_parameters();
            qDebug()<<"mouse pointer positionx "<<x<<"  "<<y;
            this->renderNow();
        }
    }

    if(mouse->type() == QEvent::MouseButtonRelease)
    {
        if(marqueeGrab > -1)
        {
            if(marqueeBounds[0] == marqueeBounds[1] &&
                marqueeBounds[2] == marqueeBounds[3])
            {
                marqueeBounds[0] = marqueeBounds[1] =
                    marqueeBounds[2] = marqueeBounds[3] = 0.0f;
            }
            else
            {
                // while dragging, the first click is stored first.
                // upon release, ensure the bounds are set to min first
                if(marqueeBounds[0] > marqueeBounds[1])
                    std::swap(marqueeBounds[0], marqueeBounds[1]);
                if(marqueeBounds[2] > marqueeBounds[3])
                    std::swap(marqueeBounds[2], marqueeBounds[3]);
            }
        }
        // only leftbutton releases would make it this far in the function
        grabIsActive = false;
        grab = -1;
        hover = -1;
        spliceGrabX = -1;
        spliceGrabY = -1;
        marqueeGrab = -1;
        return;
    }

    if(spliceshow && ((x<0.5 && y<0.5) || spliceGrabX >=0 || spliceGrabY >=0))
    {
        // new click?
        if(mouse->type() == QEvent::MouseButtonPress)
        {
            if(spliceHoverX >= 0 || spliceHoverY >= 0)
            {
                spliceGrabX = spliceHoverX;
                spliceGrabY = spliceHoverY;
                grabIsActive = true;

                qDebug() << "grabbed X" << spliceGrabX << ", Y" << spliceGrabY;
                return;
            }
        }
        // Are we roaming (haven't already grabbed something)?
        else if(!grabIsActive)
        {
            spliceHoverX = spliceHoverY = -1;
            eventGrab = nullptr;

            vbevent *event = nullptr;
            float closestDist = std::numeric_limits<float>::infinity();

            float bounds[4];

            for(vbevent* e: this->currentevents)
            {
                // build the effective boundary of continuous events
                for(int i=0; i<4; i++) bounds[i] = e->Bounds()[i];
                if(e->IsContinuous())
                {
                    if(uint32_t(spliceFrameNum) != e->Start())
                        bounds[2] = -std::numeric_limits<float>::infinity();
                    if(uint32_t(spliceFrameNum) != e->End())
                        bounds[3] = std::numeric_limits<float>::infinity();;
                }

                float d = distToBounds(fx, fy, bounds);
                if(d < closestDist)
                {
                    closestDist = d;
                    event = e;
                }
            }

            // Was the click "close" to any event box?
            if(event)
            {
                // build the effective boundary of continuous events
                for(int i=0; i<4; i++) bounds[i] = event->Bounds()[i];
                if(event->IsContinuous())
                {
                    if(uint32_t(spliceFrameNum) != event->Start())
                        bounds[2] = -std::numeric_limits<float>::infinity();
                    if(uint32_t(spliceFrameNum) != event->End())
                        bounds[3] = std::numeric_limits<float>::infinity();;
                }

                // between the Y's and on one of the X's?
                if(fy > bounds[2]-0.01f && fy < bounds[3]+0.01f)
                    for(i=0; i<2; ++i)
                        if(fabs(bounds[i] - fx) < 0.01)
                        {
                            spliceHoverX = i;
                            break;
                        }

                // between the X's and on one of the Y's?
                if(fx > bounds[0]-0.01f && fx < bounds[1]+0.01f)
                    for(i=0; i<2; ++i)
                        if(fabs(bounds[2+i] - fy) < 0.01)
                        {
                            spliceHoverY = i;
                            break;
                        }

                if((spliceHoverX >= 0) || (spliceHoverY >= 0))
                {
                    eventGrab = event;

                    if(spliceHoverX == -1)
                        setCursor(Qt::SizeVerCursor);
                    else if(spliceHoverY == -1)
                        setCursor(Qt::SizeHorCursor);
                    else if(spliceHoverX == spliceHoverY)
                        setCursor(Qt::SizeFDiagCursor);
                    else
                        setCursor(Qt::SizeBDiagCursor);

                    return;
                }
            }
        }
        else if(eventGrab) // drag
        {
            const float *bounds = eventGrab->Bounds();
            float newBounds[4];

            for(int i=0; i<4; i++) newBounds[i] = bounds[i];

            switch(spliceGrabX)
            {
            case 0: newBounds[0] = fmin(fx, bounds[1]); break;
            case 1: newBounds[1] = fmax(fx, bounds[0]); break;
            default: break;
            }
            switch(spliceGrabY)
            {
            case 0: newBounds[2] = fmin(fy, bounds[3]); break;
            case 1: newBounds[3] = fmax(fy, bounds[2]); break;
            default: break;
            }

            emit ResizedEventBoundingBox(eventGrab,
                newBounds[0], newBounds[1],
                newBounds[2], newBounds[3]);

            renderNow();

            return;
        }
    }
    // if we made it this far, either the splice box isn't shown or no
    // splice edge is being clicked or dragged. So on to the other items
    // that can be clicked or dragged...

    // new click?
    if(mouse->type() == QEvent::MouseButtonPress)
    {
        if(hover != -1)
        {
            grab = hover;
            grabIsActive = true;
        }
        else
        {
            marqueeGrab = 1;
            marqueeBounds[0] = marqueeBounds[1] = fx;
            marqueeBounds[2] = marqueeBounds[3] = fy;
            grabIsActive = true;
            this->update_parameters();
            this->renderNow();
        }
        /*
        // Check if there overlap boundary markers are not yet
        // present. In that case, set them to near the mouse.
        else if(this->bounds[0] == 0 && this->bounds[1] == 0)
        {
            this->bounds[0] = x - 0.05;
            if(this->bounds[0] < 0) this->bounds[0] = 0;

            this->bounds[1] = x + 0.05;
            if(this->bounds[1] > 1.0) this->bounds[1] = 1.0;

            // copy the changes to the shader program
            this->update_parameters();

            // copy the changes back to the GUI, if requested
            if(this->paramUpdateCB)
                (*(this->paramUpdateCB))(this->paramUpdateUserData);

            this->renderNow();
        }
        */
        return;
    }

    // "trackonly" records state of checkbox called "Show Frame Pitch"
    if(!grabIsActive && trackonly)
    {
        // free roaming -- looking for things to grab:
        for(i=0; grabArr[i].targ; ++i)
        {
            if(fabs(*(grabArr[i].src) - *(grabArr[i].targ)) < 0.01)
            {
                hover = i;
                break;
            }
        }
        this->setCursor(grabArr[i].cursor);
        if(grabArr[i].targ == NULL) hover = -1;
    }
    else
    {
        if(grab > -1)
        {
            if(*(grabArr[grab].src) >= 0 &&
                *(grabArr[grab].src) <= (grabArr[grab].maxval))
            {
                // update the value in the frame_view_gl class
                *(grabArr[grab].targ) = *(grabArr[grab].src);
            }
        }
        else if(marqueeGrab > -1)
        {
            float fx = fmin(x*2.0f,1.0f);
            float fy = fmin(y*2.0f,1.0f);
            marqueeBounds[1] = fx;
            marqueeBounds[3] = fy;
        }
        else return;

        // copy the changes back to the GUI, if requested
        // NOTE: this may change some of the values if the
        // "synch overlap" checkbox is checked.
        if(this->paramUpdateCB)
            (*(this->paramUpdateCB))(this->paramUpdateUserData);

        // copy the changes (with possible additional changes from
        // the paramUpdateCB) to the shader program
        this->update_parameters();

        this->renderNow();
    }
}

// distance to closest boundary edge
float Frame_Window::distToBounds(float x, float y, const float *bounds) const
{
    float dx0, dx1, dy0, dy1;
    float dx, dy;

    if(y > bounds[2]-0.01f && y < bounds[3]+0.01f)
    {
        dx0 = fabs(x-bounds[0]);
        dx1 = fabs(x-bounds[1]);
        dx = std::min(dx0,dx1);
    }
    else
        dx = std::numeric_limits<float>::infinity();

    if(x > bounds[0]-0.01f && x < bounds[1]+0.01f)
    {
        dy0 = fabs(y-bounds[2]);
        dy1 = fabs(y-bounds[3]);
        dy = std::min(dy0,dy1);
    }
    else
        dy = std::numeric_limits<float>::infinity();

    return std::min(dx,dy);
}
