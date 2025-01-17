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

#include "mint/compiler/lexer.h"
#include "mint/compiler/token.h"
#include "parser.hpp"
#include <cstdint>

const std::map<std::string, int> Lexer::KEYWORDS = {
	{"and", parser::token::DBL_AMP_TOKEN},
	{"assert", parser::token::ASSERT_TOKEN},
	{"break", parser::token::BREAK_TOKEN},
	{"case", parser::token::CASE_TOKEN},
	{"catch", parser::token::CATCH_TOKEN},
	{"class", parser::token::CLASS_TOKEN},
	{"const", parser::token::CONST_TOKEN},
	{"continue", parser::token::CONTINUE_TOKEN},
	{"def", parser::token::DEF_TOKEN},
	{"default", parser::token::DEFAULT_TOKEN},
	{"defined", parser::token::DEFINED_TOKEN},
	{"elif", parser::token::ELIF_TOKEN},
	{"else", parser::token::ELSE_TOKEN},
	{"enum", parser::token::ENUM_TOKEN},
	{"exit", parser::token::EXIT_TOKEN},
	{"false", parser::token::CONSTANT_TOKEN},
	{"final", parser::token::FINAL_TOKEN},
	{"for", parser::token::FOR_TOKEN},
	{"if", parser::token::IF_TOKEN},
	{"in", parser::token::IN_TOKEN},
	{"is", parser::token::IS_TOKEN},
	{"let", parser::token::LET_TOKEN},
	{"lib", parser::token::LIB_TOKEN},
	{"load", parser::token::LOAD_TOKEN},
	{"membersof", parser::token::MEMBERSOF_TOKEN},
	{"none", parser::token::CONSTANT_TOKEN},
	{"not", parser::token::EXCLAMATION_TOKEN},
	{"null", parser::token::CONSTANT_TOKEN},
	{"or", parser::token::DBL_PIPE_TOKEN},
	{"override", parser::token::OVERRIDE_TOKEN},
	{"package", parser::token::PACKAGE_TOKEN},
	{"print", parser::token::PRINT_TOKEN},
	{"raise", parser::token::RAISE_TOKEN},
	{"return", parser::token::RETURN_TOKEN},
	{"switch", parser::token::SWITCH_TOKEN},
	{"true", parser::token::CONSTANT_TOKEN},
	{"try", parser::token::TRY_TOKEN},
	{"typeof", parser::token::TYPEOF_TOKEN},
	{"var", parser::token::VAR_TOKEN},
	{"while", parser::token::WHILE_TOKEN},
	{"xor", parser::token::CARET_TOKEN},
	{"yield", parser::token::YIELD_TOKEN},
};

const std::map<std::string, int> Lexer::OPERATORS = {
	{"!", parser::token::EXCLAMATION_TOKEN},
	{"!=", parser::token::EXCLAMATION_EQUAL_TOKEN},
	{"!==", parser::token::EXCLAMATION_DBL_EQUAL_TOKEN},
	{"!~", parser::token::EXCLAMATION_TILDE_TOKEN},
	{"#!", parser::token::COMMENT_TOKEN},
	{"#", parser::token::SHARP_TOKEN},
	{"$", parser::token::DOLLAR_TOKEN},
	{"%", parser::token::PERCENT_TOKEN},
	{"%=", parser::token::PERCENT_EQUAL_TOKEN},
	{"&", parser::token::AMP_TOKEN},
	{"&&", parser::token::DBL_AMP_TOKEN},
	{"&=", parser::token::AMP_EQUAL_TOKEN},
	{"(", parser::token::OPEN_PARENTHESIS_TOKEN},
	{")", parser::token::CLOSE_PARENTHESIS_TOKEN},
	{"*", parser::token::ASTERISK_TOKEN},
	{"**", parser::token::DBL_ASTERISK_TOKEN},
	{"*=", parser::token::ASTERISK_EQUAL_TOKEN},
	{"+", parser::token::PLUS_TOKEN},
	{"++", parser::token::DBL_PLUS_TOKEN},
	{"+=", parser::token::PLUS_EQUAL_TOKEN},
	{",", parser::token::COMMA_TOKEN},
	{"-", parser::token::MINUS_TOKEN},
	{"--", parser::token::DBL_MINUS_TOKEN},
	{"-=", parser::token::MINUS_EQUAL_TOKEN},
	{".", parser::token::DOT_TOKEN},
	{"..", parser::token::DBL_DOT_TOKEN},
	{"...", parser::token::TPL_DOT_TOKEN},
	{"/", parser::token::SLASH_TOKEN},
	{"/*", parser::token::COMMENT_TOKEN},
	{"//", parser::token::COMMENT_TOKEN},
	{"/=", parser::token::SLASH_EQUAL_TOKEN},
	{":", parser::token::COLON_TOKEN},
	{":=", parser::token::COLON_EQUAL_TOKEN},
	{";", parser::token::LINE_END_TOKEN},
	{"<", parser::token::LEFT_ANGLED_TOKEN},
	{"<<", parser::token::DBL_LEFT_ANGLED_TOKEN},
	{"<<=", parser::token::DBL_LEFT_ANGLED_EQUAL_TOKEN},
	{"<=", parser::token::LEFT_ANGLED_EQUAL_TOKEN},
	{"=", parser::token::EQUAL_TOKEN},
	{"==", parser::token::DBL_EQUAL_TOKEN},
	{"===", parser::token::TPL_EQUAL_TOKEN},
	{"=>", parser::token::EQUAL_RIGHT_ANGLED_TOKEN},
	{"=~", parser::token::EQUAL_TILDE_TOKEN},
	{">", parser::token::RIGHT_ANGLED_TOKEN},
	{">=", parser::token::RIGHT_ANGLED_EQUAL_TOKEN},
	{">>", parser::token::DBL_RIGHT_ANGLED_TOKEN},
	{">>=", parser::token::DBL_RIGHT_ANGLED_EQUAL_TOKEN},
	{"?", parser::token::QUESTION_TOKEN},
	{"@", parser::token::AT_TOKEN},
	{"[", parser::token::OPEN_BRACKET_TOKEN},
	{"\\", parser::token::BACK_SLASH_TOKEN},
	{"\\\n", parser::token::NO_LINE_END_TOKEN},
	{"\n", parser::token::LINE_END_TOKEN},
	{"]", parser::token::CLOSE_BRACKET_TOKEN},
	{"]=", parser::token::CLOSE_BRACKET_EQUAL_TOKEN},
	{"^", parser::token::CARET_TOKEN},
	{"^=", parser::token::CARET_EQUAL_TOKEN},
	{"{", parser::token::OPEN_BRACE_TOKEN},
	{"|", parser::token::PIPE_TOKEN},
	{"|=", parser::token::PIPE_EQUAL_TOKEN},
	{"||", parser::token::DBL_PIPE_TOKEN},
	{"}", parser::token::CLOSE_BRACE_TOKEN},
	{"~", parser::token::TILDE_TOKEN},
};

Lexer::Lexer(DataStream *stream) :
	m_stream(stream),
	m_cptr(0),
	m_remaining(0) {}

std::string Lexer::next_token() {

	while (is_white_space(static_cast<char>(m_cptr))) {
		m_cptr = m_stream->get_char();
	}

	std::string token;
	int token_type = -1;

	enum SearchMode : std::uint8_t {
		FIND_OPERATOR,
		FIND_NUMBER,
		FIND_IDENTIFIER
	};

	SearchMode find_mode = is_operator(std::string({static_cast<char>(m_cptr)}), &token_type) ? FIND_OPERATOR
						   : is_digit(m_cptr)												  ? FIND_NUMBER
																							  : FIND_IDENTIFIER;

	if (m_remaining) {
		token += static_cast<char>(m_remaining);
		m_remaining = 0;
	}

	if (m_cptr == '\'' || m_cptr == '"') {
		return tokenize_string(static_cast<char>(m_cptr));
	}

	switch (find_mode) {
	case FIND_OPERATOR:
		while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)
			   && is_operator(token + static_cast<char>(m_cptr), &token_type)) {
			token += static_cast<char>(m_cptr);
			m_cptr = m_stream->get_char();
		}

		switch (token_type) {
		case parser::token::BACK_SLASH_TOKEN:
		case parser::token::CLOSE_BRACKET_TOKEN:
			while (is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)) {
				m_cptr = m_stream->get_char();
			}
			if (is_operator(token + static_cast<char>(m_cptr), &token_type)) {
				m_remaining = m_cptr;
				m_cptr = m_stream->get_char();
				if (UNLIKELY(is_operator(std::string({static_cast<char>(m_remaining), static_cast<char>(m_cptr)})))) {
					token_type = -1;
				}
				else {
					token += static_cast<char>(m_remaining);
					m_remaining = 0;
				}
			}
			break;

		case parser::token::COMMENT_TOKEN:
			if (token == "//" || token == "#!") {
				while (m_cptr != '\n' && m_cptr != EOF) {
					m_cptr = m_stream->get_char();
				}
				return next_token();
			}

			if (token == "/*") {
				for (;;) {
					while (m_cptr != '*' && m_cptr != EOF) {
						m_cptr = m_stream->get_char();
					}
					switch ((m_cptr = m_stream->get_char())) {
					case '/':
						m_cptr = m_stream->get_char();
						return next_token();
					case EOF:
						return {};
					default:
						break;
					}
				}
			}
			break;

		default:
			break;
		}

		if (token_type == parser::token::NO_LINE_END_TOKEN) {
			return next_token();
		}
		break;

	case FIND_NUMBER:
		while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF) && is_digit(m_cptr)) {
			token += static_cast<char>(m_cptr);
			m_cptr = m_stream->get_char();
		}

		if (m_cptr == 'b' || m_cptr == 'B' || m_cptr == 'o' || m_cptr == 'O' || m_cptr == 'x' || m_cptr == 'X') {
			while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)
				   && !is_operator(std::string({static_cast<char>(m_cptr)}))) {
				token += static_cast<char>(m_cptr);
				m_cptr = m_stream->get_char();
			}
			return token;
		}

		if (m_cptr == '.') {
			std::string decimals = ".";
			m_cptr = m_stream->get_char();
			if (is_operator(decimals + static_cast<char>(m_cptr))) {
				m_remaining = '.';
				return token;
			}
			while (is_digit(m_cptr)) {
				decimals += static_cast<char>(m_cptr);
				m_cptr = m_stream->get_char();
			}
			token += decimals;
		}

		if (m_cptr == 'e' || m_cptr == 'E') {
			std::string exponent = std::string({static_cast<char>(m_cptr)});
			m_cptr = m_stream->get_char();
			if (m_cptr == '+' || m_cptr == '-') {
				exponent += static_cast<char>(m_cptr);
				m_cptr = m_stream->get_char();
			}
			while (is_digit(m_cptr)) {
				exponent += static_cast<char>(m_cptr);
				m_cptr = m_stream->get_char();
			}
			token += exponent;
		}
		break;

	case FIND_IDENTIFIER:
		while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)
			   && !is_operator(std::string({static_cast<char>(m_cptr)}))) {
			token += static_cast<char>(m_cptr);
			m_cptr = m_stream->get_char();
		}
		break;
	}

	return token;
}

int Lexer::token_type(const std::string &token) {

	if (auto it = KEYWORDS.find(token); it != KEYWORDS.end()) {
		return it->second;
	}

	if (auto it = OPERATORS.find(token); it != OPERATORS.end()) {
		return it->second;
	}

	if (token.empty()) {
		return parser::token::FILE_END_TOKEN;
	}
	if (is_digit(token.front())) {
		return parser::token::NUMBER_TOKEN;
	}
	if (token.front() == '\'' || token.front() == '"') {
		return parser::token::STRING_TOKEN;
	}

	return parser::token::SYMBOL_TOKEN;
}

std::string Lexer::read_regex() {

	std::string regex;
	bool escape = false;

	do {
		if (m_cptr == EOF || m_cptr == '\n') {
			return regex;
		}
		regex += static_cast<char>(m_cptr);
		escape = (m_cptr == '\\');
		m_cptr = m_stream->get_char();
	}
	while ((m_cptr != '/') || escape);

	return regex;
}

std::string Lexer::format_error(const char *error) const {

	auto path = m_stream->path();
	auto line_number = m_stream->line_number();
	auto line_error = m_stream->line_error();

	return path + ":" + std::to_string(line_number) + " " + error + "\n" + line_error;
}

bool Lexer::at_end() const {
	return m_stream->at_end();
}

bool Lexer::is_digit(int c) {
#ifdef BUILD_TYPE_DEBUG
	return isascii(c) && isdigit(c);
#else
	return isdigit(c);
#endif
}

bool Lexer::is_white_space(int c) {
	return (c <= ' ') && (c != '\n') && (c >= '\0');
}

bool Lexer::is_operator(const std::string &token) {
	return OPERATORS.find(token) != OPERATORS.end();
}

bool Lexer::is_operator(const std::string &token, int *type) {

	auto it = OPERATORS.find(token);

	if (it != OPERATORS.end()) {
		*type = it->second;
		return true;
	}

	return false;
}

std::string Lexer::tokenize_string(char delim) {

	std::string token;
	bool shift = false;

	do {
		if (m_cptr == EOF) {
			return token;
		}
		token += static_cast<char>(m_cptr);
		shift = ((m_cptr == '\\') && !shift);
	}
	while (((m_cptr = m_stream->get_char()) != delim) || shift);
	token += static_cast<char>(m_cptr);

	m_cptr = m_stream->get_char();
	return token;
}

mint::token::Type mint::token::from_local_id(int id) {

#define BEGIN_TOKEN_CAST(__id) switch (__id) {

#define TOKEN_CAST(__token) \
	case parser::token::__token: \
		return mint::token::__token;

#define END_TOKEN_CAST() }

	BEGIN_TOKEN_CAST(id)
	TOKEN_CAST(ASSERT_TOKEN)
	TOKEN_CAST(BREAK_TOKEN)
	TOKEN_CAST(CASE_TOKEN)
	TOKEN_CAST(CATCH_TOKEN)
	TOKEN_CAST(CLASS_TOKEN)
	TOKEN_CAST(CONST_TOKEN)
	TOKEN_CAST(CONTINUE_TOKEN)
	TOKEN_CAST(DEF_TOKEN)
	TOKEN_CAST(DEFAULT_TOKEN)
	TOKEN_CAST(ELIF_TOKEN)
	TOKEN_CAST(ELSE_TOKEN)
	TOKEN_CAST(ENUM_TOKEN)
	TOKEN_CAST(EXIT_TOKEN)
	TOKEN_CAST(FINAL_TOKEN)
	TOKEN_CAST(FOR_TOKEN)
	TOKEN_CAST(IF_TOKEN)
	TOKEN_CAST(IN_TOKEN)
	TOKEN_CAST(LET_TOKEN)
	TOKEN_CAST(LIB_TOKEN)
	TOKEN_CAST(LOAD_TOKEN)
	TOKEN_CAST(OVERRIDE_TOKEN)
	TOKEN_CAST(PACKAGE_TOKEN)
	TOKEN_CAST(PRINT_TOKEN)
	TOKEN_CAST(RAISE_TOKEN)
	TOKEN_CAST(RETURN_TOKEN)
	TOKEN_CAST(SWITCH_TOKEN)
	TOKEN_CAST(TRY_TOKEN)
	TOKEN_CAST(WHILE_TOKEN)
	TOKEN_CAST(YIELD_TOKEN)
	TOKEN_CAST(VAR_TOKEN)
	TOKEN_CAST(CONSTANT_TOKEN)
	TOKEN_CAST(STRING_TOKEN)
	TOKEN_CAST(NUMBER_TOKEN)
	TOKEN_CAST(SYMBOL_TOKEN)
	TOKEN_CAST(NO_LINE_END_TOKEN)
	TOKEN_CAST(LINE_END_TOKEN)
	TOKEN_CAST(FILE_END_TOKEN)
	TOKEN_CAST(COMMENT_TOKEN)
	TOKEN_CAST(DOLLAR_TOKEN)
	TOKEN_CAST(AT_TOKEN)
	TOKEN_CAST(SHARP_TOKEN)
	TOKEN_CAST(BACK_SLASH_TOKEN)
	TOKEN_CAST(COMMA_TOKEN)
	TOKEN_CAST(DBL_PIPE_TOKEN)
	TOKEN_CAST(DBL_AMP_TOKEN)
	TOKEN_CAST(PIPE_TOKEN)
	TOKEN_CAST(CARET_TOKEN)
	TOKEN_CAST(AMP_TOKEN)
	TOKEN_CAST(EQUAL_TOKEN)
	TOKEN_CAST(QUESTION_TOKEN)
	TOKEN_CAST(COLON_TOKEN)
	TOKEN_CAST(COLON_EQUAL_TOKEN)
	TOKEN_CAST(CLOSE_BRACKET_EQUAL_TOKEN)
	TOKEN_CAST(PLUS_EQUAL_TOKEN)
	TOKEN_CAST(MINUS_EQUAL_TOKEN)
	TOKEN_CAST(ASTERISK_EQUAL_TOKEN)
	TOKEN_CAST(SLASH_EQUAL_TOKEN)
	TOKEN_CAST(PERCENT_EQUAL_TOKEN)
	TOKEN_CAST(DBL_LEFT_ANGLED_EQUAL_TOKEN)
	TOKEN_CAST(DBL_RIGHT_ANGLED_EQUAL_TOKEN)
	TOKEN_CAST(AMP_EQUAL_TOKEN)
	TOKEN_CAST(PIPE_EQUAL_TOKEN)
	TOKEN_CAST(CARET_EQUAL_TOKEN)
	TOKEN_CAST(EQUAL_RIGHT_ANGLED_TOKEN)
	TOKEN_CAST(DBL_DOT_TOKEN)
	TOKEN_CAST(TPL_DOT_TOKEN)
	TOKEN_CAST(DBL_EQUAL_TOKEN)
	TOKEN_CAST(TPL_EQUAL_TOKEN)
	TOKEN_CAST(EXCLAMATION_EQUAL_TOKEN)
	TOKEN_CAST(EXCLAMATION_DBL_EQUAL_TOKEN)
	TOKEN_CAST(IS_TOKEN)
	TOKEN_CAST(EQUAL_TILDE_TOKEN)
	TOKEN_CAST(EXCLAMATION_TILDE_TOKEN)
	TOKEN_CAST(LEFT_ANGLED_TOKEN)
	TOKEN_CAST(RIGHT_ANGLED_TOKEN)
	TOKEN_CAST(LEFT_ANGLED_EQUAL_TOKEN)
	TOKEN_CAST(RIGHT_ANGLED_EQUAL_TOKEN)
	TOKEN_CAST(DBL_LEFT_ANGLED_TOKEN)
	TOKEN_CAST(DBL_RIGHT_ANGLED_TOKEN)
	TOKEN_CAST(PLUS_TOKEN)
	TOKEN_CAST(MINUS_TOKEN)
	TOKEN_CAST(ASTERISK_TOKEN)
	TOKEN_CAST(SLASH_TOKEN)
	TOKEN_CAST(PERCENT_TOKEN)
	TOKEN_CAST(EXCLAMATION_TOKEN)
	TOKEN_CAST(TILDE_TOKEN)
	TOKEN_CAST(TYPEOF_TOKEN)
	TOKEN_CAST(MEMBERSOF_TOKEN)
	TOKEN_CAST(DEFINED_TOKEN)
	TOKEN_CAST(DBL_PLUS_TOKEN)
	TOKEN_CAST(DBL_MINUS_TOKEN)
	TOKEN_CAST(DBL_ASTERISK_TOKEN)
	TOKEN_CAST(DOT_TOKEN)
	TOKEN_CAST(OPEN_PARENTHESIS_TOKEN)
	TOKEN_CAST(CLOSE_PARENTHESIS_TOKEN)
	TOKEN_CAST(OPEN_BRACKET_TOKEN)
	TOKEN_CAST(CLOSE_BRACKET_TOKEN)
	TOKEN_CAST(OPEN_BRACE_TOKEN)
	TOKEN_CAST(CLOSE_BRACE_TOKEN)
	END_TOKEN_CAST()

	return mint::token::FILE_END_TOKEN;
}
