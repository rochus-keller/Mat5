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

#include "MatLexer.h"
#include <QtDebug>
#include <QBuffer>
#include "qtiocompressor.h"
using namespace Mat;

void MatLexer::swapByteOrder(char* ptr, quint32 len )
{
	const quint32 max = len / 2;
	for( quint32 i = 0; i < max; i++ )
	{
		char tmp = ptr[ i ];
		ptr[ i ] = ptr[ len - 1 - i ];
		ptr[ len - 1 - i ] = tmp;
	}
}

static quint8 calcPadding( quint32 len, quint32 boundary )
{
	const int rem = len % boundary;
	if( rem )
		return boundary - rem;
	else
		return 0;
}

void MatLexer::readPadding(int len, int boundary )
{
	const int rem = len % boundary;
	QByteArray rest;
	if( rem )
	{
		rest = d_in->read( boundary - rem );
	}
	if( !rest.isEmpty() && rest.count( char(0) ) != rest.size() )
	{
		qDebug() << "## Remainder not null" << rest;
	}
}

MatLexer::MatLexer(bool byteSwap):d_in(0),d_needByteSwap(byteSwap)
{
}

MatLexer::~MatLexer()
{
	release();
}

bool MatLexer::setDevice(QIODevice * in, bool own, bool expectHeader)
{
	if( in == 0 )
		return false;
	if( !in->isOpen() )
	{
		if( !in->open(QIODevice::ReadOnly) )
			return false;
	}else
		in->reset();
	if( expectHeader )
	{
		bool swap = false;
		const QByteArray header = in->read(116);
		if( header.size() < 116 )
			return false;
		if( !header.startsWith("MATLAB 5.0 MAT-file") )
			return false;
		const QByteArray subsytemDataOffset = in->read(8);
		if( subsytemDataOffset.size() < 8 )
			return false;
		const QByteArray flags = in->read(4);
		if( flags.size() < 4 )
			return false;
		const quint16 mi = 0x4d49;
		const char* mi_ptr = (const char*)(&mi);
		if( flags[2] == mi_ptr[0] && flags[3] == mi_ptr[1] )
			swap = false;
		else if( flags[2] == mi_ptr[1] && flags[3] == mi_ptr[0] )
			swap = true;
		else
			return false;
		quint16 version = 0;
		if( swap )
			version = flags[1] + ( flags[0] << 8 );
		else
			version = flags[0] + ( flags[1] << 8 );
		if( version != 0x0100 )
			return false;
		d_needByteSwap = swap;
	}
	release();
	d_in = in;
	d_owner = own;
	d_keep = 0;

	return true;
}

bool MatLexer::setDevice(MatLexer::InStream * in)
{
	if( in == 0 )
		return false;
	if( !in->isOpen() )
		return false;
	d_in = in;
	d_keep = in;
	d_owner = false;
	return true;
}

MatLexer::DataElement MatLexer::nextElement()
{
	enum { miCOMPRESSED = 15 };

	if( d_in == 0 || d_in->atEnd() )
		return DataElement();

	const QByteArray peek = d_in->peek(4);
	if( peek.size() < 4 )
		return DataElement(true);

	DataElement e;
	e.d_end = false;
	if( peek[1] != 0 || peek[2] != 0 ) // Die Bedingung im Handbuch ist falsch!
	{
		// Small Data Element Format
		// nn tt
		// T=1..7, N=1..4
		// Little endian N0 T0
		// Big endian 0N 0T
		// Vergleich:
		// T=1..18
		// Little endian: T000
		// Big endian: 000T
		// Die mittleren zwei sind immer Null; bei Small Format ist immer eins nicht null.

		// folgendes empirisch ermittelt; die Spezifikation ist fehlerhaft
		qint32 val;
		if( read( d_in, val, d_needByteSwap ) != 4 )
			return DataElement(true);
		qint16 type = val & 0x0000ffff;
		qint16 len = ( val >> 16 );
		// folgendes würde man aus der Spezifikation interpretieren, was aber offensichtlich falsch ist!
//		if( read( d_in, len, d_needByteSwap ) != 2 ) // zuerst Length !
//			return DataElement(true);
//		if( read( d_in, type, d_needByteSwap ) != 2 )
//			return DataElement(true);
		if( len > 4 )
			return DataElement(true);
		e.d_type = type;
		e.d_stream = new InStream(d_in,len, calcPadding(len,4) );
	}else
	{
		// tttt nnnn
		// type count
		// T=1..18
		// Little endian: Tttt Nnnn
		// Big endian: tttT nnnN

		// Normal Format
		qint32 type;
		qint32 len;
		if( read( d_in, type, d_needByteSwap ) != 4 ) // zuerst Type !
			return DataElement(true);
		if( read( d_in, len, d_needByteSwap ) != 4 )
			return DataElement(true);
		if( type == miCOMPRESSED )
		{
			// komprimierte Daten haben kein Padding am Schluss
			e.d_stream = new InStream( d_in, len, 0, true );
			if( read( e.d_stream.data(), type, d_needByteSwap ) != 4 ) // zuerst Type !
				return DataElement(true);
			if( read( e.d_stream.data(), len, d_needByteSwap ) != 4 )
				return DataElement(true);
			e.d_type = type;
		}else
		{
			e.d_type = type;
			e.d_stream = new InStream(d_in, len, calcPadding( len, 8 ) );
		}

	}
	return e;
}

void MatLexer::readAll()
{
	QByteArray tmp;
	tmp.resize( 64000 );
	while( d_in->read( tmp.data(), tmp.size() ) > 0 )
		;
}

void MatLexer::release()
{
	if( d_in != 0 && d_owner )
		delete d_in;
	d_in = 0;
	d_owner = false;
}

MatLexer::InStream::InStream(QIODevice *in, quint32 len, quint8 padding, bool compressed):
	d_in(in),d_len(len),d_padding(padding),d_compressed(compressed)
{
	Q_ASSERT( in != 0 );
	if( compressed )
	{
		d_len = 0;
		d_padding = 0;
		InStream* inorig = new InStream( in, len, padding );
		inorig->setParent(this);
		QtIOCompressor* incom = new QtIOCompressor( inorig );
		incom->setParent(this);
		if( !incom->open(QIODevice::ReadOnly) )
			qWarning() << "InStream cannot open QtIOCompressor:" << incom->errorString();
		d_in = incom;
	}
	QIODevice::open(QIODevice::ReadOnly);
}

MatLexer::InStream::~InStream()
{
	if( d_len > 0 || d_padding > 0 )
		qWarning() << "Deleting InStream where not all bytes were read" << ( d_len + d_padding );
}

qint64 MatLexer::InStream::bytesAvailable() const
{
	if( d_compressed )
		return d_in->bytesAvailable() + QIODevice::bytesAvailable();
	else
		return d_len + QIODevice::bytesAvailable();
}

qint64 MatLexer::InStream::readData(char *data, qint64 maxSize)
{
	if( d_compressed )
		return d_in->read( data, maxSize );
	else if( d_len > 0 )
	{
		const qint64 read = d_in->read( data, qMin( maxSize, qint64(d_len) ) );
		if( read != -1 )
		{
			d_len -= read;
			if( d_len == 0 )
				eatPadding();
		}
		return read;
	}else
		return -1;
}

void MatLexer::InStream::eatPadding()
{
	QByteArray rest;
	if( d_padding )
	{
		rest = d_in->read( d_padding );
		d_padding = 0;
	}
	if( !rest.isEmpty() && rest.count( char(0) ) != rest.size() )
	{
		qDebug() << "## Remainder not null" << rest;
	}

}
