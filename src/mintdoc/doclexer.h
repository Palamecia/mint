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

#ifndef MINTDOC_DOCLEXER_H
#define MINTDOC_DOCLEXER_H

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>

class DocLexer {
public:
	enum Token : std::int8_t {
		SHARP_TOKEN,
		ASTERISK_TOKEN,
		DBL_ASTERISK_TOKEN,
		TPL_ASTERISK_TOKEN,
		UNDERSCORE_TOKEN,
		DBL_UNDERSCORE_TOKEN,
		TPL_UNDERSCORE_TOKEN,
		TILDE_TOKEN,
		DBL_TILDE_TOKEN,
		BACKQUOTE_TOKEN,
		DBL_BACKQUOTE_TOKEN,
		TPL_BACKQUOTE_TOKEN,
		PIPE_TOKEN,
		HYPHEN_TOKEN,
		DBL_HYPHEN_TOKEN,
		TPL_HYPHEN_TOKEN,
		OPEN_PARENTHESIS_TOKEN,
		CLOSE_PARENTHESIS_TOKEN,
		OPEN_BRACKET_TOKEN,
		DBL_OPEN_BRACKET_TOKEN,
		CLOSE_BRACKET_TOKEN,
		DBL_CLOSE_BRACKET_TOKEN,
		CLOSE_BRACKET_OPEN_PARENTHESIS_TOKEN,
		OPEN_BRACE_TOKEN,
		CLOSE_BRACE_TOKEN,
		LEFT_ANGLED_TOKEN,
		RIGHT_ANGLED_TOKEN,
		NUMBER_TOKEN,
		NUMBER_PERIOD_TOKEN,
		WORD_TOKEN,
		BLANK_TOKEN,
		LINE_BREAK_TOKEN,
		FILE_END_TOKEN
	};

	static constexpr const std::size_t TAB_STOP = 4;

	explicit DocLexer(std::stringstream &stream);
	DocLexer(const DocLexer &) = delete;
	DocLexer(DocLexer &&) = delete;
	~DocLexer() = default;
	
	DocLexer &operator=(const DocLexer &) = delete;
	DocLexer &operator=(DocLexer &&) = delete;

	bool skip_to_column(std::size_t column);
	std::tuple<Token, std::string> next_token();

	[[nodiscard]] bool at_end() const;

	[[nodiscard]] std::size_t get_line_number() const;
	[[nodiscard]] std::size_t get_column_number() const;
	[[nodiscard]] std::size_t get_token_column_number() const;
	[[nodiscard]] std::size_t get_first_non_blank_column_number() const;

	static bool is_digit(int c);
	static bool is_white_space(int c);
	static bool is_operator(const std::string &token);
	static bool is_operator(const std::string &token, Token *type);

protected:
	int next_char();

private:
	static constexpr const Token UNKNOWN_TOKEN = static_cast<Token>(-1);
	static const std::unordered_map<std::string, Token> OPERATORS;
	
	std::stringstream &m_stream;
	int m_cptr;

	std::string m_token;
	std::size_t m_line = 1;
	std::size_t m_column = 1;
	std::size_t m_token_column = 0;
	std::size_t m_first_non_blank_column = 1;
};

#endif // MINTDOC_DOCLEXER_H
