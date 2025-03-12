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

#include "mainwindow.h"

#include <QApplication>
#include <QtPlugin>

class VBApplication : public QApplication
{
private:
    MainWindow *mainWindow;

public:
    VBApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        setOrganizationName("USC MIRC");
        setOrganizationDomain("imi.cas.sc.edu");
        setApplicationName(APP_NAME);
        setApplicationVersion(APP_VERSION_STR);

        mainWindow = new MainWindow;
    }

    MainWindow *Win() { return mainWindow; }

    // MacOS: handle starting by clicking on a project file or
    // dragging a project file onto the app
    bool event(QEvent *event) override
    {
        if(event->type() == QEvent::FileOpen)
        {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            const QUrl url = openEvent->url();
            if(url.isLocalFile())
            {
                mainWindow->RequestOpenProject(url.toLocalFile());
                event->accept();
            }
        }

        return QApplication::event(event);
    }
};

int main(int argc, char *argv[])
{
    VBApplication a(argc, argv);

    for(int i=0; i<argc; i++) std::cerr << i << ": " << argv[i] << "\n";
    if(argc > 1) a.Win()->SetStartingProject(argv[1]);

    a.Win()->resize(704,640);
    a.Win()->show();
    return a.exec();

	// If you have any clean-up code to execute, don't put it here.
	// Connect it to the aboutToQuit() signal or use qAddPostRoutine().
}
