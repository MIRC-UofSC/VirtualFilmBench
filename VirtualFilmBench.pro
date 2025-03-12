#-----------------------------------------------------------------------------
# This file is part of Virtual Film Bench
#
# Copyright (c) 2024 South Carolina Research Foundation and Thomas Aschenbach
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# Virtual Film Bench is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# Funding for Virtual Film Bench development was provided through a grant
# from the National Endowment for the Humanities
#
# Project contributors include: Thomas Aschenbach (Colorlab, inc.),
# L. Scott Johnson (USC), Greg Wilsbacher (USC), Pingping Cai (USC),
# and Stella Garcia (USC).
#-----------------------------------------------------------------------------

# PREREQUISITES:
#
# place libraries and include files in system locations like /usr/local/lib
# on unix and osx, or under C:\include and C:\lib under windows, or put
# them or link them to the Virtual Film Bench source directory tree either directly
# or under subdirectories called "release" and "debug"
#
# Or add the appropriate paths to the platform-specific sections of this
# project file
#
# Additional libraries required:
# libav (the ffmpeg libraries)
# dpx
# dspfilters
# openexr

QT       += core gui multimedia xml

QT += opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VirtualFilmBench
TEMPLATE = app

# The version of Virtual Bench
APP_NAME = Virtual-Film-Bench
VERSION = 1.0
VERSION_QUAL =

# nb: when updating the version number, be sure to update LICENSE.txt too

# Make the version number visible in the source code
DEFINES += APP_NAME=\\\"$$APP_NAME\\\"
DEFINES += APP_VERSION_STR=\\\"$$VERSION\\\"
DEFINES += APP_VERSION_QUAL=\\\"$$VERSION_QUAL\\\"

ICON = $$PWD/virtualbench.icns
RC_ICONS = $$PWD/virtualbench.ico

#------------------------------------------------------------------------------
# platform-specific include paths
win32 {
        INCLUDEPATH += /include
        DEFINES += __STDC_CONSTANT_MACROS
        INCLUDEPATH += $$PWD/include
} else:unix {
        INCLUDEPATH += /usr/local/include/ /opt/local/include/ /opt/homebrew/Cellar/ffmpeg@4/4.4.4/include/
}

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/


# If DSP Filters are not on the include path yet...
#INCLUDEPATH += ../DSPFilters/shared/DSPFilters/include
INCLUDEPATH+=   /opt/homebrew/include
QMAKE_LIBDIR +=  /opt/homebrew/lib
#-----------------------------------------------------------------------------
# platform-specific linking
macx {
        #QMAKE_LIBDIR += /usr/local/lib /opt/local/lib
        QMAKE_LIBDIR_FLAGS += -L/opt/homebrew/Cellar/ffmpeg@4/4.4.4/lib/
        QMAKE_LIBDIR_FLAGS += -L/opt/homebrew/Cellar/
        QMAKE_RPATHDIR += @executable_path/../Frameworks/

        QMAKE_LINK += -headerpad_max_install_names
} else:win32 {
        QMAKE_LIBDIR += "C:/lib"
        QMAKE_LIBDIR += $$PWD/lib
        LIBS += -lopengl32
} else:unix {
        LIBS += -lGL
}

CONFIG(release, debug|release): QMAKE_LIBDIR += $$PWD/release/
else:CONFIG(debug, debug|release): QMAKE_LIBDIR += $$PWD/debug/

#------------------------------------------------------------------------------
SOURCES += \
    attributelabel.cpp \
    decimalelidedelegate.cpp \
    eventdataform.cpp \
    eventdialog.cpp \
    eventfilter.cpp \
    eventfilterdialog.cpp \
    eventquickconfig.cpp \
    filmgauge.cpp \
    frametexture.cpp \
    listselectdialog.cpp \
    main.cpp\
    mainwindow.cpp \
    FilmScan.cpp \
    project.cpp \
    propertiesdialog.cpp \
    propertylist.cpp \
    readframedpx.cpp \
    vbevent.cpp \
    vbproject.cpp \
    openglwindow.cpp \
    frame_view_gl.cpp \
    readframetiff.cpp \
    preferencesdialog.cpp \
    extractdialog.cpp \
    metadata.cpp

HEADERS  += mainwindow.h \
    FilmScan.h \
    attributelabel.h \
    decimalelidedelegate.h \
    eventdataform.h \
    eventdialog.h \
    eventfilter.h \
    eventfilterdialog.h \
    eventquickconfig.h \
    filmgauge.h \
    frametexture.h \
    listselectdialog.h \
    overlap.h \
    project.h \
    propertiesdialog.h \
    propertylist.h \
    readframedpx.h \
    DPX.h \
    DPXHeader.h \
    DPXStream.h \
    vbevent.h \
    vbproject.h \
    vfbexception.h \
    openglwindow.h \
    frame_view_gl.h \
    readframetiff.h \
    preferencesdialog.h \
    extractdialog.h \
    metadata.h

FORMS    += mainwindow.ui \
    eventdataform.ui \
    eventdialog.ui \
    eventfilterdialog.ui \
    listselectdialog.ui \
    preferencesdialog.ui \
    extractdialog.ui


win32: LIBS += -ltiff
else: LIBS += -ltiff

unix: CONFIG += link_pkgconfig

# libav libraries
LIBS += -lavcodec -lavfilter -lavformat -lavutil
LIBS += -lswscale -lswresample


# other libraries
win32:LIBS += -lDPX
else:LIBS += /home/core/data/libs/libdpx/libdpx.a

## Turn off unecessary warnings
unix: QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-private-field \
    -Wno-unused-variable -Wno-unused-parameter \
    -Wno-ignored-qualifiers -Wno-unused-function -Wno-sign-compare \
    -Wno-unused-local-typedef -Wno-reserved-user-defined-literal

RESOURCES += \
    data.qrc \
    shaders.qrc \
    license.qrc \
    images.qrc

DISTFILES += \
    logos2.png
