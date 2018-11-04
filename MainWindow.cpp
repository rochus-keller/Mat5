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

#include "MainWindow.h"
#include "MatParser.h"
#include "MatReader.h"
#include <QtDebug>
#include <QFile>
#include <QTreeWidget>
#include <QTabWidget>
#include <QTextBrowser>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QTextStream>
#include <QFontMetrics>
#include <QAction>
#include <QInputDialog>
#include <ctype.h>
using namespace Mat;

enum TabIndex { _TreeTab, _LogTab, _TextTab, _ArrayTab };
enum Columns { _NameCol, _TypeCol, _ValueCol };

static QString _formatList( const QByteArray& a )
{
	QString str;
	QTextStream out( &str );
	for( int i = 0; i < a.size(); i++ )
		out << int(quint8(a[i])) << " ";
	return str;
}

static QString _formatList( const QVariantList& l )
{
	QString str;
	QTextStream out( &str );
	for( int i = 0; i < l.size(); i++ )
	{
		out << l[i].toString() << "  ";
	}
	return str;
}

static QString _indent( int level )
{
	QString str;
	for( int i = 0; i < level; i++ )
	{
		if( i != 0 )
			str += QChar('|');
		str += QChar('\t');
	}
	return str;
}

static bool _isPrintable( const QByteArray& a )
{
	for( int i = 0; i < a.size(); i++ )
	{
		if( a[i] != 0 && !::isprint( a[i] ) )
			return false;
	}
	return true;
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent),d_limit(50)
{
	d_tab = new QTabWidget(this);
	setCentralWidget( d_tab );

	d_tree = new QTreeWidget(this);
	d_tree->setHeaderLabels( QStringList() << tr("Name") << tr("Type") << tr("Value") );
	d_tree->setAlternatingRowColors(true);
	d_tree->setAllColumnsShowFocus(true);
	connect( d_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onDblClick(QTreeWidgetItem*,int)) );
	d_tab->addTab( d_tree, tr("Tree") );
	d_log = new QTextBrowser(this);
	d_log->setLineWrapMode( QTextEdit::NoWrap );
	d_log->setTabStopWidth( d_log->fontMetrics().width(QLatin1String("WWW")) );
	d_tab->addTab( d_log, tr("Log") );
	d_text = new QTextBrowser(this);
	QFont f;
	f.setPointSize(11);
	f.setStyleHint(QFont::TypeWriter);
	d_text->setFont( f );
	d_tab->addTab( d_text, tr("Text") );
	d_list = new QListWidget(this);
	d_list->setFlow( QListView::LeftToRight );
	d_list->setWrapping(true);
	d_list->setAlternatingRowColors(true);
	d_tab->addTab( d_list, tr("Array") );

	QMenuBar* mb = menuBar();

	QMenu* fm = mb->addMenu(tr("&File"));
	fm->addAction(tr("Open..."), this, SLOT(onOpen()), tr("CTRL+O") );
	fm->addAction(tr("Save log..."), this, SLOT(onSaveLog()) );
	fm->addAction(tr("Save text..."), this, SLOT(onSaveText()) );
	fm->addAction(tr("Save array..."), this, SLOT(onSaveArray()) );
	fm->addSeparator();
	fm->addAction(tr("Quit"), this, SLOT(close()), tr("CTRL+Q") );

	QMenu* am = mb->addMenu(tr("&Actions"));
	am->addAction(tr("Parse to log"), this, SLOT(onParseToLog()) );
	am->addAction(tr("Set max. array size..."), this, SLOT(onSetLimit()) );
	am->addSeparator();
	am->addAction(tr("Expand all"), d_tree, SLOT(expandAll()) );
	am->addAction(tr("Expand selected"), this, SLOT(onExpandSelected()), tr("CTRL+E") );
	am->addAction(tr("Collapse all"), d_tree, SLOT(collapseAll()) );
	am->addSeparator();
	am->addAction(tr("Show value"), this, SLOT(onShowValue()), tr("CTRL+Return") );
	am->addAction(tr("Find name..."), this, SLOT(onFindName()), tr("CTRL+F") );
	am->addAction(tr("Find value..."), this, SLOT(onFindValue()), tr("CTRL+SHIFT+F") );
	am->addAction(tr("Find again"), this, SLOT(onFindAgain()), tr("F3") );

	QMenu* hm = mb->addMenu(tr("&Help"));
	hm->addAction(tr("About..."), this, SLOT(onAbout()) );

	d_tree->setContextMenuPolicy( Qt::ActionsContextMenu );
	QAction* a = new QAction(tr("Expand all"), d_tree );
	connect( a, SIGNAL(triggered()), d_tree, SLOT(expandAll()));
	d_tree->addAction( a );
	a = new QAction(tr("Expand selected"), d_tree );
	connect( a, SIGNAL(triggered()), this, SLOT(onExpandSelected()) );
	d_tree->addAction( a );
	a = new QAction(tr("Collapse all"), d_tree );
	connect( a, SIGNAL(triggered()), d_tree, SLOT(collapseAll()) );
	d_tree->addAction( a );

	clearAll();
}

MainWindow::~MainWindow()
{
	
}

bool MainWindow::showFile(const QString &path)
{
	const QString title = tr("Open MAT5 File");
	QFile file(path);
	if( !file.open(QIODevice::ReadOnly) )
	{
		QMessageBox::critical( this, title, tr("Cannot open file for reading:\n%1").arg(path) );
		return false;
	}
	MatReader r;
	r.setLimit(d_limit);
	if( !r.setDevice(&file) )
	{
		QMessageBox::critical( this, title, tr("The file has an invalid format:\n%1").arg(path) );
		return false;
	}
	clearAll();
	d_fileName = path;
	d_log->append( tr("Parsing file '%1'").arg(path) );
	if( d_limit )
		d_log->append( tr("Array lengths are limited to %1 elements!").arg(d_limit) );

	QApplication::setOverrideCursor( Qt::WaitCursor );
	QVariantList elems;
	QVariant v = r.nextElement();
	int errs = 0;
	if( r.hasError() )
	{
		d_log->append( tr("##Error: %1").arg( r.getError() ) );
		errs++;
	}
	while( v.isValid() )
	{
		elems.append(v);
		d_log->append( tr("Parsed '%1' %2").arg(v.typeName()).arg( v.toString() ) );
		v = r.nextElement();
		if( r.hasError() )
		{
			d_log->append( tr("##Error: %1").arg( r.getError() ) );
			errs++;
		}
	}
	buildTree( elems );
	QApplication::restoreOverrideCursor();
	if( errs )
	{
		d_tab->setCurrentIndex( _LogTab );
		QMessageBox::critical( this, title, tr("There were parsing errors.\nOnly parts of the file could be read.") );
		d_log->append( tr("Parsing completed with errors") );
	}else
	{
		d_tab->setCurrentIndex( _TreeTab );
		d_log->append( tr("Parsing completed successfully") );
	}
	setWindowTitle( tr("%1 - MAT5 Viewer").arg( QFileInfo(path).fileName() ) );
	return true;
}

bool MainWindow::parseToLog(const QString &path)
{
	QFile file(path);
	if( !file.open(QIODevice::ReadOnly) )
	{
		QMessageBox::critical( this, tr("Parsing to Log"), tr("Cannot open file for reading:\n%1").arg(path) );
		return false;
	}
	d_log->clear();
	d_tab->setCurrentIndex( _LogTab );
	d_log->append( tr("Parsing file '%1'").arg(path) );
	if( d_limit )
		d_log->append( tr("Array lengths are limited to %1 elements!").arg(d_limit) );
	MatParser p;
	p.setLimit(d_limit);
	if( !p.setDevice(&file) )
	{
		d_log->append( tr("The file has an invalid format") );
		return false;
	}
	QString str;
	QTextStream out(&str);
	QApplication::setOverrideCursor( Qt::WaitCursor );
	MatParser::Token t = p.nextToken();
	int level = 0;
	bool flags = false;
	while( t.d_type != MatParser::Null )
	{
		const QString indent = _indent( level );
		switch( t.d_type )
		{
		case MatParser::Value:
			if( flags )
			{
				QVariantList l = t.d_value.toList();
				Q_ASSERT( !l.isEmpty() );
				const quint32 f = l.first().toUInt();
				QString tmp;
				if( f & 0x200 )
					tmp += "logical ";
				if( f & 0x400 )
					tmp += "global ";
				if( f & 0x800 )
					tmp += "complex ";
				if( tmp.isEmpty() )
					tmp = "<none>";
				out << indent << tr("Class: %1").arg( f & 0xff ) << endl;
				out << indent << tr("Flags: %1").arg( tmp ) << endl;
				flags = false;
			}else if( t.d_value.type() == QVariant::ByteArray )
			{
				QByteArray a = t.d_value.toByteArray();
				out << indent ;
				if( a.isEmpty() )
					out << "Value: <none>";
				else if( _isPrintable(a) )
				{
					a.replace( char(0), ' ');
					out << tr("Value (String):  %1").arg( a.simplified().data() );
				}else
					out << tr("Value (Array): [  %1]").arg( _formatList( a ) );
				out << endl;
			}else if( t.d_value.type() == QVariant::List )
				out << indent << tr("Value (Array): [  %1]").arg( _formatList( t.d_value.toList() ) ) << endl;
			else
				out << indent << tr("Value (%1):  %2").arg( t.d_value.typeName() ).arg(
						   t.d_value.toString().simplified() ) << endl;
			break;
		case MatParser::BeginMatrix:
			out << indent << tr("Begin Matrix") << endl;
			level++;
			flags = true;
			break;
		case MatParser::EndMatrix:
			out << indent << tr("End Matrix") << endl;
			level--;
			break;
		case MatParser::Error:
			out << tr("### Error: %1").arg( t.d_value.toString() ) << endl;
			break;
		default:
			break;
		}

		t = p.nextToken();
	}
	d_log->append( str );
	d_log->moveCursor( QTextCursor::Start );
	d_log->ensureCursorVisible();
	QApplication::restoreOverrideCursor();
	return true;
}

void MainWindow::onOpen()
{
	const QString path = QFileDialog::getOpenFileName( this, tr("Select MAT5 File"), QFileInfo(d_fileName).absolutePath() );
	if( path.isEmpty() )
		return;

	showFile(path);
}

void MainWindow::onParseToLog()
{
	parseToLog(d_fileName);
}

void MainWindow::onAbout()
{
	QMessageBox::about( this, tr("MAT5 Viewer"),
	  tr("<html>Release: %1   Date: %2<br><br>"

	  "MAT5 Viewer can be used to inspect MATLAB MAT Level 5 files.<br>"
	  "See <a href=\"http://www.mathworks.com/access/helpdesk/help/pdf_doc/matlab/matfile_format.pdf\">"
		 "mathworks.com/.../matlab/matfile_format.pdf</a> for details.<br><br>"

	  "Author: Rochus Keller, me@rochus-keller.info<br><br>"

	  "Terms of use:<br>"
	  "The software and documentation are provided <em>as is</em>, without warranty of any kind, "
	  "expressed or implied, including but not limited to the warranties of merchantability, "
	  "fitness for a particular purpose and noninfringement. In no event shall the author or copyright holders be "
	  "liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, "
	  "arising from, out of or in connection with the software or the use or other dealings in the software.</html>" ).
						arg( "0.5" ).arg( "2016-05-23" ));
}

static void _expand( QTreeView* tv, const QModelIndex& index, bool expand )
{
	if( !index.isValid() )
		return;
	const int count = index.model()->rowCount( index );
	if( count == 0 )
		return;

	if( expand )
		// Öffne von oben nach unten
		tv->setExpanded( index, true );
	for( int i = 0; i < count; i++ )
		_expand( tv, index.model()->index( i, 0, index ), expand );
	if( !expand )
		// Gehe zuerst runter, dann von unten nach oben schliessen
		tv->setExpanded( index, false );
}

void MainWindow::onExpandSelected()
{
	if( d_tree->currentIndex().isValid() )
		_expand( d_tree, d_tree->currentIndex(), true );
}

void MainWindow::onDblClick(QTreeWidgetItem *item, int column)
{
	if( item != 0 && column == _ValueCol && item->data( _ValueCol, Qt::UserRole ).isValid() )
	{
		QVariant v = item->data( _ValueCol, Qt::UserRole );
		if( v.type() == QVariant::List )
		{
			d_list->clear();
			QVariantList l = v.toList();
			foreach( const QVariant& v, l )
				d_list->addItem( v.toString() );
			d_tab->setCurrentIndex( _ArrayTab );
		}else
		{
			d_text->clear();
			d_text->setPlainText( v.toString() );
			d_tab->setCurrentIndex( _TextTab );
		}
	}
}

void MainWindow::onSaveLog()
{
	const QString title = tr("Save log");
	QString path = QFileDialog::getSaveFileName( this, title, QFileInfo(d_fileName).absolutePath(), "*.txt" );
	if( path.isEmpty() )
		return;

	if( QFileInfo(path).suffix().isEmpty() )
		path += QLatin1String(".txt");
	QFile file(path);
	if( !file.open(QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( this, title, tr("Cannot open file for writing:\n%1").arg(path) );
		return;
	}
	QTextStream out(&file);
	out.setCodec("UTF-8");
	out << d_log->toPlainText();
}

void MainWindow::onSaveText()
{
	const QString title = tr("Save text");
	QString path = QFileDialog::getSaveFileName( this, title, QFileInfo(d_fileName).absolutePath(), "*.txt" );
	if( path.isEmpty() )
		return;

	if( QFileInfo(path).suffix().isEmpty() )
		path += QLatin1String(".txt");
	QFile file(path);
	if( !file.open(QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( this, title, tr("Cannot open file for writing:\n%1").arg(path) );
		return;
	}
	QTextStream out(&file);
	out.setCodec("UTF-8");
	out << d_text->toPlainText();
}

void MainWindow::onSaveArray()
{
	const QString title = tr("Save array");
	QString path = QFileDialog::getSaveFileName( this, title, QFileInfo(d_fileName).absolutePath(), "*.txt" );
	if( path.isEmpty() )
		return;

	if( QFileInfo(path).suffix().isEmpty() )
		path += QLatin1String(".txt");
	QFile file(path);
	if( !file.open(QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( this, title, tr("Cannot open file for writing:\n%1").arg(path) );
		return;
	}
	QTextStream out(&file);
	out.setCodec("UTF-8");
	for( int i = 0; i < d_list->count(); i++ )
		out << d_list->item(i)->text() << endl;
}

void MainWindow::onShowValue()
{
	onDblClick( d_tree->currentItem(), _ValueCol );
}

void MainWindow::onFindName()
{
	const QString str = QInputDialog::getText( this, tr("Find name"), tr("Enter text (case insensitive):") );
	if( str.isEmpty() )
		return;

	d_curFound = 0;
	d_found = d_tree->findItems( str, Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchFixedString, _NameCol );
	if( !d_found.isEmpty() )
	{
		d_tree->setCurrentItem( d_found.first() );
		d_tree->scrollToItem( d_found.first() );
		d_tab->setCurrentIndex(_TreeTab);
	}
}

void MainWindow::onFindValue()
{
	const QString str = QInputDialog::getText( this, tr("Find value"), tr("Enter text (case insensitive):") );
	if( str.isEmpty() )
		return;

	d_curFound = 0;
	d_found = d_tree->findItems( str, Qt::MatchContains | Qt::MatchRecursive | Qt::MatchFixedString, _ValueCol );
	if( !d_found.isEmpty() )
	{
		d_tree->setCurrentItem( d_found.first() );
		d_tree->scrollToItem( d_found.first() );
		d_tab->setCurrentIndex(_TreeTab);
	}
}

void MainWindow::onFindAgain()
{
	if( d_found.isEmpty() )
		return;
	const int pos = d_found.indexOf( d_tree->currentItem() );
	if( pos == -1 )
	{
		d_tree->setCurrentItem( d_found[d_curFound] );
		d_tree->scrollToItem( d_found[d_curFound] );
		d_curFound = ( d_curFound + 1 ) % d_found.size();
	}else
	{
		d_curFound = ( pos + 1 ) % d_found.size();
		d_tree->setCurrentItem( d_found[d_curFound] );
		d_tree->scrollToItem( d_found[d_curFound] );
	}
	d_tab->setCurrentIndex(_TreeTab);
}

void MainWindow::onSetLimit()
{
	bool ok;
	const int limit = QInputDialog::getInteger( this, tr("Set Array Element Limit"), tr("Maximum number of elements (0..all):"),
							  d_limit, 0, 32000, 1, &ok );
	if( !ok || d_limit == limit )
		return;
	d_limit = limit;
	const QString path = d_fileName; // speichere den path da in showFile clear aufgerufen wird
	if( !path.isEmpty() )
		showFile( path );
}

void MainWindow::clearAll()
{
	setWindowTitle( tr("MAT5 Viewer") );
	d_log->clear();
	d_text->clear();
	d_tree->clear();
	d_list->clear();
	d_fileName.clear();
	d_found.clear();
}

void MainWindow::buildTree(const QVariantList & l)
{
	foreach( const QVariant& v, l )
		createItem( 0, v );
}

static inline QString _nameOrEmpty( const QByteArray& name )
{
	if( name.isEmpty() )
		return QLatin1String("<unnamed>");
	else
		return name;
}

QString _formatValue( const QVariant& v )
{
	const int maxlen = 100;
	QString res;
	if( v.type() == QVariant::List )
		res = _formatList( v.toList() );
	else
		res = v.toString().simplified();
	if( res.size() > maxlen )
	{
		res = res.left(maxlen);
		res += "...";
	}
	return res;
}

static inline QString _dims( const QVector<qint32>& dims )
{
	QString res = "Dimensions: ";
	foreach( quint32 d, dims )
		res += QString::number(d) + QChar(' ');
	return res;
}

QTreeWidgetItem* MainWindow::createItem(QTreeWidgetItem *p, const QVariant &v)
{
	QTreeWidgetItem* item = 0;
	if( p == 0 )
		item = new QTreeWidgetItem( d_tree );
	else
		item = new QTreeWidgetItem( p );
	if( v.canConvert<Mat::NumericArray>())
	{
		Mat::NumericArray m = v.value<Mat::NumericArray>();
		item->setData( _NameCol, Qt::UserRole, v );
		item->setText( _NameCol, _nameOrEmpty(m.d_name) );
		item->setText( _TypeCol, "NumArray" );
		item->setText( _ValueCol, _dims( m.d_dims ) );
		QTreeWidgetItem* real = createItem( item, m.d_real );
		real->setText( _NameCol, "#real");
		if( !m.d_img.isEmpty() )
		{
			QTreeWidgetItem* img = createItem( item, m.d_img );
			img->setText( _NameCol, "#imaginary");
		}
	}else if( v.canConvert<Mat::String>())
	{
		Mat::String m = v.value<Mat::String>();
		item->setData( _NameCol, Qt::UserRole, v );
		item->setText( _NameCol, _nameOrEmpty(m.d_name) );
		item->setText( _TypeCol, "CharArray" );
		item->setText( _ValueCol, _formatValue( m.d_str.simplified() ) );
		item->setData( _ValueCol, Qt::UserRole, m.d_str );

	}else if( v.canConvert<Mat::Structure>())
	{
		Mat::Structure m = v.value<Mat::Structure>();
		item->setData( _NameCol, Qt::UserRole, v );
		item->setText( _NameCol, _nameOrEmpty(m.d_name) );
		if( m.isObject() )
		{
			item->setText( _ValueCol, QLatin1String("Class: ") + m.d_className );
			item->setText( _TypeCol, "Object" );
		}else
			item->setText( _TypeCol, "Structure" );
		QMap<QByteArray,QVariantList>::const_iterator i;
		for( i = m.d_fields.begin(); i != m.d_fields.end(); ++i )
		{
			QVariantList l = i.value();
			if( l.size() == 1 )
			{
				QTreeWidgetItem* sub = createItem( item, l.first() );
				sub->setText( _NameCol, i.key() );
			}else
			{
				QTreeWidgetItem* sub = new QTreeWidgetItem( item );
				sub->setText( _TypeCol, "Column" );
				sub->setText( _NameCol, i.key() );
				for( int i = 0; i < l.size(); i++ )
				{
					QTreeWidgetItem* subsub = createItem( sub, l[i] );
					subsub->setText( _NameCol, QString("#%1").arg( i+1, 3, 10, QChar('0') ) ); // RISK
				}
			}
		}
	}else if( v.canConvert<Mat::CellArray>())
	{
		Mat::CellArray m = v.value<Mat::CellArray>();
		item->setData( _NameCol, Qt::UserRole, v );
		item->setText( _NameCol, _nameOrEmpty(m.d_name) );
		item->setText( _TypeCol, "CellArray" );
		item->setText( _ValueCol, _dims( m.d_dims ) );
		for( int i = 0; i < m.d_cells.size(); i++ )
		{
			QTreeWidgetItem* sub = createItem( item, m.d_cells[i] );
			sub->setText( _NameCol, QString("#%1").arg( i+1, 3, 10, QChar('0') ) ); // RISK
		}
	}else if( v.canConvert<Mat::SparseArray>())
	{
		Mat::SparseArray m = v.value<Mat::SparseArray>();
		item->setData( _NameCol, Qt::UserRole, v );
		item->setText( _NameCol, _nameOrEmpty(m.d_name) );
		item->setText( _TypeCol, "SparseArray" );
		item->setText( _ValueCol, "<not yet supported>" );

	}else if( v.canConvert<Mat::Undocumented>())
	{
		Mat::Undocumented m = v.value<Mat::Undocumented>();
		item->setData( _NameCol, Qt::UserRole, v );
		item->setText( _NameCol, _nameOrEmpty(m.d_name) );
		item->setText( _TypeCol, "Undocumented" );
		item->setText( _ValueCol, _formatValue( m.d_value ) );
		item->setData( _ValueCol, Qt::UserRole, m.d_value );
		createItem( item, m.d_sub );
	}else
	{
		item->setText( _NameCol, _nameOrEmpty(QByteArray()) );
		if( v.type() == QVariant::List )
			item->setText( _TypeCol, "Array" );
		else
			item->setText( _TypeCol, v.typeName() );
		item->setText( _ValueCol, _formatValue(v) );
		item->setData( _ValueCol, Qt::UserRole, v );
	}
	return item;
}
