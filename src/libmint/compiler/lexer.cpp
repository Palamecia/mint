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

using namespace std;

const map<string, int> Lexer::keywords = {
	{"and", parser::token::dbl_amp_token},
	{"assert", parser::token::assert_token},
	{"break", parser::token::break_token},
	{"case", parser::token::case_token},
	{"catch", parser::token::catch_token},
	{"class", parser::token::class_token},
	{"const", parser::token::const_token},
	{"continue", parser::token::continue_token},
	{"def", parser::token::def_token},
	{"default", parser::token::default_token},
	{"defined", parser::token::defined_token},
	{"elif", parser::token::elif_token},
	{"else", parser::token::else_token},
	{"enum", parser::token::enum_token},
	{"exit", parser::token::exit_token},
	{"false", parser::token::constant_token},
	{"final", parser::token::final_token},
	{"for", parser::token::for_token},
	{"if", parser::token::if_token},
	{"in", parser::token::in_token},
	{"is", parser::token::is_token},
	{"let", parser::token::let_token},
	{"lib", parser::token::lib_token},
	{"load", parser::token::load_token},
	{"membersof", parser::token::membersof_token},
	{"none", parser::token::constant_token},
	{"not", parser::token::exclamation_token},
	{"null", parser::token::constant_token},
	{"or", parser::token::dbl_pipe_token},
	{"override", parser::token::override_token},
	{"package", parser::token::package_token},
	{"print", parser::token::print_token},
	{"raise", parser::token::raise_token},
	{"return", parser::token::return_token},
	{"switch", parser::token::switch_token},
	{"true", parser::token::constant_token},
	{"try", parser::token::try_token},
	{"typeof", parser::token::typeof_token},
	{"while", parser::token::while_token},
	{"xor", parser::token::caret_token},
	{"yield", parser::token::yield_token},
	{"var", parser::token::var_token}
};

const map<string, int> Lexer::operators = {
	{"$", parser::token::dollar_token},
	{"@", parser::token::at_token},
	{"+", parser::token::plus_token},
	{"-", parser::token::minus_token},
	{"*", parser::token::asterisk_token},
	{"/", parser::token::slash_token},
	{"%", parser::token::percent_token},
	{"!", parser::token::exclamation_token},
	{"~", parser::token::tilde_token},
	{"=", parser::token::equal_token},
	{":", parser::token::dbldot_token},
	{".", parser::token::dot_token},
	{"..", parser::token::dot_dot_token},
	{"...", parser::token::tpl_dot_token},
	{",", parser::token::comma_token},
	{"(", parser::token::open_parenthesis_token},
	{")", parser::token::close_parenthesis_token},
	{"[", parser::token::open_bracket_token},
	{"\\", parser::token::back_slash_token},
	{"\\\n", parser::token::no_line_end_token},
	{"]", parser::token::close_bracket_token},
	{"]=", parser::token::close_bracket_equal_token},
	{"{", parser::token::open_brace_token},
	{"}", parser::token::close_brace_token},
	{"<", parser::token::left_angled_token},
	{">", parser::token::right_angled_token},
	{"?", parser::token::question_token},
	{"^", parser::token::caret_token},
	{"|", parser::token::pipe_token},
	{"&", parser::token::amp_token},
	{"#", parser::token::sharp_token},
	{"||", parser::token::dbl_pipe_token},
	{"&&", parser::token::dbl_amp_token},
	{"++", parser::token::dbl_plus_token},
	{"--", parser::token::dbl_minus_token},
	{"**", parser::token::dbl_asterisk_token},
	{"#!", parser::token::comment_token},
	{"//", parser::token::comment_token},
	{"/*", parser::token::comment_token},
	{"==", parser::token::dbl_equal_token},
	{"===", parser::token::tpl_equal_token},
	{"!=", parser::token::exclamation_equal_token},
	{"!==", parser::token::exclamation_dbl_equal_token},
	{":=", parser::token::dbldot_equal_token},
	{"+=", parser::token::plus_equal_token},
	{"-=", parser::token::minus_equal_token},
	{"*=", parser::token::asterisk_equal_token},
	{"/=", parser::token::slash_equal_token},
	{"%=", parser::token::percent_equal_token},
	{"<<=", parser::token::dbl_left_angled_equal_token},
	{">>=", parser::token::dbl_right_angled_equal_token},
	{"&=", parser::token::amp_equal_token},
	{"|=", parser::token::pipe_equal_token},
	{"^=", parser::token::caret_equal_token},
	{"=~", parser::token::equal_tilde_token},
	{"!~", parser::token::exclamation_tilde_token},
	{"<=", parser::token::left_angled_equal_token},
	{">=", parser::token::right_angled_equal_token},
	{"<<", parser::token::dbl_left_angled_token},
	{">>", parser::token::dbl_right_angled_token},
	{";", parser::token::line_end_token},
	{"\n", parser::token::line_end_token},
};

Lexer::Lexer(DataStream *stream) :
	m_stream(stream),
	m_cptr(0),
	m_remaining(0) {

}

string Lexer::next_token() {
	
	while (is_white_space(static_cast<char>(m_cptr))) {
		m_cptr = m_stream->get_char();
	}

	string token;
	int token_type = -1;
	enum SearchMode { find_operator, find_number, find_identifier };
	SearchMode find_mode = is_operator(string({static_cast<char>(m_cptr)}), &token_type) ? find_operator : is_digit(m_cptr) ? find_number : find_identifier;

	if (m_remaining) {
		token += static_cast<char>(m_remaining);
		m_remaining = 0;
	}

	if (m_cptr == '\'' || m_cptr == '"') {
		return tokenize_string(static_cast<char>(m_cptr));
	}

	switch (find_mode) {
	case find_operator:
		while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)
			   && is_operator(token + static_cast<char>(m_cptr), &token_type)) {
			token += static_cast<char>(m_cptr);
			m_cptr = m_stream->get_char();
		}

		switch (token_type) {
		case parser::token::back_slash_token:
		case parser::token::close_bracket_token:
			while (is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)) {
				m_cptr = m_stream->get_char();
			}
			if (is_operator(token + static_cast<char>(m_cptr), &token_type)) {
				m_remaining = m_cptr;
				m_cptr = m_stream->get_char();
				if (UNLIKELY(is_operator(string({static_cast<char>(m_remaining), static_cast<char>(m_cptr)})))) {
					token_type = -1;
				}
				else {
					token += static_cast<char>(m_remaining);
					m_remaining = 0;
				}
			}
			break;

		case parser::token::comment_token:
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

		if (token_type == parser::token::no_line_end_token) {
			return next_token();
		}
		break;

	case find_number:
		while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)
			   && is_digit(m_cptr)) {
			token += static_cast<char>(m_cptr);
			m_cptr = m_stream->get_char();
		}

		if (m_cptr == 'b' || m_cptr == 'B' || m_cptr == 'o' || m_cptr == 'O' || m_cptr == 'x' || m_cptr == 'X') {
			while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)
				   && !is_operator(string({static_cast<char>(m_cptr)}))) {
				token += static_cast<char>(m_cptr);
				m_cptr = m_stream->get_char();
			}
			return token;
		}

		if (m_cptr == '.') {
			string decimals = ".";
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
			string exponent = string({static_cast<char>(m_cptr)});
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

	case find_identifier:
		while (!is_white_space(static_cast<char>(m_cptr)) && (m_cptr != EOF)
			   && !is_operator(string({static_cast<char>(m_cptr)}))) {
			token += static_cast<char>(m_cptr);
			m_cptr = m_stream->get_char();
		}
		break;
	}

	return token;
}

int Lexer::token_type(const string &token) {

	auto it = keywords.find(token);
	if (it != keywords.end()) {
		return it->second;
	}

	it = operators.find(token);
	if (it != operators.end()) {
		return it->second;
	}

	if (token.empty()) {
		return parser::token::file_end_token;
	}
	if (is_digit(token.front())) {
		return parser::token::number_token;
	}
	if (token.front() == '\'' || token.front() == '"') {
		return parser::token::string_token;
	}

	return parser::token::symbol_token;
}

string Lexer::read_regex() {

	string regex;
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

string Lexer::format_error(const char *error) const {

	auto path = m_stream->path();
	auto lineNumber = m_stream->line_number();
	auto lineError = m_stream->line_error();

	return path + ":"  + to_string(lineNumber) + " " + error + "\n" + lineError;
}

bool Lexer::at_end() const {
	return m_stream->at_end();
}

bool Lexer::is_digit(char c) {
#ifdef BUILD_TYPE_DEBUG
	return isascii(c) && isdigit(c);
#else
	return isdigit(c);
#endif
}

bool Lexer::is_white_space(char c) {
	return (c <= ' ') && (c != '\n') && (c >= '\0');
}

bool Lexer::is_operator(const string &token) {
	return operators.find(token) != operators.end();
}

bool Lexer::is_operator(const string &token, int *type) {

	auto it = operators.find(token);

	if (it != operators.end()) {
		*type = it->second;
		return true;
	}

	return false;
}

string Lexer::tokenize_string(char delim) {

	string token;
	bool shift = false;

	do {
		if (m_cptr == EOF) {
			return token;
		}
		token += static_cast<char>(m_cptr);
		shift = ((m_cptr == '\\') && !shift);
	} while (((m_cptr = m_stream->get_char()) != delim) || shift);
	token += static_cast<char>(m_cptr);
	
	m_cptr = m_stream->get_char();
	return token;
}

mint::token::Type mint::token::from_local_id(int id) {

#define BEGIN_TOKEN_CAST(__id) \
	switch (__id) {

#define TOKEN_CAST(__token) \
	case parser::token::__token: return mint::token::__token;

#define END_TOKEN_CAST() \
	}

	BEGIN_TOKEN_CAST(id)
	TOKEN_CAST(assert_token)
	TOKEN_CAST(break_token)
	TOKEN_CAST(case_token)
	TOKEN_CAST(catch_token)
	TOKEN_CAST(class_token)
	TOKEN_CAST(const_token)
	TOKEN_CAST(continue_token)
	TOKEN_CAST(def_token)
	TOKEN_CAST(default_token)
	TOKEN_CAST(elif_token)
	TOKEN_CAST(else_token)
	TOKEN_CAST(enum_token)
	TOKEN_CAST(exit_token)
	TOKEN_CAST(final_token)
	TOKEN_CAST(for_token)
	TOKEN_CAST(if_token)
	TOKEN_CAST(in_token)
	TOKEN_CAST(let_token)
	TOKEN_CAST(lib_token)
	TOKEN_CAST(load_token)
	TOKEN_CAST(override_token)
	TOKEN_CAST(package_token)
	TOKEN_CAST(print_token)
	TOKEN_CAST(raise_token)
	TOKEN_CAST(return_token)
	TOKEN_CAST(switch_token)
	TOKEN_CAST(try_token)
	TOKEN_CAST(while_token)
	TOKEN_CAST(yield_token)
	TOKEN_CAST(var_token)
	TOKEN_CAST(constant_token)
	TOKEN_CAST(string_token)
	TOKEN_CAST(number_token)
	TOKEN_CAST(symbol_token)
	TOKEN_CAST(no_line_end_token)
	TOKEN_CAST(line_end_token)
	TOKEN_CAST(file_end_token)
	TOKEN_CAST(comment_token)
	TOKEN_CAST(dollar_token)
	TOKEN_CAST(at_token)
	TOKEN_CAST(sharp_token)
	TOKEN_CAST(back_slash_token)
	TOKEN_CAST(comma_token)
	TOKEN_CAST(dbl_pipe_token)
	TOKEN_CAST(dbl_amp_token)
	TOKEN_CAST(pipe_token)
	TOKEN_CAST(caret_token)
	TOKEN_CAST(amp_token)
	TOKEN_CAST(equal_token)
	TOKEN_CAST(question_token)
	TOKEN_CAST(dbldot_token)
	TOKEN_CAST(dbldot_equal_token)
	TOKEN_CAST(close_bracket_equal_token)
	TOKEN_CAST(plus_equal_token)
	TOKEN_CAST(minus_equal_token)
	TOKEN_CAST(asterisk_equal_token)
	TOKEN_CAST(slash_equal_token)
	TOKEN_CAST(percent_equal_token)
	TOKEN_CAST(dbl_left_angled_equal_token)
	TOKEN_CAST(dbl_right_angled_equal_token)
	TOKEN_CAST(amp_equal_token)
	TOKEN_CAST(pipe_equal_token)
	TOKEN_CAST(caret_equal_token)
	TOKEN_CAST(dot_dot_token)
	TOKEN_CAST(tpl_dot_token)
	TOKEN_CAST(dbl_equal_token)
	TOKEN_CAST(tpl_equal_token)
	TOKEN_CAST(exclamation_equal_token)
	TOKEN_CAST(exclamation_dbl_equal_token)
	TOKEN_CAST(is_token)
	TOKEN_CAST(equal_tilde_token)
	TOKEN_CAST(exclamation_tilde_token)
	TOKEN_CAST(left_angled_token)
	TOKEN_CAST(right_angled_token)
	TOKEN_CAST(left_angled_equal_token)
	TOKEN_CAST(right_angled_equal_token)
	TOKEN_CAST(dbl_left_angled_token)
	TOKEN_CAST(dbl_right_angled_token)
	TOKEN_CAST(plus_token)
	TOKEN_CAST(minus_token)
	TOKEN_CAST(asterisk_token)
	TOKEN_CAST(slash_token)
	TOKEN_CAST(percent_token)
	TOKEN_CAST(exclamation_token)
	TOKEN_CAST(tilde_token)
	TOKEN_CAST(typeof_token)
	TOKEN_CAST(membersof_token)
	TOKEN_CAST(defined_token)
	TOKEN_CAST(dbl_plus_token)
	TOKEN_CAST(dbl_minus_token)
	TOKEN_CAST(dbl_asterisk_token)
	TOKEN_CAST(dot_token)
	TOKEN_CAST(open_parenthesis_token)
	TOKEN_CAST(close_parenthesis_token)
	TOKEN_CAST(open_bracket_token)
	TOKEN_CAST(close_bracket_token)
	TOKEN_CAST(open_brace_token)
	TOKEN_CAST(close_brace_token)
	END_TOKEN_CAST()

	return mint::token::file_end_token;
}
