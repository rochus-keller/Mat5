#ifndef MATLEXER_H
#define MATLEXER_H

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

#include <QIODevice>
#include <QSharedData>

namespace Mat
{
	class MatLexer
	{
	public:
		class InStream : public QIODevice, public QSharedData
		{
		public:
			InStream( QIODevice* in, quint32 len, quint8 padding, bool compressed = false );
			~InStream();
			qint64 bytesAvailable () const;
			bool isSequential() const { return true; }
			quint32 getLen() const { return d_len; }
		protected:
			qint64 readData( char * data, qint64 maxSize );
			qint64 writeData(const char *, qint64 ) { return -1; }
			void eatPadding();
		private:
			QIODevice* d_in;
			quint32 d_len;
			quint8 d_padding;
			bool d_compressed;
		};

		template<class T>
		static quint32 read( QIODevice* in, T& i, bool swap )
		{
			const int count = in->read( (char*)(&i), sizeof(T) );
			if( count != sizeof(T) )
				return count;
			if( swap )
				swapByteOrder( (char*)(&i), sizeof(T) );
			return count;
		}

		MatLexer(bool byteSwap = false);
		~MatLexer();

		bool setDevice( QIODevice*, bool own = false, bool expectHeader = true );
		bool setDevice( InStream* );
		bool needsByteSwap() const { return d_needByteSwap; }

		struct DataElement
		{
			quint8 d_type;
			bool d_error;
			bool d_end;
			QExplicitlySharedDataPointer<InStream> d_stream;
			DataElement():d_type(0),d_error(false),d_end(true){}
			DataElement(bool e):d_type(0),d_error(e),d_end(true){}
		};
		DataElement nextElement();
		void readAll();
	protected:
		void release();
		static void swapByteOrder(char* ptr, quint32 len );
		void readPadding( int len, int boundary );
	private:
		QIODevice* d_in;
		QExplicitlySharedDataPointer<InStream> d_keep;
		bool d_needByteSwap;
		bool d_owner;
	};
}

#endif // MATLEXER_H
