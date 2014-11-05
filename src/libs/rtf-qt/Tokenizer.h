/*
	Copyright (C)  2010  Brad Hards <bradh@frogmouth.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RTFREADER_TOKENIZER_H
#define RTFREADER_TOKENIZER_H

#include "Token.h"

#include <QtCore/QFile>

namespace RtfReader
{
	/**
	  RTF tokenizer
	  */
	class Tokenizer {
	public:
		explicit Tokenizer(QFile* _inputDevice)
			: m_inputDevice(_inputDevice), m_encoding("utf-8")
		{}

		/**
		 * @brief Считать блок документа
		 */
		Token fetchToken();

		/**
		 * @brief Установить кодировку считываемого документа
		 */
		void setEncoding(const QString& _name, int _value);

	private:
		QFile *m_inputDevice;
		void pullControl( Token *token );
		void pullControlWord( Token *token );
		void pullControlSymbol( Token *token );
		void pullPlainText( Token *token );

		/**
		 * @brief Кодировка документа, по умолчанию utf-8
		 */
		QString m_encoding;

		/**
		 * @brief Считать заданный символ из таблицы в необходимой кодировке
		 */
		QString readCharacter(uint _codepoint) const;
	};
}

#endif
