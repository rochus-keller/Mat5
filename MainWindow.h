#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

#include <QMainWindow>
#include <QVariant>

class QTabWidget;
class QTextBrowser;
class QTreeWidget;
class QListWidget;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();
	bool showFile( const QString& path );
	bool parseToLog( const QString& path );
	void setLimit( quint16 l ) { d_limit = l; }
protected slots:
	void onOpen();
	void onParseToLog();
	void onAbout();
	void onExpandSelected();
	void onDblClick( QTreeWidgetItem * item, int column );
	void onSaveLog();
	void onSaveText();
	void onSaveArray();
	void onShowValue();
	void onFindName();
	void onFindValue();
	void onFindAgain();
	void onSetLimit();
protected:
	void clearAll();
	void buildTree( const QVariantList& );
	QTreeWidgetItem *createItem( QTreeWidgetItem* p, const QVariant& v );
private:
	QTabWidget* d_tab;
	QTextBrowser* d_log;
	QTextBrowser* d_text;
	QTreeWidget* d_tree;
	QListWidget* d_list;
	QString d_fileName;
	QList<QTreeWidgetItem *> d_found;
	int d_curFound;
	quint16 d_limit;
};

#endif // MAINWINDOW_H
