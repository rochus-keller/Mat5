#ifndef MATPARSER_H
#define MATPARSER_H

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

class QIODevice;

namespace Mat
{
	class MatLexer;

	class MatParser
	{
	public:
		enum TokenType { Null, Value, BeginMatrix, EndMatrix, Error };
		struct Token
		{
			quint8 d_type;
			QVariant d_value;
			Token(quint8 type = Null, const QVariant& v = QVariant() ):d_type(type),d_value(v) {}
		};

		MatParser();
		~MatParser();
		bool setDevice( QIODevice*, bool own = false );
		Token nextToken();
		Token peekToken();
		quint16 getLimit() const { return d_limit; }
		void setLimit(quint16 l) { d_limit = l; }
		void skipLevel();
	protected:
		void releaseLexer();
		Token readValue( QIODevice *in, quint8 type );
	private:
		QList<MatLexer*> d_lex;
		Token d_peek;
		quint16 d_limit; // 0..alles
	};
}

#endif // MATPARSER_H
