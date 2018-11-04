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

INCLUDEPATH += ../

SOURCES += \
    ../Mat5/qtiocompressor.cpp \
    ../Mat5/MatWriter.cpp \
    ../Mat5/MatReader.cpp \
    ../Mat5/MatParser.cpp \
    ../Mat5/MatLexer.cpp

HEADERS  += \
    ../Mat5/qtiocompressor.h \
    ../Mat5/MatWriter.h \
    ../Mat5/MatReader.h \
    ../Mat5/MatParser.h \
    ../Mat5/MatLexer.h
