/*
* Copyright 2016-2018 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the Mat5 library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
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
*
* GNU Lesser General Public License Usage
* Alternatively, this file may be used under the terms of the GNU Lesser
* General Public License version 3 as published by the Free Software
* Foundation and appearing in the file LICENSE.LGPL included in the
* packaging of this file. Please review the following information to
* ensure the GNU Lesser General Public License version 3 requirements
* will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
*/

#include "MatWriter.h"
#include <QSysInfo>
#include <QDateTime>
#include <QBuffer>
#include <QTemporaryFile>
#include <QtDebug>
#include <QDataStream>
#include "qtiocompressor.h"
using namespace Mat;

enum DataType { miINT8 = 1, miUINT8 = 2, miINT16 = 3, miUINT16 = 4, miINT32 = 5, miUINT32 = 6,
				miSINGLE = 7, miDOUBLE = 9, miINT64 = 12, miUINT64 = 13, miMATRIX = 14,
				miCOMPRESSED = 15,
				miUTF8 = 16, miUTF16 = 17, miUTF32 = 18 };

enum ArrayType { mxCELL_CLASS = 1, mxSTRUCT_CLASS = 2, mxOBJECT_CLASS = 3, mxCHAR_CLASS = 4,
				 mxSPARSE_CLASS = 5, mxDOUBLE_CLASS = 6, mxSINGLE_CLASS = 7, mxINT8_CLASS = 8,
				 mxUINT8_CLASS = 9, mxINT16_CLASS = 10, mxUINT16_CLASS = 11, mxINT32_CLASS = 12,
				 mxUINT32_CLASS = 13, mxINT64_CLASS = 14, mxUINT64_CLASS = 15 };

static inline qint32 _totalCount( const MatWriter::Dims& v )
{
	if( v.isEmpty() )
		return 0;
	qint32 res = 1;
	foreach( qint32 i, v )
		res *= i;
	return res;
}

MatWriter::MatWriter():d_owner(false)
{
}

MatWriter::~MatWriter()
{
	release();
}

bool MatWriter::setDevice(QIODevice * out, bool own, bool _writeHeader)
{
	if( out == 0 )
		return true;
	if( !out->isOpen() )
	{
		if( !out->open(QIODevice::WriteOnly) )
			return false;
	}
	release();
	d_level.append( Level(out) );
	d_owner = own;
	if( _writeHeader )
		writeHeader();
	return true;
}

void MatWriter::beginMatrix(bool large)
{
	if( d_level.isEmpty() )
		return;

	QIODevice* out = 0;
	if( large )
	{
		QTemporaryFile* tf = new QTemporaryFile();
		tf->open();
		out = tf;
	}else
	{
		out = new QBuffer();
		out->open( QIODevice::ReadWrite );
	}
	d_level.append( Level(out) );
}

void MatWriter::endMatrix(bool compress)
{
	Q_UNUSED( compress ); // TODO
	if( d_level.size() < 2 )
		return;
	QIODevice* from = d_level.last().d_out;
	QIODevice* to = d_level[ d_level.size() - 2 ].d_out;
	const int len = from->pos();
	QByteArray buf;
	buf.resize( 0xffff );
	from->seek(0);

	if( compress )
	{
		QTemporaryFile temp;
		temp.open();
		QtIOCompressor cmp( &temp );
		cmp.open(QIODevice::WriteOnly);
		writeTag( &cmp, miMATRIX, len );
		while( !from->atEnd() )
		{
			const int read = from->read( buf.data(), buf.size() );
			cmp.write( buf.left(read) );
		}
		writePadding( &cmp, len );
		cmp.close();
		const int len2 = temp.pos();
		temp.seek(0);
		writeTag( to, miCOMPRESSED, len2 );
		while( !temp.atEnd() )
		{
			const int read = temp.read( buf.data(), buf.size() );
			to->write( buf.left(read) );
		}
	}else
	{
		writeTag( to, miMATRIX, len );
		while( !from->atEnd() )
		{
			const int read = from->read( buf.data(), buf.size() );
			to->write( buf.left(read) );
		}
		writePadding( to, len );
	}
	delete d_level.last().d_out;
	d_level.removeLast();
}

void MatWriter::beginStructure(const QList<QByteArray> &fieldNames, int rowCount, bool large, const QByteArray &name)
{
	beginMatrix( large );

	Q_ASSERT( !d_level.isEmpty() );
	Q_ASSERT( !fieldNames.isEmpty() && rowCount >= 1 );
	d_level.last().d_type.d_mxType = mxSTRUCT_CLASS;
	d_level.last().d_dims << rowCount << 1;
	writeArrayFlags( d_level.last().d_out, d_level.last().d_type.d_mxType );
	writeArrayDims( d_level.last().d_out, d_level.last().d_dims );
	d_level.last().d_dims[1] = fieldNames.size(); // nachdem gespeichert hier zweckentfremden zur Kontrolle der Zeilenbreite
	writeArrayName( d_level.last().d_out, name );
	qint32 nameLen = 0;
	foreach( const QByteArray& n, fieldNames )
	{
		if( n.size() > nameLen )
			nameLen = n.size();
	}
	nameLen = qMin( nameLen, 31 ) + 1;
	QByteArray names;
	foreach( const QByteArray& n, fieldNames )
	{
		names.append(n.left(31));
		names.append( QByteArray( nameLen - n.size(), char(0) ) );
	}
	writeTag( d_level.last().d_out, miINT32, 4 );
	write( d_level.last().d_out, nameLen );
	writeDataElement( d_level.last().d_out, miINT8, names );
}

void MatWriter::addStructureRow(const QVariantList & l)
{
	Q_ASSERT( !d_level.isEmpty() );
	if( d_level.last().d_type.d_mxType != mxSTRUCT_CLASS || d_level.last().d_dims.size() < 2 )
	{
		qWarning() << "MatWriter::writeStructureRow: writing rows without header";
		return;
	}
	if( l.size() != d_level.last().d_dims[1] )
	{
		qWarning() << "MatWriter::writeStructureRow: row has invalid number of columns";
		return;
	}
	if( d_level.last().d_dims[0] <= 0 )
	{
		qWarning() << "MatWriter::writeStructureRow: too many rows";
		return;
	}
	for( int i = 0; i < l.size() ; i++ )
		writeCell( l[i] );
	d_level.last().d_dims[0]--;
}

void MatWriter::endStructure(bool compress)
{
	if( d_level.last().d_type.d_mxType != mxSTRUCT_CLASS )
	{
		qWarning() << "MatWriter::endStructure: not a structure";
		return;
	}
	if( d_level.last().d_dims[0] > 0 )
	{
		qWarning() << "MatWriter::endStructure: not all rows written";
		return;
	}
	endMatrix( compress );
}

void MatWriter::beginNumArray(const MatWriter::Dims & dims, int numType, bool large, const QByteArray &name)
{
	Q_ASSERT( isNumeric( numType ) );
	beginMatrix(large);
	TypeLen t = matTypeFromMetaType( numType );
	const qint32 count = _totalCount(dims);
	d_level.last().d_dims << count;
	t.d_len *= count;
	d_level.last().d_type = t;
	writeArrayFlags( d_level.last().d_out, t.d_mxType );
	writeArrayDims( d_level.last().d_out, dims );
	writeDataElement( d_level.last().d_out, miINT8, name );
	writeTag( d_level.last().d_out, t.d_miType, t.d_len );
}

void MatWriter::addNumArrayElement(const QVariant & v)
{
	Q_ASSERT( !d_level.isEmpty() );
	if( !d_level.last().d_type.isNumArray() )
	{
		qWarning() << "MatWriter::addNumArrayElement: not a numeric array";
		return;
	}
	if( v.type() == QVariant::List )
	{
		// Array von Skalaren
		QVariantList l = v.toList();
		foreach( const QVariant& v, l )
		{
			if( matTypeFromMetaType( v.type() ).d_mxType != d_level.last().d_type.d_mxType )
			{
				qWarning() << "MatWriter::addNumArrayElement: incompatible element type" << v;
				return;
			}
			writeData( d_level.last().d_out, v );
		}
		d_level.last().d_dims[0] -= l.size();
	}else if( v.type() == QVariant::ByteArray )
	{
		// Array of UInt8
		if( d_level.last().d_type.d_mxType == mxUINT8_CLASS )
		{
			const QByteArray data = v.toByteArray();
			writeData( d_level.last().d_out, v );
			d_level.last().d_dims[0] -= data.size();
			return;
		}else
		{
			qWarning() << "MatWriter::addNumArrayElement: cannot add UInt8 to array type" << d_level.last().d_type.d_mxType;
			return;
		}
	}else
	{
		if( matTypeFromMetaType( v.type() ).d_mxType != d_level.last().d_type.d_mxType )
		{
			qWarning() << "MatWriter::addNumArrayElement: incompatible element type" << v;
			return;
		}
		writeData( d_level.last().d_out, v );
		d_level.last().d_dims[0] -= 1;
	}
}

void MatWriter::writeCell(const QVariant & val, const QByteArray &name)
{
	if( isString( val.type() ) )
	{
		addCharArray( val.toString(), name );
	}else if( isNumeric( val.type() ) )
	{
		Dims dims;
		dims << 1 << 1;
		beginNumArray( dims, val.type(), false, name );
		addNumArrayElement( val );
		endNumArray();
	}else if( val.type() == QVariant::ByteArray )
	{
		Dims dims;
		dims << 1 << val.toByteArray().size();
		beginNumArray( dims, QMetaType::UChar, false, name );
		addNumArrayElement( val );
		endNumArray();
	}else if( val.type() == QVariant::List )
	{
		QVariantList l = val.toList();
		if( l.isEmpty() )
		{
			qWarning() << "MatWriter::writeCell: empty lists not supported";
			return;
		}
		Dims dims;
		dims << l.size() << 1; 
		const quint32 type = l.first().type();
		bool homogeneous = true;
		for( int i = 1; i < l.size(); i++ )
		{
			if( l[i].type() != type )
			{
				homogeneous = false;
				break;
			}
		}
		if( !homogeneous || !isNumeric( type ) )
		{
			qWarning() << "MatWriter::writeCell: CellArrays not yet supported";
			return;
		}else
		{
			beginNumArray( dims, type, false, name ); 
			addNumArrayElement( val );
			endNumArray();
		}
	}else if( val.canConvert<QIODevice*>() )
	{
		QDataStream in( val.value<QIODevice*>() );
		if( in.atEnd() )
		{
			qWarning() << "MatWriter::writeCell: empty data streams not supported";
			return;
		}
		QVariantList l;
		quint32 type = QVariant::Invalid;
		bool homogeneous = true;
		while( !in.atEnd() )
		{
			QVariant v;
			in >> v;
			if( type == QVariant::Invalid )
				type = v.type();
			else if( v.type() != type )
			{
				homogeneous = false;
				break;
			}
			l.append(v);
		}
		if( !homogeneous || !isNumeric( type ) )
		{
			qWarning() << "MatWriter::writeCell: CellArrays not yet supported";
			return;
		}else
		{
			Dims dims;
			dims << l.size() << 1;
			beginNumArray( dims, type, false, name ); 
			addNumArrayElement( l );
			endNumArray();
		}
	}else
	{
		qWarning() << "MatWriter::writeCell: value type not yet supported" << val;
	}
}

void MatWriter::release()
{
	for( int i = 0; i < d_level.size(); i++ )
	{
		if( i != 0 || d_owner )
			delete d_level[i].d_out;
	}
	d_level.clear();
	d_owner = false;
}

void MatWriter::writeHeader()
{
	Q_ASSERT( !d_level.isEmpty() );
	QString platform;
#if defined(Q_OS_WIN32)
	platform = "WIN32";
#elif defined(Q_OS_WIN64)
	platform = "WIN64";
#elif defined(Q_OS_LINUX)
	platform = "Linux";
	platform += QString::number(QSysInfo::WordSize);
#elif defined(Q_OS_DARWIN64)
	platform = "OSX64";
#elif defined(Q_OS_DARWIN32)
	platform = "OSX32";
#else
	platform = "?";
	qWarning() << "MatWriter::writeHeader: unknown operating system";
#endif
	platform += QChar(' ');
	if( QSysInfo::BigEndian )
		platform += "BE";
	else
		platform += "LE";

	const QByteArray header = QString("MATLAB 5.0 MAT-file, Platform: %1, Created on: %2").
            arg( platform ).arg( QDateTime::currentDateTime().toString( "ddd MMM d hh:mm:ss yyyy" ) ).toUtf8();
	d_level.last().d_out->write( header );
	d_level.last().d_out->write( QByteArray( 116 - header.size(), char(0) ) );
	d_level.last().d_out->write( QByteArray( 8, char(0) ) ); // Header Subsystem Data Offset Field
	write( d_level.last().d_out, quint16(0x0100) );
	write( d_level.last().d_out, quint16(0x4d49) );
}

void MatWriter::writeDataElement( QIODevice* out, quint16 miType, const QByteArray & in)
{
	const int len = in.size();
	writeTag( out, miType, len );
	out->write(in);
	writePadding( out, len );
}

void MatWriter::writeTag(QIODevice* out , quint8 miType, quint32 byteLen )
{
	if( byteLen <= 4 )
	{
		const quint32 val = miType + ( byteLen << 16 );
		write( out, val );
	}else
	{
		write( out, qint32( miType ) );
		write( out, qint32( byteLen ) );
	}
}

void MatWriter::writePadding(QIODevice* out, int len)
{
	if( len <= 4 )
	{
		out->write( QByteArray( 4 - len, char(0) ) );
	}else
	{
		const int padding = ( 8 - ( len % 8 ) ) % 8;
		out->write( QByteArray( padding, char(0) ) );
	}
}

void MatWriter::writeData(QIODevice * out, const QVariant & v)
{
	Q_ASSERT( v.type() != QVariant::List );
	switch( int(v.type()) )
	{
	case QVariant::ByteArray:
		out->write( v.toByteArray() );
		break;
	case QVariant::Char:
	case QVariant::String:
		out->write( v.toString().toUtf8() );
		break;
	case QVariant::Double:
		write( out, v.toDouble() );
		break;
	case QVariant::Int:
		write( out, v.toInt() );
		break;
	case QVariant::LongLong:
		write( out, v.toLongLong() );
		break;
	case QVariant::UInt:
		write( out, v.toUInt() );
		break;
	case QVariant::ULongLong:
		write( out, v.toULongLong() );
		break;
	case QVariant::Bool:
		write( out, quint8( v.toBool()) );
		break;
	case QMetaType::UChar:
		write( out, quint8( v.toUInt() ) );
		break;
	case QMetaType::Float:
		write( out, v.value<float>() );
		break;
	case QMetaType::Short:
		write( out, v.value<short>() );
		break;
	case QMetaType::UShort:
		write( out, v.value<ushort>() );
		break;
	case QMetaType::Long:
		write( out, v.value<long>() );
		break;
	case QMetaType::ULong:
		write( out, v.value<ulong>() );
		break;
	default:
		qWarning() << "MatWriter::writeData: variant type not supported" << v;
		break;
	}
}

void MatWriter::writeArrayDims(QIODevice * out, const Dims & dims)
{
	const int len = 4 * dims.size();
	writeTag( out, miINT32, len );
	foreach( qint32 d, dims )
		write( out, d );
	writePadding( out, len );
}

void MatWriter::writeArrayFlags(QIODevice * out, quint16 type)
{
	writeTag( out, miUINT32, 2 * 4 );
	write( out, qint32(type) );
	write( out, qint32( 0 ) );
}

void MatWriter::writeArrayName(QIODevice * out, const QByteArray & name)
{
	writeDataElement( out, miINT8, name );
}

MatWriter::TypeLen MatWriter::matTypeFromMetaType(int metaType )
{
	switch( metaType )
	{
	case QVariant::ByteArray:
		return TypeLen( miUINT8, mxUINT8_CLASS, 0 );
	case QVariant::Char:
	case QVariant::String:
		return TypeLen( miUTF8, mxCHAR_CLASS, 0 );
	case QVariant::Double:
		return TypeLen( miDOUBLE, mxDOUBLE_CLASS, 8 );
	case QVariant::Int:
	case QMetaType::Long:
		return TypeLen( miINT32, mxINT32_CLASS, 4 );
	case QVariant::LongLong:
		return TypeLen( miINT64, mxINT64_CLASS, 8 );
	case QVariant::UInt:
	case QMetaType::ULong:
		return TypeLen( miUINT32, mxUINT32_CLASS, 4 );
	case QVariant::ULongLong:
		return TypeLen( miUINT64, mxUINT64_CLASS, 8 );
	case QVariant::Bool:
	case QMetaType::UChar:
		return TypeLen( miUINT8, mxUINT8_CLASS, 1 );
	case QMetaType::Float:
		return TypeLen( miSINGLE, mxSINGLE_CLASS, 4 );
	case QMetaType::Short:
		return TypeLen( miINT16, mxINT16_CLASS, 2 );
	case QMetaType::UShort:
		return TypeLen( miUINT16, mxUINT16_CLASS, 2 );
	default:
		qWarning() << "MatWriter::matTypeFromMetaType: variant type not supported" << metaType;
		return TypeLen(0,0,0);
	}
}

bool MatWriter::isNumeric(int metaType)
{
	switch( metaType )
	{
	case QVariant::Double:
	case QVariant::Int:
	case QVariant::LongLong:
	case QVariant::UInt:
	case QVariant::ULongLong:
	case QVariant::Bool:
	case QMetaType::UChar:
	case QMetaType::Float:
	case QMetaType::Short:
	case QMetaType::UShort:
	case QMetaType::Long:
	case QMetaType::ULong:
		return true;
	default:
		return false;
	}
}

bool MatWriter::isString(int metaType)
{
	switch( metaType )
	{
	case QVariant::Char:
	case QVariant::String:
		return true;
	default:
		return false;
	}
}

void MatWriter::addCharArray(const QString & str, const QByteArray &name)
{
	const QByteArray utf8 = str.toUtf8();

	Dims dims;
	dims << 1 << str.size();

	beginMatrix(false);
	writeArrayFlags( d_level.last().d_out, mxCHAR_CLASS );
	writeArrayDims( d_level.last().d_out, dims );
	writeDataElement( d_level.last().d_out, miINT8, name );
	writeDataElement( d_level.last().d_out, miUTF8, utf8 );
	endMatrix();

}

void MatWriter::endNumArray(bool compress)
{
	if( !d_level.last().d_type.isNumArray() )
	{
		qWarning() << "MatWriter::endNumArray: not a numeric array";
		return;
	}
	if( d_level.last().d_dims[0] > 0 )
	{
		qWarning() << "MatWriter::endNumArray: not all elements written";
		return;
	}
	writePadding( d_level.last().d_out, d_level.last().d_type.d_len );
	endMatrix( compress );
}

bool MatWriter::TypeLen::isNumArray() const
{
	return d_mxType >= mxDOUBLE_CLASS && d_mxType <= mxUINT64_CLASS;
}
