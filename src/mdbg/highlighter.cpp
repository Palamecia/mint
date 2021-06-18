#include "highlighter.h"

#include <memory/globaldata.h>
#include <memory/class.h>
#include <system/bufferstream.h>
#include <system/terminal.h>
#include <system/assert.h>
#include <compiler/lexer.h>
#include <compiler/token.h>

using namespace std;
using namespace mint;

#define is_standard_symbol(_token) \
	((_token == "self") || (_token == "va_args"))

#define is_operator_alias(_token) \
	((_token == "and") || (_token == "or") || (_token == "xor") || (_token == "not"))

#define is_comment(_token) \
	((_token.find("/*", pos) != string::npos) || (_token.find("//", pos) != string::npos) || (_token.find("#!", pos) != string::npos))

bool resolve_path(const vector<Symbol> &context, PackageData *&pack, ClassDescription *&desc) {

	for (const Symbol &symbol : context) {
		if (desc) {
			desc = desc->findSubClass(symbol);
			if (desc == nullptr) {
				return false;
			}
		}
		else if (pack) {
			desc = pack->findClassDescription(symbol);
			if (desc == nullptr) {
				pack = pack->findPackage(symbol);
				if (pack == nullptr) {
					return false;
				}
			}
		}
		else {
			desc = GlobalData::instance().findClassDescription(symbol);
			if (desc == nullptr) {
				pack = GlobalData::instance().findPackage(symbol);
				if (pack == nullptr) {
					desc = GlobalData::instance().findClassDescription(symbol);
					if (desc == nullptr) {
						pack = GlobalData::instance().findPackage(symbol);
						if (pack == nullptr) {
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

static bool is_defined_class(const vector<Symbol> &context, const Symbol &token) {

	PackageData *pack = nullptr;
	ClassDescription *desc = nullptr;

	if (resolve_path(context, pack, desc)) {

		if (desc) {
			return desc->findSubClass(token) != nullptr;
		}

		if (pack) {
			return pack->getClass(token) != nullptr;
		}

		return GlobalData::instance().getClass(token) != nullptr;
	}

	return false;
}

static bool is_defined_symbol(const vector<Symbol> &context, const Symbol &token) {

	PackageData *pack = nullptr;
	ClassDescription *desc = nullptr;

	if (resolve_path(context, pack, desc)) {

		if (desc) {
			Class *prototype = desc->generate();
			return prototype->globals().members().find(token) != prototype->globals().members().end();
		}

		if (pack) {
			return pack->symbols().find(token) != pack->symbols().end();
		}

		return GlobalData::instance().symbols().find(token) != GlobalData::instance().symbols().end();
	}

	return false;
}

enum Style {
	text,
	comment,
	keyword,
	constant,
	user_type,
	number_literal,
	string_literal,
	regex_literal,
	standard_symbol
};

void set_style(Style style) {
	switch (style) {
	case text:
		term_cprint(stdout, "\033[0m");
		break;

	case comment:
		term_cprint(stdout, "\033[1;30m");
		break;

	case keyword:
		term_cprint(stdout, "\033[0m");
		term_cprint(stdout, "\033[3;34m");
		break;

	case constant:
		term_cprint(stdout, "\033[0;33m");
		break;

	case user_type:
		term_cprint(stdout, "\033[0m");
		term_cprint(stdout, "\033[1;31m");
		break;

	case number_literal:
		term_cprint(stdout, "\033[0;31m");
		break;

	case string_literal:
		term_cprint(stdout, "\033[0;32m");
		break;

	case regex_literal:
		term_cprint(stdout, "\033[0;35m");
		break;

	case standard_symbol:
		term_cprint(stdout, "\033[0m");
		term_cprint(stdout, "\033[3;33m");
		break;
	}
}

enum State {
	expect_start,
	expect_value,
	expect_operator
};

void print_highlighted(const string &script) {

	State state = expect_start;
	vector<Symbol> context;

	BufferStream stream(script);
	Lexer lexer(&stream);
	size_t pos = 0;

	while (!stream.atEnd()) {

		string token = lexer.nextToken();
		auto start = script.find(token, pos);
		auto lenght = token.length();

		if (start != string::npos) {

			auto comment_pos = script.find("/*", pos);

			if ((comment_pos >= pos) && (comment_pos <= start)) {
				set_style(comment);
				start = script.find(token, script.find("*/", comment_pos) + 2);
				term_cprint(stdout, script.substr(pos, start - pos).c_str());
				pos = start;
			}

			switch (token::fromLocalId(lexer.tokenType(token))) {
			case token::assert_token:
			case token::break_token:
			case token::case_token:
			case token::catch_token:
			case token::class_token:
			case token::const_token:
			case token::continue_token:
			case token::def_token:
			case token::default_token:
			case token::elif_token:
			case token::else_token:
			case token::enum_token:
			case token::exit_token:
			case token::for_token:
			case token::if_token:
			case token::in_token:
			case token::lib_token:
			case token::load_token:
			case token::package_token:
			case token::print_token:
			case token::raise_token:
			case token::return_token:
			case token::switch_token:
			case token::try_token:
			case token::while_token:
			case token::yield_token:
			case token::constant_token:
			case token::is_token:
			case token::typeof_token:
			case token::membersof_token:
			case token::defined_token:
				set_style(keyword);
				context.clear();
				state = expect_start;
				break;

			case token::number_token:
				set_style(number_literal);
				context.clear();
				state = expect_operator;
				break;

			case token::string_token:
				set_style(string_literal);
				context.clear();
				state = expect_operator;
				break;

			case token::slash_token:
				if (state == expect_operator) {
					set_style(text);
					state = expect_value;
				}
				else {
					token += lexer.readRegex();
					token += lexer.nextToken();
					lenght = token.length();

					if (isalpha(script[start + lenght])) {
						token += lexer.nextToken();
						lenght = token.length();
					}

					set_style(regex_literal);
					state = expect_operator;
				}
				context.clear();
				break;

			case token::symbol_token:
				if (is_defined_class(context, Symbol(token))) {
					set_style(user_type);
				}
				else if (is_defined_symbol(context, Symbol(token))) {
					set_style(constant);
				}
				else if (is_standard_symbol(token)) {
					set_style(standard_symbol);
				}
				else {
					set_style(text);
				}
				context.push_back(Symbol(token));
				state = expect_operator;
				break;

			case token::dot_token:
				set_style(text);
				state = expect_value;
				break;

			default:
				if (is_operator_alias(token)) {
					set_style(keyword);
					state = expect_value;
				}
				else {
					set_style(text);
					if (Lexer::isOperator(token)) {
						state = expect_value;
					}
					else {
						state = expect_operator;
					}
				}
				context.clear();
				break;
			}

			term_cprint(stdout, script.substr(pos, (start - pos) + lenght).c_str());
			pos = start + lenght;
		}
		else {

			token = script.substr(pos);

			if (is_comment(token)) {
				set_style(comment);
			}
			else {
				set_style(text);
			}

			term_cprint(stdout, token.c_str());
			break;
		}
	}

	term_cprint(stdout, "\033[0m\n");
}
