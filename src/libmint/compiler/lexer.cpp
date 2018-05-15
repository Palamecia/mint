#include "compiler/lexer.h"
#include "parser.hpp"

using namespace std;
using namespace yy;

const map<string, int> Lexer::keywords = {
	{"and", parser::token::dbl_amp_token},
	{"assert", parser::token::assert_token},
	{"break", parser::token::break_token},
	{"catch", parser::token::catch_token},
	{"class", parser::token::class_token},
	{"const", parser::token::const_token},
	{"continue", parser::token::continue_token},
	{"def", parser::token::def_token},
	{"defined", parser::token::defined_token},
	{"elif", parser::token::elif_token},
	{"else", parser::token::else_token},
	{"enum", parser::token::enum_token},
	{"exit", parser::token::exit_token},
	{"false", parser::token::constant_token},
	{"for", parser::token::for_token},
	{"if", parser::token::if_token},
	{"in", parser::token::in_token},
	{"is", parser::token::is_token},
	{"lib", parser::token::lib_token},
	{"load", parser::token::load_token},
	{"membersof", parser::token::membersof_token},
	{"none", parser::token::constant_token},
	{"not", parser::token::exclamation_token},
	{"null", parser::token::constant_token},
	{"or", parser::token::dbl_pipe_token},
	{"print", parser::token::print_token},
	{"raise", parser::token::raise_token},
	{"return", parser::token::return_token},
	{"true", parser::token::constant_token},
	{"try", parser::token::try_token},
	{"typeof", parser::token::typeof_token},
	{"while", parser::token::while_token},
	{"xor", parser::token::caret_token},
	{"yield", parser::token::yield_token}
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
	{"]", parser::token::close_bracket_token},
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
	{"!=", parser::token::exclamation_equal_token},
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

string Lexer::nextToken() {

	while (isWhiteSpace(m_cptr)) {
		m_cptr = m_stream->getChar();
	}

	bool findOperator = isOperator(string() + (char)m_cptr);
	string token;

	if (m_remaining) {
		token += (char)m_remaining;
		m_remaining = 0;
	}

	if (m_cptr == '\'' || m_cptr == '"') {
		return tokenizeString(m_cptr);
	}

	while (!isWhiteSpace(m_cptr) && (m_cptr != EOF)) {
		if (findOperator) {
			if (isOperator(token + (char)m_cptr)) {
				token += (char)m_cptr;
			}
			else {
				break;
			}
		}
		else {
			if (isOperator(string() + (char)m_cptr) || isWhiteSpace(m_cptr)) {
				break;
			}
			else {
				token += (char)m_cptr;
			}
		}
		m_cptr = m_stream->getChar();
	}

	if (m_cptr == '.') {
		for (char c : token) {
			if (!isdigit(c)) {
				return token;
			}
		}
		string decimals = ".";
		m_cptr = m_stream->getChar();
		if (isOperator(decimals + (char)m_cptr)) {
			m_remaining = '.';
			return token;
		}
		while (isdigit(m_cptr)) {
			decimals += (char)m_cptr;
			m_cptr = m_stream->getChar();
		}
		token += decimals;
	}

	if (token == "//" || token == "#!") {
		while (m_cptr != '\n') {
			 m_cptr = m_stream->getChar();
		}
		return nextToken();
	}
	if (token == "/*") {
		for (;;) {
			while (m_cptr != '*') {
				m_cptr = m_stream->getChar();
			}
			if ((m_cptr = m_stream->getChar()) == '/') {
				m_cptr = m_stream->getChar();
				return nextToken();
			}
		}
	}

	return token;
}

int Lexer::tokenType(const string &token) {

	auto it = keywords.find(token);
	if (it != keywords.end()) {
		return it->second;
	}

	it = operators.find(token);
	if (it != operators.end()) {
		return it->second;
	}

	if (isdigit(token.front())) {
		return parser::token::number_token;
	}
	else if (token.front() == '\'' || token.front() == '"') {
		return parser::token::string_token;
	}

	return parser::token::symbol_token;
}

string Lexer::readRegex() {

	string regex;
	bool escape = false;

	do {
		regex += m_cptr;
		escape = (m_cptr == '\\');
		m_cptr = m_stream->getChar();
	}
	while ((m_cptr != '/') || escape);

	return regex;
}

string Lexer::formatError(const char *error) const {

	auto path = m_stream->path();
	auto lineNumber = m_stream->lineNumber();
	auto lineError = m_stream->lineError();

	return path + ":"  + to_string(lineNumber) + " " + error + "\n" + lineError;
}

bool Lexer::atEnd() const {
	return m_stream->atEnd();
}

bool Lexer::isWhiteSpace(char c) {
	return (c <= ' ') && (c != '\n') && (c >= '\0');
}

bool Lexer::isOperator(const string &token) {
	return operators.find(token) != operators.end();
}

string Lexer::tokenizeString(char delim) {

	string token;
	bool shift = false;

	do {
		token += m_cptr;
	} while (((m_cptr = m_stream->getChar()) != delim) || (shift = ((m_cptr == '\\') && !shift)));
	token += m_cptr;

	m_cptr = m_stream->getChar();
	return token;
}
