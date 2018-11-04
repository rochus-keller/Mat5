#ifndef MATWRITER_H
#define MATWRITER_H

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
#include <QVariant>
#include <QVector>
#include <QPair>

namespace Mat
{
	class MatWriter
	{
	public:
		typedef QVector<qint32> Dims;
		template<class T>
		static quint32 write( QIODevice* out, T i )
		{
			char buf[sizeof(T)];
			write( buf, i );
			out->write( buf, sizeof(T) );
			return sizeof(T);
		}
		template<class T>
		static quint32 write( char* out, T i )
		{
			::memcpy( out, (char*)(&i), sizeof(T) );
			return sizeof(T);
		}

		MatWriter();
		~MatWriter();
		bool setDevice( QIODevice*, bool own = false, bool writeHeader = true );
		void beginStructure( const QList<QByteArray>& fieldNames, int rowCount = 1, bool large = false, const QByteArray& name = QByteArray() );
		void addStructureRow( const QVariantList& );
		void endStructure(bool compress = false);
		void beginNumArray( const Dims&, int numType, bool large = false, const QByteArray& name = QByteArray() ); // QMetaType::Type
		void addNumArrayElement( const QVariant& );
		void endNumArray(bool compress = false);
		void addCharArray( const QString&, const QByteArray& name = QByteArray() );
		// TODO: CellArray, Object, SparseArray
	protected:
		void beginMatrix( bool large = false );
		void endMatrix( bool compress = false );
		void writeCell( const QVariant&, const QByteArray& name = QByteArray() );
		void release();
		void writeHeader();
		// Primitiven
		static void writeTag( QIODevice*, quint8 miType, quint32 byteLen );
		static void writePadding( QIODevice*, int len );
		static void writeData( QIODevice*, const QVariant& );
		// Elemente
		static void writeDataElement( QIODevice*, quint16 miType, const QByteArray& );
		static void writeArrayDims( QIODevice*, const Dims& );
		static void writeArrayFlags( QIODevice*, quint16 type );
		static void writeArrayName( QIODevice*, const QByteArray& );
		struct TypeLen
		{
			quint8 d_miType;
			quint8 d_mxType;
			quint32 d_len;
			TypeLen(quint8 mi, quint8 mx, quint32 l ):d_miType(mi),d_mxType(mx),d_len(l) {}
			bool isNumArray() const;
		};
		static TypeLen matTypeFromMetaType( int metaType );
		static bool isNumeric( int metaType );
		static bool isString( int metaType );
	private:
		struct Level
		{
			QIODevice* d_out;
			TypeLen d_type;
			Dims d_dims;
			Level( QIODevice* out = 0, quint8 mxType = 0 ):d_type(0, mxType, 0),d_out(out){}
		};
		QList<Level> d_level;
		bool d_owner;
	};
}

Q_DECLARE_METATYPE( QIODevice* )

#endif // MATWRITER_H
