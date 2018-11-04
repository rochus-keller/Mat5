#ifndef MATREADER_H
#define MATREADER_H

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

#include <QVariant>
#include <QVector>

class QIODevice;

namespace Mat
{
	struct Matrix
	{
		QByteArray d_name;
		bool d_logical;
		bool d_global;
		bool d_valid;
		bool isValid() const { return d_valid; }
		Matrix():d_logical(false),d_global(false),d_valid(false){}
	};

	struct NumericArray : public Matrix
	{
		QVector<qint32> d_dims;
		QVariantList d_real;
		QVariantList d_img;
		QVariant getReal(int i = 0) const;
		QVariant getReal(int row, int col ) const;
		QVariant getReal(int row, int col, int z ) const;
		void allocReal( int rows, const QVariant& val = QVariant() );
		void allocReal( int rows, int cols, const QVariant& val = QVariant() );
	};

	struct String : public Matrix
	{
		QString d_str;
	};

	struct Structure : public Matrix
	{
		// Eine Structure ist immer zugleich auch eine Tabelle
		bool isObject() const { return !d_className.isEmpty(); }
		QByteArray d_className;
		QMap<QByteArray,QVariantList> d_fields;
		QString getString( const QByteArray& field ) const;
		QVariant getValue( const QByteArray& field ) const;
		Structure getStruct( const QByteArray& field ) const;
		NumericArray getArray( const QByteArray& field ) const;
		QVariant getArrayValue( const QByteArray& field, quint32 = 0 ) const;
		quint32 getArrayLen( const QByteArray& field ) const;
	};

	struct CellArray : public Matrix
	{
		QVector<qint32> d_dims;
		QVariantList d_cells;
		QVariant getValue( quint32 i ) const;
		QVariant getValue( quint32 row, quint32 col ) const;
		Structure getStruct( quint32 row, quint32 col ) const;
		QString getString( quint32 i ) const;
		QString getString( quint32 row, quint32 col ) const;
	};

	struct SparseArray : public Matrix
	{
	};

	struct Undocumented : public Matrix
	{
		QVariant d_value;
		QVariant d_sub;
	};

	class MatParser;

	class MatReader
	{
	public:
		MatReader();
		~MatReader();
		bool setDevice( QIODevice*, bool own = false );
		QVariant nextElement();
		QString getError() const { return d_error; }
		bool hasError() const { return !d_error.isEmpty(); }
		quint16 getLimit() const;
		void setLimit(quint16);
	private:
		QVariant readMatrix();
		QVariant error( const char* );
		bool readFields( Structure&, const QList<QByteArray> &names );
	private:
		MatParser* d_parser;
		QString d_error;
	};
}

Q_DECLARE_METATYPE(Mat::NumericArray)
Q_DECLARE_METATYPE(Mat::String)
Q_DECLARE_METATYPE(Mat::Structure)
Q_DECLARE_METATYPE(Mat::CellArray)
Q_DECLARE_METATYPE(Mat::SparseArray)
Q_DECLARE_METATYPE(Mat::Undocumented)

#endif // MATREADER_H
