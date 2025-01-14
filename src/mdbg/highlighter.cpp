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

#include "highlighter.h"

#include <mint/compiler/lexicalhandler.h>
#include <mint/memory/globaldata.h>
#include <mint/system/terminal.h>

using namespace mint;

#define is_standard_symbol(_token) ((_token == "self") || (_token == "va_args"))

class Highlighter : public LexicalHandler {
public:
	Highlighter(size_t from_line, size_t to_line, size_t current_line) :
		m_from_line(from_line),
		m_to_line(to_line),
		m_current_line(current_line) {}

protected:
	bool on_script_end() override {
		set_style(TEXT);
		print_highlighted("\n");
		return true;
	}

	bool on_symbol_token(const std::vector<std::string> &context, const std::string &token,
						 std::string::size_type offset) override {
		if (is_defined_class(context, token)) {
			set_style(USER_TYPE);
		}
		else if (is_defined_symbol(context, token)) {
			set_style(CONSTANT);
		}
		else if (is_standard_symbol(token)) {
			set_style(STANDARD_SYMBOL);
		}
		else {
			set_style(TEXT);
		}
		return true;
	}

	bool on_token(token::Type type, const std::string &token, std::string::size_type offset) override {
		switch (type) {
		case token::LINE_END_TOKEN:
			return true;
		case token::ASSERT_TOKEN:
		case token::BREAK_TOKEN:
		case token::CASE_TOKEN:
		case token::CATCH_TOKEN:
		case token::CLASS_TOKEN:
		case token::CONST_TOKEN:
		case token::CONTINUE_TOKEN:
		case token::DEF_TOKEN:
		case token::DEFAULT_TOKEN:
		case token::DEFINED_TOKEN:
		case token::ELIF_TOKEN:
		case token::ELSE_TOKEN:
		case token::ENUM_TOKEN:
		case token::EXIT_TOKEN:
		case token::FINAL_TOKEN:
		case token::FOR_TOKEN:
		case token::IF_TOKEN:
		case token::IN_TOKEN:
		case token::IS_TOKEN:
		case token::LET_TOKEN:
		case token::LIB_TOKEN:
		case token::LOAD_TOKEN:
		case token::MEMBERSOF_TOKEN:
		case token::OVERRIDE_TOKEN:
		case token::PACKAGE_TOKEN:
		case token::PRINT_TOKEN:
		case token::RAISE_TOKEN:
		case token::RETURN_TOKEN:
		case token::SWITCH_TOKEN:
		case token::TRY_TOKEN:
		case token::TYPEOF_TOKEN:
		case token::VAR_TOKEN:
		case token::WHILE_TOKEN:
		case token::YIELD_TOKEN:
			set_style(KEYWORD);
			break;
		case token::CONSTANT_TOKEN:
			set_style(CONSTANT);
			break;
		case token::STRING_TOKEN:
			set_style(STRING_LITERAL);
			break;
		case token::REGEX_TOKEN:
			set_style(REGEX_LITERAL);
			break;
		case token::NUMBER_TOKEN:
			set_style(NUMBER_LITERAL);
			break;
		case token::MODULE_PATH_TOKEN:
			set_style(MODULE_PATH);
			break;
		case token::OPEN_BRACE_TOKEN:
		case token::CLOSE_BRACE_TOKEN:
		case token::OPEN_BRACKET_TOKEN:
		case token::CLOSE_BRACKET_TOKEN:
		case token::CLOSE_BRACKET_EQUAL_TOKEN:
		case token::OPEN_PARENTHESIS_TOKEN:
		case token::CLOSE_PARENTHESIS_TOKEN:
			set_style(BRACE);
			break;
		case token::COMMENT_TOKEN:
			// done in on_comment
			return true;
		case token::SYMBOL_TOKEN:
			// done in on_symbol_token
			break;
		default:
			set_style(TEXT);
			break;
		}
		print_highlighted(token);
		return true;
	}

	bool on_white_space(const std::string &token, std::string::size_type offset) override {
		set_style(TEXT);
		print_highlighted(token);
		return true;
	}

	bool on_comment(const std::string &token, std::string::size_type offset) override {
		set_style(COMMENT);
		print_highlighted(token.substr(0, token.rfind('\n')));
		return true;
	}

	bool on_new_line(size_t line_number, std::string::size_type offset) override {
		if (line_number == m_from_line) {
			m_print = true;
		}
		if (line_number <= m_to_line) {
			print_line_number(line_number);
		}
		else {
			set_style(TEXT);
			print_highlighted("\n");
			return false;
		}
		return true;
	}

	enum Style {
		TEXT,
		COMMENT,
		KEYWORD,
		CONSTANT,
		USER_TYPE,
		MODULE_PATH,
		NUMBER_LITERAL,
		STRING_LITERAL,
		REGEX_LITERAL,
		STANDARD_SYMBOL,
		BRACE
	};

	void set_style(Style style) {
		switch (style) {
		case TEXT:
			print_highlighted("\033[0m");
			break;

		case COMMENT:
			print_highlighted("\033[1;30m");
			break;

		case KEYWORD:
			print_highlighted("\033[0m");
			print_highlighted("\033[3;34m");
			break;

		case CONSTANT:
			print_highlighted("\033[0;33m");
			break;

		case USER_TYPE:
			print_highlighted("\033[0;36m");
			break;

		case MODULE_PATH:
			print_highlighted("\033[0;35m");
			break;

		case NUMBER_LITERAL:
			print_highlighted("\033[0;33m");
			break;

		case STRING_LITERAL:
			print_highlighted("\033[0;32m");
			break;

		case REGEX_LITERAL:
			print_highlighted("\033[0;31m");
			break;

		case STANDARD_SYMBOL:
			print_highlighted("\033[0m");
			print_highlighted("\033[3;33m");
			break;

		case BRACE:
			print_highlighted("\033[0;35m");
			break;
		}
	}

	void print_line_number(size_t line_number) {
		if (m_print) {

			if (line_number != m_from_line) {
				Terminal::print(stdout, "\n");
			}

			auto amount_of_digits = [](size_t value) -> int {
				int amount = 1;
				while (value /= 10) {
					amount++;
				}
				return amount;
			};

			const int digits = (amount_of_digits(line_number + static_cast<size_t>(m_to_line)) / 4) + 3;
			if (line_number == m_current_line) {
				Terminal::printf(stdout, "\033[1;31;7m %*zd ⮞ \033[0m ", digits, line_number);
			}
			else {
				Terminal::printf(stdout, "\033[1;37;7m %*zd   \033[0m ", digits, line_number);
			}
		}
	}

	void print_highlighted(const std::string &str) {
		if (m_print) {
			Terminal::print(stdout, str.c_str());
		}
	}

	static bool is_defined_class(const std::vector<std::string> &context, const std::string &token) {

		Symbol symbol(token);
		PackageData *pack = nullptr;
		ClassDescription *desc = nullptr;

		if (resolve_path(context, pack, desc)) {

			if (desc) {
				return desc->find_class_description(symbol) != nullptr;
			}

			if (pack) {
				return pack->get_class(symbol) != nullptr;
			}

			GlobalData *global_data = GlobalData::instance();
			return global_data->get_class(symbol) != nullptr;
		}

		return false;
	}

	static bool is_defined_symbol(const std::vector<std::string> &context, const std::string &token) {

		Symbol symbol(token);
		PackageData *pack = nullptr;
		ClassDescription *desc = nullptr;

		if (resolve_path(context, pack, desc)) {

			if (desc) {
				Class *prototype = desc->generate();
				return prototype->globals().contains(symbol);
			}

			if (pack) {
				return pack->symbols().contains(symbol);
			}

			GlobalData *global_data = GlobalData::instance();
			return global_data->symbols().contains(symbol);
		}

		return false;
	}

	static bool resolve_path(const std::vector<std::string> &context, PackageData *&pack, ClassDescription *&desc) {

		for (const std::string &token : context) {
			Symbol symbol(token);
			if (desc) {
				desc = desc->find_class_description(symbol);
				if (desc == nullptr) {
					return false;
				}
			}
			else if (pack) {
				desc = pack->find_class_description(symbol);
				if (desc == nullptr) {
					pack = pack->find_package(symbol);
					if (pack == nullptr) {
						return false;
					}
				}
			}
			else {
				const GlobalData *global_data = GlobalData::instance();
				desc = global_data->find_class_description(symbol);
				if (desc == nullptr) {
					pack = global_data->find_package(symbol);
					if (pack == nullptr) {
						return false;
					}
				}
			}
		}

		return true;
	}

private:
	bool m_print = false;
	size_t m_from_line;
	size_t m_to_line;
	size_t m_current_line;
};

void print_highlighted(size_t from_line, size_t to_line, size_t current_line, std::ifstream &&script) {
	Highlighter highlighter(from_line, to_line, current_line);
	highlighter.parse(script);
}
