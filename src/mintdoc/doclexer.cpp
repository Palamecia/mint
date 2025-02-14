/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#include "doclexer.h"
#include "docparser.hpp"

#include <mint/config.h>

const std::unordered_map<std::string, int> DocLexer::OPERATORS = {
	{"#", yy::parser::token::SHARP_TOKEN},
	{"\n", yy::parser::token::LINE_BREAK_TOKEN},
};

DocLexer::DocLexer(std::stringstream &stream) :
	m_stream(stream),
	m_cptr(stream.get()) {}

std::string DocLexer::next_token() {

	std::string token;

	enum SearchMode : std::uint8_t {
		FIND_OPERATOR,
		FIND_NUMBER,
		FIND_BLANK,
		FIND_WORD
	};

	SearchMode find_mode = is_operator(std::string({static_cast<char>(m_cptr)})) ? FIND_OPERATOR
						   : is_digit(m_cptr)									 ? FIND_NUMBER
						   : is_white_space(m_cptr)								 ? FIND_BLANK
																				 : FIND_WORD;

	switch (find_mode) {
	case FIND_OPERATOR:
		while (!is_white_space(m_cptr) && (m_cptr != EOF) && is_operator(token + static_cast<char>(m_cptr))) {
			token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		break;
	case FIND_NUMBER:
		while (!is_white_space(m_cptr) && (m_cptr != EOF) && is_digit(m_cptr)) {
			token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		break;
	case FIND_BLANK:
		while (is_white_space(m_cptr)) {
			token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		break;
	case FIND_WORD:
		while (!is_white_space(m_cptr) && (m_cptr != EOF) && !is_digit(m_cptr)
			   && !is_operator(std::string({static_cast<char>(m_cptr)}))) {
			token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		break;
	}

	return token;
}

int DocLexer::token_type(const std::string &token) {
	if (auto it = OPERATORS.find(token); it != OPERATORS.end()) {
		return it->second;
	}
	if (token.empty()) {
		return yy::parser::token::FILE_END_TOKEN;
	}
	if (is_white_space(token.front())) {
		return yy::parser::token::BLANK_TOKEN;
	}
	return yy::parser::token::WORD_TOKEN;
}

std::string DocLexer::format_error(const char *error) const {
	return error;
}

bool DocLexer::at_end() const {
	return m_stream.eof();
}

bool DocLexer::is_digit(int c) {
#ifdef BUILD_TYPE_DEBUG
	return isascii(c) && isdigit(c);
#else
	return isdigit(c);
#endif
}

bool DocLexer::is_white_space(int c) {
	return (c == ' ') || (c == '\t');
}

bool DocLexer::is_operator(const std::string &token) {
	return OPERATORS.find(token) != OPERATORS.end();
}

bool DocLexer::is_operator(const std::string &token, int *type) {
	if (auto it = OPERATORS.find(token); it != OPERATORS.end()) {
		*type = it->second;
		return true;
	}
	return false;
}

int DocLexer::next_char() {
	int cptr = m_stream.get();
	return cptr;
}
