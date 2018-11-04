/*
* Copyright 2016-2018 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the Mat5Viewer application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.info.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QtGui/QApplication>
#include <QFileInfo>
#include <QTime>
#include <QtDebug>
#include "MainWindow.h"
#include "MatParser.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	const QStringList args = QCoreApplication::arguments();
	QString path;
	for( int i = 1; i < args.size(); i++ ) // arg 0 enthält Anwendungspfad
	{
		if( !args[ i ].startsWith( '-' ) )
		{
			path = args[ i ];
			break;
		}
	}

	MainWindow w;
	w.showMaximized();
	if( !path.isEmpty() )
	{
		QTime start = QTime::currentTime();
		qDebug() << "Start at " << start;
		QFile file( path );
		Mat::MatParser p;
		p.setLimit(0);
		p.setDevice( &file );
		Mat::MatParser::Token t = p.nextToken();
		int i = 0;
		while( t.d_type != Mat::MatParser::Null && t.d_type != Mat::MatParser::Error )
		{
			i++;
			t = p.nextToken();
		}
		if( t.d_type != Mat::MatParser::Error )
			qDebug() << "Error" << t.d_value.toString() << start.secsTo(QTime::currentTime()) << i;
		else
			qDebug() << "Success" << start.secsTo(QTime::currentTime()) << i;
		return 0;

		if( QFileInfo(path).size() < 50000 )
			w.setLimit(0);
		w.showFile(path);
	}
	
	return a.exec();
}
