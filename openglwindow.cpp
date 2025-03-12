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

#include "openglwindow.h"
#include "QtGui/qevent.h"
#include <iostream>
#include <QtCore/QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QtGui/QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QtGui/QPainter>

OpenGLWindow::OpenGLWindow(QWindow *parent)
    : QWindow(parent)
    , m_update_pending(false)
    , m_animating(false)
    , m_context(0)
    , m_device(0)
{

    setSurfaceType(QWindow::OpenGLSurface);
    windowaspect_y2x=0.7;

}
 void OpenGLWindow::resizeEvent(QResizeEvent *event)
 {

     // first try to detect the direction of the window resize
     if (event->oldSize().width() != event->size().width())
        this->resize(event->size().width(),             // apply the correct ratio
                     event->size().width() * windowaspect_y2x); // to either of width/height
     else
        this->resize(event->size().height() / windowaspect_y2x, // apply the correct ratio
                     event->size().height());           // to either of width/height
     event->accept(); // we've finished processing resize event here



 }
OpenGLWindow::~OpenGLWindow()
{
    delete m_device;
}
void OpenGLWindow::render(QPainter *painter)
{
    Q_UNUSED(painter);

}

void OpenGLWindow::initialize()
{

}

void OpenGLWindow::render()
{
    if (!m_device)
        m_device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_device->setSize(size());

    QPainter painter(m_device);
    render(&painter);

}

void OpenGLWindow::renderLater()
{
    if (!m_update_pending) {
    //	m_update_pending = true;
       // QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool OpenGLWindow::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::UpdateRequest:
        m_update_pending = false;
       // renderNow();
        return true;


    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();

}



void OpenGLWindow::renderNow()
{
    if (!isExposed())
        return;

    bool needsInitialize = false;



    if (!m_context) {
        m_context = new QOpenGLContext(this);
        m_context->setParent(NULL);

        m_context->setFormat(requestedFormat());
        m_context->create();

        needsInitialize = true;
    }
    if (m_context->currentContext() ==0)
        m_context->makeCurrent(this);



    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();
    }
    \


    render();










    m_context->swapBuffers(this);


    if (m_animating)
        renderLater();
}

void OpenGLWindow::setAnimating(bool animating)
{
    m_animating = animating;

    if (animating)
        renderLater();


}

void OpenGLWindow::PrintGLVersion(QTextStream &stream)
{
    stream << "OpenGL version "<< (this->m_context->format().majorVersion()) << "."<< (this->m_context->format().minorVersion()) << "\n";
}
