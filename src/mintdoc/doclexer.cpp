/**
 * Copyright (c) 2026 Gauvain CHERY.
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
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>

const std::unordered_map<std::string, DocLexer::Token> DocLexer::operators = {
    {"___", Token::tpl_underscore},
    {"__", Token::dbl_underscore},
    {"_", Token::underscore},
    {"-", Token::hyphen},
    {"--", Token::dbl_hyphen},
    {"---", Token::tpl_hyphen},
    {"(", Token::open_parenthesis},
    {")", Token::close_parenthesis},
    {"[", Token::open_bracket},
    {"[[", Token::dbl_open_bracket},
    {"]", Token::close_bracket},
    {"](", Token::close_bracket_open_parenthesis},
    {"]]", Token::dbl_close_bracket},
    {"{", Token::open_brace},
    {"}", Token::close_brace},
    {"*", Token::asterisk},
    {"**", Token::dbl_asterisk},
    {"***", Token::tpl_asterisk},
    {"\n", Token::line_break},
    {"#", Token::sharp},
    {"`", Token::backquote},
    {"``", Token::dbl_backquote},
    {"```", Token::tpl_backquote},
    {"<", Token::left_angled},
    {">", Token::right_angled},
    {"|", Token::pipe},
    {"~", Token::tilde},
    {"~~", Token::dbl_tilde},
};

DocLexer::DocLexer(std::stringstream& stream) :
    _stream(stream),
    _cptr(stream.get()) {}

bool DocLexer::skip_to_column(std::size_t column) {
	while (_column <= column && !_stream.eof()) {
		if (_cptr == '\n') {
			_cptr = next_char();
			return false;
		}
		_cptr = next_char();
	}
	return true;
}

std::tuple<DocLexer::Token, std::string> DocLexer::next_token() {

	DocLexer::Token token_type = unknown_token;

	_token_column = _column - 1;
	_token.clear();

	if (_cptr == EOF) {
		return {Token::file_end, _token};
	}

	enum SearchMode : std::uint8_t {
		find_operator,
		find_number,
		find_blank,
		find_word
	};

	const auto find_mode = is_operator(std::string({static_cast<char>(_cptr)})) ? find_operator
	                       : is_digit(_cptr)                                    ? find_number
	                       : is_white_space(_cptr)                              ? find_blank
	                                                                            : find_word;

	switch (find_mode) {
	case find_operator:
		while (!is_white_space(_cptr) && (_cptr != EOF) && is_operator(_token + static_cast<char>(_cptr), &token_type)) {
			_token += static_cast<char>(_cptr);
			_cptr = next_char();
		}
		return {token_type, _token};
	case find_number:
		while (!is_white_space(_cptr) && (_cptr != EOF) && is_digit(_cptr)) {
			_token += static_cast<char>(_cptr);
			_cptr = next_char();
		}
		if (_cptr != '.') {
			return {Token::number, _token};
		}
		_token += static_cast<char>(_cptr);
		_cptr = next_char();
		return {Token::number_period, _token};
	case find_blank:
		while (is_white_space(_cptr)) {
			_token += static_cast<char>(_cptr);
			_cptr = next_char();
		}
		return {Token::blank, _token};
	case find_word:
		while (!is_white_space(_cptr) && (_cptr != EOF) && !is_digit(_cptr)
		       && !is_operator(std::string({static_cast<char>(_cptr)}))) {
			_token += static_cast<char>(_cptr);
			_cptr = next_char();
		}
		return {Token::word, _token};
	}

	return {Token::file_end, _token};
}

bool DocLexer::at_end() const {
	return _stream.eof();
}

std::size_t DocLexer::get_line_number() const {
	return _line;
}

std::size_t DocLexer::get_column_number() const {
	return _column;
}

std::size_t DocLexer::get_token_column_number() const {
	return _token_column;
}

std::size_t DocLexer::get_first_non_blank_column_number() const {
	return _first_non_blank_column;
}

bool DocLexer::is_digit(int c) {
#ifdef MINT_BUILD_TYPE_DEBUG
	return isascii(c) && isdigit(c);
#else
	return isdigit(c);
#endif
}

bool DocLexer::is_white_space(int c) {
	return (c == ' ') || (c == '\t');
}

bool DocLexer::is_operator(const std::string& token) {
	return operators.contains(token);
}

bool DocLexer::is_operator(const std::string& token, Token* type) {
	if (auto it = operators.find(token); it != operators.end()) {
		*type = it->second;
		return true;
	}
	return false;
}

int DocLexer::next_char() {
	const int cptr = _stream.get();
	switch (cptr) {
	case '\n':
		_first_non_blank_column = 0;
		_column = 0;
		_line++;
		break;
	case '\t':
		if (_first_non_blank_column == _column) {
			_first_non_blank_column += tab_stop - (_column % tab_stop);
		}
		_column += tab_stop - (_column % tab_stop);
		break;
	case ' ':
		if (_first_non_blank_column == _column) {
			_first_non_blank_column++;
		}
		_column++;
		break;
	default:
		_column++;
		break;
	}
	return cptr;
}
