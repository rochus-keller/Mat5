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

#include "MatReader.h"
#include "MatParser.h"
#include <QtDebug>
using namespace Mat;

enum ArrayType { mxCELL_CLASS = 1, mxSTRUCT_CLASS = 2, mxOBJECT_CLASS = 3, mxCHAR_CLASS = 4,
				 mxSPARSE_CLASS = 5, mxDOUBLE_CLASS = 6, mxSINGLE_CLASS = 7, mxINT8_CLASS = 8,
				 mxUINT8_CLASS = 9, mxINT16_CLASS = 10, mxUINT16_CLASS = 11, mxINT32_CLASS = 12,
				 mxUINT32_CLASS = 13, mxINT64_CLASS = 14, mxUINT64_CLASS = 15,
				 mxUndocumented16 = 16, mxUndocumented17 = 17 };

MatReader::MatReader()
{
	d_parser = new MatParser();
}

MatReader::~MatReader()
{
	delete d_parser;
}

bool MatReader::setDevice(QIODevice * in, bool own)
{
	return d_parser->setDevice( in, own );
}

QVariant MatReader::nextElement()
{
	d_error.clear();
	MatParser::Token t = d_parser->nextToken();
	switch( t.d_type )
	{
	case MatParser::Value:
		return t.d_value;
	case MatParser::BeginMatrix:
		{
			QVariant v = readMatrix();
			if( !d_error.isEmpty() )
				return QVariant();
			t = d_parser->nextToken();
			if( t.d_type == MatParser::EndMatrix )
				return v;
			else
				return error("Invalid matrix end");
		}
	case MatParser::EndMatrix:
		Q_ASSERT( false );
		break;
	case MatParser::Error:
		d_error = t.d_value.toString();
		break;
	}
	return QVariant();
}

quint16 MatReader::getLimit() const
{
	return d_parser->getLimit();
}

void MatReader::setLimit(quint16 l)
{
	d_parser->setLimit(l);
}

static inline qint32 _totalCount( const QVector<qint32>& v )
{
	qint32 res = 1;
	foreach( qint32 i, v )
		res *= i;
	return res;
}

static QVariantList _toList( const QByteArray& a, bool _signed, int limit )
{
	QVariantList res;
	for( int i = 0; i < a.size() && ( limit == 0 || i < limit ); i++ )
	{
		if( _signed )
			res.append(qint32(qint8(a.at(i))));
		else
			res.append(quint32(quint8(a.at(i))));
	}
	return res;
}

static QList<QByteArray> _split( const QByteArray& str, int chunkLen )
{
	QList<QByteArray> res;
	int pos = 0;
	while( pos < str.size() )
	{
		const int to = str.indexOf( char(0), pos );
		res.append( str.mid( pos, to - pos ) );
		pos += chunkLen;
	}
	return res;
}

QVariant MatReader::readMatrix()
{
	const int limit = d_parser->getLimit();

	MatParser::Token t = d_parser->peekToken();
	if( t.d_type == MatParser::EndMatrix )
		return QVariant(); // Das kommt tatsächlich vor
	t = d_parser->nextToken(); // eat
	QVariantList l = t.d_value.toList();
	if( t.d_type != MatParser::Value || l.size() != 2 )
		return error("Invalid array flags");
	const quint32 f = l.first().toUInt();
	const bool logical = f & 0x200;
	const bool global = f & 0x400;
	const bool complex = f & 0x800;
	const int type = f & 0xff;
	const quint32 nzmax = l.last().toUInt();
	Q_UNUSED(nzmax);

	t = d_parser->nextToken();
	l = t.d_value.toList();
	if( type <= 15 && ( t.d_type != MatParser::Value || l.isEmpty() ) )
		return error("Invalid array dimensions");
	QVector<qint32> dims( l.size() );
	for( int i = 0; i < dims.size(); i++ )
		dims[i] = l[i].toInt();
	const qint32 totalCount = _totalCount( dims );

	t = d_parser->nextToken();
	if( t.d_type != MatParser::Value || t.d_value.type() != QVariant::ByteArray )
		return error("Invalid array name");
	const QByteArray name = t.d_value.toByteArray();

	switch( type )
	{
	case mxDOUBLE_CLASS:
	case mxSINGLE_CLASS:
	case mxINT16_CLASS:
	case mxUINT16_CLASS:
	case mxINT32_CLASS:
	case mxUINT32_CLASS:
	case mxINT64_CLASS:
	case mxUINT64_CLASS:
	case mxINT8_CLASS:
	case mxUINT8_CLASS:
		if( dims.size() < 2 )
			return error("At least two dimensions required");
		else
		{
			t = d_parser->nextToken();
			l.clear();
			if( t.d_value.type() == QVariant::ByteArray )
				l = _toList( t.d_value.toByteArray(), type != mxUINT8_CLASS, limit );
			else if( t.d_value.type() == QVariant::List )
				l = t.d_value.toList();
			else
				l << t.d_value;
			if( t.d_type != MatParser::Value || ( limit == 0 && l.size() != totalCount ) )
				return error("Invalid array real part");
			NumericArray a;
			a.d_valid = true;
			a.d_name = name;
			a.d_logical = logical;
			a.d_global = global;
			a.d_dims = dims;
			a.d_real = l;
			if( complex )
			{
				t = d_parser->nextToken();
				l.clear();
				if( t.d_value.type() == QVariant::ByteArray )
					l = _toList( t.d_value.toByteArray(), type != mxUINT8_CLASS, limit );
				else if( t.d_value.type() == QVariant::List )
					l = t.d_value.toList();
				else
					l << t.d_value;
				if( t.d_type != MatParser::Value || ( limit == 0 && l.size() != totalCount ) )
					return error("Invalid array complex part");
				a.d_img = l;
			}
			return QVariant::fromValue(a);
		}
		break;
	case mxSPARSE_CLASS:
		if( dims.size() < 0 || dims.size() > 2 )
			return error("Invalid sparse array dimensions");
		else
		{
			t = d_parser->nextToken(); // Row Index (ir) miINT32 nzmax * sizeOfDataType (The nzmax value is stored in Array Flags.)
			t = d_parser->nextToken(); // Column Index (jc) miINT32 (N+1) * sizeof(int32) where N is the second element of the Dimensions array subelement.
			t = d_parser->nextToken(); // Real part (pr)
			if( complex )
				t = d_parser->nextToken(); // Imaginary part (pi)
			qWarning() << "## Sparse arrays not yet supported";
			SparseArray a;
			a.d_valid = true;
			a.d_name = name;
			a.d_logical = logical;
			a.d_global = global;
			return QVariant::fromValue(a);
		}
		break;
	case mxCELL_CLASS:
		if( dims.size() < 2 )
			return error("At least two dimensions required");
		else
		{
			CellArray a;
			a.d_valid = true;
			a.d_name = name;
			a.d_logical = logical;
			a.d_dims = dims;
			a.d_global = global;
			t = d_parser->peekToken();
			if( t.d_type == MatParser::BeginMatrix )
			{
				int i = 0;
				do
				{
					t = d_parser->nextToken(); // eat
					a.d_cells.append( readMatrix() );
					if( !d_error.isEmpty() )
						return QVariant();
					t = d_parser->nextToken();
					if( t.d_type != MatParser::EndMatrix )
						return error("Invalid cell end");
					i++;
					if( limit != 0 && i >= limit )
					{
						d_parser->skipLevel();
						break;
					}
					t = d_parser->peekToken();
				}while( t.d_type == MatParser::BeginMatrix );
			}
			return QVariant::fromValue(a);
		}
		break;
	case mxCHAR_CLASS:
		{
			t = d_parser->nextToken();
			if( t.d_type != MatParser::Value || t.d_value.toString().size() != totalCount ) // Char Arrays sind unlimitiert!
				return error("Invalid char array");
			String s;
			s.d_valid = true;
			s.d_name = name;
			s.d_logical = logical;
			s.d_global = global;
			s.d_str = t.d_value.toString();
			return QVariant::fromValue(s);
		}
		break;
	case mxSTRUCT_CLASS:
		{
			t = d_parser->nextToken();
			if( t.d_type != MatParser::Value || t.d_value.type() != QVariant::Int )
				return error("Invalid struct format");
			const qint32 nameLength = t.d_value.toInt();
			t = d_parser->nextToken();
			if( t.d_type != MatParser::Value || t.d_value.type() != QVariant::ByteArray )
				return error("Invalid struct format");
			const QList<QByteArray> names = _split( t.d_value.toByteArray(), nameLength );
			Structure s;
			s.d_valid = true;
			s.d_name = name;
			s.d_logical = logical;
			s.d_global = global;

			if( !readFields( s, names ) )
				return QVariant();
			return QVariant::fromValue( s );
		}
		break;
	case mxUndocumented17:
	case mxUndocumented16:
		{
			// Folgendes ist nicht dokumentiert; habe das empirisch anhand dem fig Format ermittelt.
			Undocumented s;
			s.d_valid = true;
			s.d_name = name;
			s.d_logical = logical;
			s.d_global = global;
			if( type == mxUndocumented17 )
			{
				t = d_parser->nextToken();
				if( t.d_type != MatParser::Value )
					return error("Invalid type 17 format");
				s.d_value = t.d_value;
			}
			t = d_parser->nextToken();
			if( t.d_type != MatParser::BeginMatrix )
				return error("Invalid type 17 start");
			s.d_sub = readMatrix();
			if( !d_error.isEmpty() )
				return QVariant();
			t = d_parser->nextToken();
			if( t.d_type != MatParser::EndMatrix )
				return error("Invalid type 17 end");
			return QVariant::fromValue( s );
		}
		break;
	case mxOBJECT_CLASS:
		{
			t = d_parser->nextToken();
			if( t.d_type != MatParser::Value || t.d_value.type() != QVariant::ByteArray )
				return error("Invalid class format");
			const QByteArray className = t.d_value.toByteArray();
			t = d_parser->nextToken();
			if( t.d_type != MatParser::Value || t.d_value.type() != QVariant::Int )
				return error("Invalid class format");
			const qint32 nameLength = t.d_value.toInt();
			t = d_parser->nextToken();
			if( t.d_type != MatParser::Value || t.d_value.type() != QVariant::ByteArray )
				return error("Invalid class format");
			const QList<QByteArray> names = _split( t.d_value.toByteArray(), nameLength );
			Structure s;
			s.d_valid = true;
			s.d_name = name;
			s.d_className = className;
			s.d_logical = logical;
			s.d_global = global;
			if( !readFields( s, names ) )
				return QVariant();
			return QVariant::fromValue( s );
		}
		break;
	}
	qWarning() << "## Invalid array type" << type;
	return error("Invalid array type");
}

QVariant MatReader::error(const char * msg)
{
	d_error = msg;
	return QVariant();
}

bool MatReader::readFields(Structure & s, const QList<QByteArray>& names)
{
	int n = 0;
	const quint32 limit = d_parser->getLimit() * names.size(); // in jedem Feld max Limit
	MatParser::Token t = d_parser->peekToken();
	if( t.d_type == MatParser::BeginMatrix )
	{
		do
		{
			t = d_parser->nextToken(); // eat
			const QVariant v = readMatrix();
			if( !d_error.isEmpty() )
				return false;
			s.d_fields[ names[ n % names.size() ] ].append( v );
			t = d_parser->nextToken();
			if( t.d_type != MatParser::EndMatrix )
				return error("Invalid field end").toBool();
			n++;
			if( limit != 0 && quint32(n) >= limit )
			{
				d_parser->skipLevel();
				break;
			}
			t = d_parser->peekToken();
		}while( t.d_type == MatParser::BeginMatrix );
	}
	if( n != names.size() && n % names.size() != 0 )
		return error("Fields and names not consistent").toBool();
	return true;
}

QString Structure::getString(const QByteArray &field) const
{
	QVariantList l = d_fields.value(field);
	if( l.isEmpty() )
		return QString();
	else if( l.first().canConvert<String>() )
		return l.first().value<String>().d_str;
	else
		return l.first().toString();
}

QVariant Structure::getValue(const QByteArray &field) const
{
	QVariantList l = d_fields.value(field);
	if( l.isEmpty() )
		return QVariant();
	else
		return l.first();
}

Mat::Structure Structure::getStruct(const QByteArray &field) const
{
	return getValue(field).value<Structure>();
}

Mat::NumericArray Structure::getArray(const QByteArray &field) const
{
	return getValue(field).value<NumericArray>();
}

QVariant Structure::getArrayValue(const QByteArray &field, quint32 i) const
{
	QVariantList l = d_fields.value(field);
	if( l.isEmpty() )
		return QVariant();
	else if( l.first().canConvert<NumericArray>() )
		return l.first().value<NumericArray>().getReal(i);
	else
		return QVariant();
}

quint32 Structure::getArrayLen(const QByteArray &field) const
{
	QVariantList l = d_fields.value(field);
	if( l.isEmpty() )
		return 0;
	else if( l.first().canConvert<NumericArray>() )
		return l.first().value<NumericArray>().d_real.size();
	else
		return 0;
}

QVariant NumericArray::getReal(int i) const
{
	if( i >= d_real.size() )
		return QVariant();
	else
		return d_real[i];
}

QVariant NumericArray::getReal(int row, int col) const
{
	if( d_dims.size() != 2 )
		return QVariant();
	return getReal( row +
					col * d_dims[0] );
}

QVariant NumericArray::getReal(int row, int col, int z) const
{
	if( d_dims.size() == 2 )
	{
		if( z == 0 )
			return getReal( row, col );
		//else
		qWarning() << "NumericArray::getReal: accessing 2D array with 3D indices";
		return QVariant();
	}
	if( d_dims.size() != 3 )
		return QVariant();
	return getReal( d_dims[ 0 ] * d_dims[ 1 ] * z +
					col * d_dims[ 0 ] +
					row );
}

void NumericArray::allocReal(int rows, const QVariant &val)
{
	d_real.clear();
	d_dims.resize(1);
	d_dims[0] = rows;
	for( int i = 0; i < rows; i++ )
		d_real.append( val );
}

void NumericArray::allocReal(int rows, int cols, const QVariant &val)
{
	d_real.clear();
	d_dims.resize(2);
	d_dims[0] = rows;
	d_dims[1] = cols;
	for( int i = 0; i < ( rows * cols ); i++ )
		d_real.append( val );
}

QVariant CellArray::getValue(quint32 i) const
{
	if( int(i) < d_cells.size() )
		return d_cells[i];
	else
		return QVariant();
}

QVariant CellArray::getValue(quint32 row, quint32 col) const
{
	if( d_dims.size() != 2 )
		return QVariant();
	return getValue( row + col * d_dims[0] );
}

Structure CellArray::getStruct(quint32 row, quint32 col) const
{
	return getValue( row, col ).value<Structure>();
}

QString CellArray::getString(quint32 i) const
{
	if( int(i) >= d_cells.size() )
		return QString();
	else if( d_cells[i].canConvert<String>() )
		return d_cells[i].value<String>().d_str;
	else
		return d_cells[i].toString();
}

QString CellArray::getString(quint32 row, quint32 col) const
{
	if( d_dims.size() != 2 )
		return QString();
	return getString( row + col * d_dims[0] );
}
