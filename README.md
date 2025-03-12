# Virtual Film Bench

# Develop Requirements
Download the Qt installer from
	https://www.qt.io/download-qt-installer.
Run the installer	
	chmod +x qt*.run 
	./qt*.run.
	
	Sign up or sign in at using Qt account.

	Install Qt Creator

Run Qtmaintaince Tools to add multimedia support.

Install additional Libs
	sudo apt-get install ffmpeg
	sudo apt-get install libtiff5-dev libavfilter-dev 
	sudo apt-get install libswscale-dev

Build libDPX 
	https://github.com/PatrickPalmer/dpx
	get libdpx.a

Build DSPFilter
	https://github.com/vinniefalco/DSPFilters
	get libDSPFilters.a

Configure the aeogui.pro
	change the INCLUDEPATH to your own include path.
	change the LIBS to your path

Build the project.


