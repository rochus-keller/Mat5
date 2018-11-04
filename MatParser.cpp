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

#include "MatParser.h"
#include "MatLexer.h"
#include <QBuffer>
#include <QVector>
using namespace Mat;

enum DataType { miINT8 = 1, miUINT8 = 2, miINT16 = 3, miUINT16 = 4, miINT32 = 5, miUINT32 = 6,
				miSINGLE = 7, miDOUBLE = 9, miINT64 = 12, miUINT64 = 13, miMATRIX = 14,
				miCOMPRESSED = 15,
				miUTF8 = 16, miUTF16 = 17, miUTF32 = 18 };

MatParser::MatParser():d_limit(0)
{
}

MatParser::~MatParser()
{
	releaseLexer();
}

bool MatParser::setDevice(QIODevice * in, bool own)
{
	releaseLexer();
	d_lex.append( new MatLexer() );
	return d_lex.first()->setDevice( in, own );
}

MatParser::Token MatParser::nextToken()
{
	Q_ASSERT( !d_lex.isEmpty() );

	if( d_peek.d_type != Null )
	{
		Token t = d_peek;
		d_peek = Token();
		return t;
	}
	// else

	MatLexer::DataElement e = d_lex.last()->nextElement();
	if( e.d_end )
	{
		if( d_lex.size() > 1 )
		{
			delete d_lex.last();
			d_lex.removeLast();
			return Token(EndMatrix);
		}else
			return Token(Null);
	}else if( e.d_error )
		return Token(Error, "Lexer Error" );
	// else
	switch( e.d_type )
	{
	case miMATRIX:
		{
			d_lex.append( new MatLexer( d_lex.first()->needsByteSwap() ) );
			d_lex.last()->setDevice( e.d_stream.data() );
		}
		return Token(BeginMatrix);
	case miCOMPRESSED:
		return Token(Error, "miCOMPRESSED");
	default:
		return readValue( e.d_stream.data(), e.d_type );
	}
	Q_ASSERT( false );
	return Token(Error);
}

MatParser::Token MatParser::peekToken()
{
	d_peek = Token();
	d_peek = nextToken();
	return d_peek;
}

void MatParser::skipLevel()
{
	if( d_lex.size() > 1 )
	{
		d_lex.last()->readAll();
	}
}

void MatParser::releaseLexer()
{
	foreach( MatLexer* lex, d_lex )
		delete lex;
	d_lex.clear();
}


template<class T>
static MatParser::Token _read( QIODevice* in, bool swap, const char* name, int limit )
{
	QVariantList tmp;
	int i = 0;
	while( !in->atEnd() )
	{
		if( limit != 0 && i >= limit )
		{
			in->readAll();
			break;
		}
		T v;
		if( MatLexer::read( in, v, swap ) != sizeof(T) )
			return MatParser::Token(MatParser::Error, name );
		else
			tmp.append(v);
		i++;
	}
	if( tmp.size() == 1 )
		return MatParser::Token(MatParser::Value,tmp.first());
	else
		return MatParser::Token(MatParser::Value,tmp);
}

MatParser::Token MatParser::readValue(QIODevice* in, quint8 type)
{
	Q_ASSERT( !d_lex.isEmpty() );
	const bool swap = d_lex.first()->needsByteSwap();
	switch( type )
	{
	case miINT8: // ASCII-Names werden laut MATLAB Spec als miINT8 Array abgespeichert
		return Token(Value, in->readAll() ); // ByteArray wird nur Limitiert, wenn nicht als Char Array verwendet
	case miUINT8:
		return _read<quint8>( in, swap, "miUINT8", d_limit );
	case miINT16:
		return _read<qint16>( in, swap, "miINT16", d_limit );
	case miUINT16:
		return _read<quint16>( in, swap, "miUINT16", d_limit );
	case miINT32:
		return _read<qint32>( in, swap, "miINT32", d_limit );
	case miUINT32:
		return _read<quint32>( in, swap, "miUINT32", d_limit );
	case miSINGLE:
		return _read<float>( in, swap, "miSINGLE", d_limit );
	case miDOUBLE:
		return _read<double>( in, swap, "miDOUBLE", d_limit );
	case miINT64:
		return _read<qint64>( in, swap, "miINT64", d_limit );
	case miUINT64:
		return _read<quint64>( in, swap, "miUINT64", d_limit );
	case miUTF8:
		return Token(Value, QString::fromUtf8( in->readAll() ) ); //  Strings ohne Limit
	case miUTF16:
		{
			QVector<ushort> tmp;
			while( !in->atEnd() )
			{
				ushort v;
				if( MatLexer::read( in, v, swap ) != 2 )
					return Token(Error, "miUTF16");
				else
					tmp.append(v);
			}
			return Token(Value, QString::fromUtf16( tmp.data(), tmp.size() ) );
		}
		break;
	case miUTF32:
		{
			QVector<uint> tmp;
			while( !in->atEnd() )
			{
				uint v;
				if( MatLexer::read( in, v, swap ) != 4 )
					return Token(Error, "miUTF32");
				else
					tmp.append(v);
			}
			return Token(Value, QString::fromUcs4( tmp.data(), tmp.size() ) ); 
		}
		break;
	}
	return Token(Error, "Invalid type");
}

