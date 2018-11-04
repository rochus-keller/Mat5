#/*
#* Copyright 2016-2018 Rochus Keller <mailto:me@rochus-keller.info>
#*
#* This file is part of the Mat5Viewer application.
#*
#* The following is the license that applies to this copy of the
#* application. For a license to use the application under conditions
#* other than those described here, please email to me@rochus-keller.info.
#*
#* GNU General Public License Usage
#* This file may be used under the terms of the GNU General Public
#* License (GPL) versions 2.0 or 3.0 as published by the Free Software
#* Foundation and appearing in the file LICENSE.GPL included in
#* the packaging of this file. Please review the following information
#* to ensure GNU General Public Licensing requirements will be met:
#* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
#* http://www.gnu.org/copyleft/gpl.html.
#*/

QT       += core gui

TARGET = Mat5Viewer
TEMPLATE = app

win32 {
    INCLUDEPATH += $$[QT_INSTALL_PREFIX]/include/zlib
	DEFINES -= UNICODE
 }else {
	DESTDIR = ./tmp
	OBJECTS_DIR = ./tmp
	CONFIG(debug, debug|release) {
		DESTDIR = ./tmp-dbg
		OBJECTS_DIR = ./tmp-dbg
		DEFINES += _DEBUG
	}
	RCC_DIR = ./tmp
	UI_DIR = ./tmp
	MOC_DIR = ./tmp
 }


SOURCES += main.cpp\
        MainWindow.cpp \
    MatLexer.cpp \
    MatParser.cpp \
    MatReader.cpp \
    qtiocompressor.cpp

HEADERS  += MainWindow.h \
    MatLexer.h \
    MatParser.h \
    MatReader.h \
    qtiocompressor.h
