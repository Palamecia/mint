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

#include <mint/config.h>

const std::unordered_map<std::string, DocLexer::Token> DocLexer::OPERATORS = {
	{"___", TPL_UNDERSCORE_TOKEN},
	{"__", DBL_UNDERSCORE_TOKEN},
	{"_", UNDERSCORE_TOKEN},
	{"-", HYPHEN_TOKEN},
	{"--", DBL_HYPHEN_TOKEN},
	{"---", TPL_HYPHEN_TOKEN},
	{"(", OPEN_PARENTHESIS_TOKEN},
	{")", CLOSE_PARENTHESIS_TOKEN},
	{"[", OPEN_BRACKET_TOKEN},
	{"[[", DBL_OPEN_BRACKET_TOKEN},
	{"]", CLOSE_BRACKET_TOKEN},
	{"](", CLOSE_BRACKET_OPEN_PARENTHESIS_TOKEN},
	{"]]", DBL_CLOSE_BRACKET_TOKEN},
	{"{", OPEN_BRACE_TOKEN},
	{"}", CLOSE_BRACE_TOKEN},
	{"*", ASTERISK_TOKEN},
	{"**", DBL_ASTERISK_TOKEN},
	{"***", TPL_ASTERISK_TOKEN},
	{"\n", LINE_BREAK_TOKEN},
	{"#", SHARP_TOKEN},
	{"`", BACKQUOTE_TOKEN},
	{"``", DBL_BACKQUOTE_TOKEN},
	{"```", TPL_BACKQUOTE_TOKEN},
	{"<", LEFT_ANGLED_TOKEN},
	{">", RIGHT_ANGLED_TOKEN},
	{"|", PIPE_TOKEN},
	{"~", TILDE_TOKEN},
	{"~~", DBL_TILDE_TOKEN},
};

DocLexer::DocLexer(std::stringstream &stream) :
	m_stream(stream),
	m_cptr(stream.get()) {}

bool DocLexer::skip_to_column(std::size_t column) {
	while (m_column <= column && !m_stream.eof()) {
		if (m_cptr == '\n') {
			m_cptr = next_char();
			return false;
		}
		m_cptr = next_char();
	}
	return true;
}

std::tuple<DocLexer::Token, std::string> DocLexer::next_token() {

	DocLexer::Token token_type = UNKNOWN_TOKEN;

	m_token_column = m_column - 1;
	m_token.clear();

	if (m_cptr == EOF) {
		return {FILE_END_TOKEN, m_token};
	}

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
		while (!is_white_space(m_cptr) && (m_cptr != EOF)
			   && is_operator(m_token + static_cast<char>(m_cptr), &token_type)) {
			m_token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		return {token_type, m_token};
	case FIND_NUMBER:
		while (!is_white_space(m_cptr) && (m_cptr != EOF) && is_digit(m_cptr)) {
			m_token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		if (m_cptr != '.') {
			return {NUMBER_TOKEN, m_token};
		}
		m_token += static_cast<char>(m_cptr);
		m_cptr = next_char();
		return {NUMBER_PERIOD_TOKEN, m_token};
	case FIND_BLANK:
		while (is_white_space(m_cptr)) {
			m_token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		return {BLANK_TOKEN, m_token};
	case FIND_WORD:
		while (!is_white_space(m_cptr) && (m_cptr != EOF) && !is_digit(m_cptr)
			   && !is_operator(std::string({static_cast<char>(m_cptr)}))) {
			m_token += static_cast<char>(m_cptr);
			m_cptr = next_char();
		}
		return {WORD_TOKEN, m_token};
	}

	return {FILE_END_TOKEN, m_token};
}

bool DocLexer::at_end() const {
	return m_stream.eof();
}

std::size_t DocLexer::get_line_number() const {
	return m_line;
}

std::size_t DocLexer::get_column_number() const {
	return m_column;
}

std::size_t DocLexer::get_token_column_number() const {
	return m_token_column;
}

std::size_t DocLexer::get_first_non_blank_column_number() const {
	return m_first_non_blank_column;
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

bool DocLexer::is_operator(const std::string &token, Token *type) {
	if (auto it = OPERATORS.find(token); it != OPERATORS.end()) {
		*type = it->second;
		return true;
	}
	return false;
}

int DocLexer::next_char() {
	int cptr = m_stream.get();
	switch (cptr) {
	case '\n':
		m_first_non_blank_column = 0;
		m_column = 0;
		m_line++;
		break;
	case '\t':
		if (m_first_non_blank_column == m_column) {
			m_first_non_blank_column += TAB_STOP - (m_column % TAB_STOP);
		}
		m_column += TAB_STOP - (m_column % TAB_STOP);
		break;
	case ' ':
		if (m_first_non_blank_column == m_column) {
			m_first_non_blank_column++;
		}
		m_column++;
		break;
	default:
		m_column++;
		break;
	}
	return cptr;
}
