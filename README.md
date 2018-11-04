# Welcome to Mat5

Mat5 is a library and viewer application to read and write MATLAB MAT Level 5 files as specified in http://www.mathworks.com/access/helpdesk/help/pdf_doc/matlab/matfile_format.pdf

## Download and Installation

You can either compile Mat5Viewer yourself or download
the pre-compiled version from here: 

http://software.rochus-keller.info/Mat5Viewer_win32.gz

http://software.rochus-keller.info/Mat5Viewer_linux_x86.tar.gz

This is a compressed single-file executable which was built using the source code from here. Since it is a single executable, it can just be downloaded and unpacked. No installation is necessary. You therefore need no special privileges to run the application on your machine. 

## How to Build Mat5Viewer

### Preconditions
Mat5 was originally developed using Qt4.x. The single-file executables are static builds based on Qt 4.4.3. But it compiles also well with the Qt 4.8 series and should also be compatible with Qt 5.x. 

You can download the Qt 4.4.3 source tree from here: http://download.qt.io/archive/qt/4.4/qt-all-opensource-src-4.4.3.tar.gz

The source tree also includes documentation and build instructions.

If you intend to do static builds on Windows without dependency on C++ runtime libs and manifest complications, follow the recommendations in this post: http://www.archivum.info/qt-interest@trolltech.com/2007-02/00039/Fed-up-with-Windows-runtime-DLLs-and-manifest-files-Here's-a-solution.html

Here is the summary on how to do implement Qt Win32 static builds:
1. in Qt/mkspecs/win32-msvc2005/qmake.conf replace MD with MT and MDd with MTd
2. in Qt/mkspecs/features clear the content of the two embed_manifest_*.prf files (but don't delete the files)
3. run configure -release -static -platform win32-msvc2005

### Build Steps
Follow these steps if you inted to build Mat5Viewer yourself (don't forget to meet the preconditions before you start):

1. Create a directory; let's call it BUILD_DIR
2. Download the Mat5Viewer source code from https://github.com/rochus-keller/Mat5/archive/master.zip and unpack it to the BUILD_DIR; rename the subdirectory to "Mat5".
3. Goto the BUILD_DIR/Mat5 subdirectory and execute `QTDIR/bin/qmake Mat5Viewer.pro` (see the Qt documentation concerning QTDIR).
4. Run make; after a couple of minutes you will find the executable in the tmp subdirectory.

Alternatively you can open Mat5Viewer.pro using QtCreator and build it there.

Note that the qtiocompressor.h/cpp files belong to another project (see file headers for license) and are deployed with this source code for convenience.

## Support
If you need support or would like to post issues or feature requests please use the Github issue list at https://github.com/rochus-keller/Mat5/issues or send an email to the author.



