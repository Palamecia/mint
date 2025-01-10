/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_LEXER_H
#define MINT_LEXER_H

#include "mint/system/datastream.h"

#include <map>

namespace mint {

class MINT_EXPORT Lexer {
public:
	explicit Lexer(DataStream *stream);

	std::string next_token();
	int token_type(const std::string &token);

	std::string read_regex();

	std::string format_error(const char *error) const;
	bool at_end() const;

	static bool is_digit(char c);
	static bool is_white_space(char c);
	static bool is_operator(const std::string &token);
	static bool is_operator(const std::string &token, int *type);

protected:
	std::string tokenize_string(char delim);

private:
	static const std::map<std::string, int> KEYWORDS;
	static const std::map<std::string, int> OPERATORS;

	DataStream *m_stream;
	int m_cptr;
	int m_remaining; // hack
};

}

#endif // MINT_LEXER_H
