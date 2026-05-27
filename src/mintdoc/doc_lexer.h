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

#ifndef MINTDOC_DOC_LEXER_H
#define MINTDOC_DOC_LEXER_H

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>

class DocLexer {
public:
	enum class Token : std::int8_t {
		sharp,
		asterisk,
		dbl_asterisk,
		tpl_asterisk,
		underscore,
		dbl_underscore,
		tpl_underscore,
		tilde,
		dbl_tilde,
		backquote,
		dbl_backquote,
		tpl_backquote,
		pipe,
		hyphen,
		dbl_hyphen,
		tpl_hyphen,
		open_parenthesis,
		close_parenthesis,
		open_bracket,
		dbl_open_bracket,
		close_bracket,
		dbl_close_bracket,
		close_bracket_open_parenthesis,
		open_brace,
		close_brace,
		left_angled,
		right_angled,
		number,
		number_period,
		word,
		blank,
		line_break,
		file_end,
	};

	static constexpr const std::size_t tab_stop = 4;

	explicit DocLexer(std::stringstream& stream);
	DocLexer(const DocLexer&) = delete;
	DocLexer(DocLexer&&) = delete;
	~DocLexer() = default;

	DocLexer& operator=(const DocLexer&) = delete;
	DocLexer& operator=(DocLexer&&) = delete;

	bool skip_to_column(std::size_t column);
	std::tuple<Token, std::string> next_token();

	[[nodiscard]] bool at_end() const;

	[[nodiscard]] std::size_t get_line_number() const;
	[[nodiscard]] std::size_t get_column_number() const;
	[[nodiscard]] std::size_t get_token_column_number() const;
	[[nodiscard]] std::size_t get_first_non_blank_column_number() const;

	static bool is_digit(int c);
	static bool is_white_space(int c);
	static bool is_operator(const std::string& token);
	static bool is_operator(const std::string& token, Token* type);

protected:
	int next_char();

private:
	static constexpr const Token unknown_token = static_cast<Token>(-1);
	static const std::unordered_map<std::string, Token> operators;

	std::stringstream& _stream;
	int _cptr;

	std::string _token;
	std::size_t _line = 1;
	std::size_t _column = 1;
	std::size_t _token_column = 0;
	std::size_t _first_non_blank_column = 1;
};

#endif // MINTDOC_DOC_LEXER_H
