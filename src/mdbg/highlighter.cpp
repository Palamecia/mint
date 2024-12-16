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

#define is_standard_symbol(_token) \
((_token == "self") || (_token == "va_args"))

	class Highlighter : public LexicalHandler {
public:
	Highlighter(size_t from_line, size_t to_line, size_t current_line) :
		m_from_line(from_line),
		m_to_line(to_line),
		m_current_line(current_line) {

	}

protected:
	bool on_script_end() override {
		set_style(text);
		print_highlighted("\n");
		return true;
	}

	bool on_symbol_token(const std::vector<std::string> &context, const std::string &token, std::string::size_type offset) override {
		if (is_defined_class(context, token)) {
			set_style(user_type);
		}
		else if (is_defined_symbol(context, token)) {
			set_style(constant);
		}
		else if (is_standard_symbol(token)) {
			set_style(standard_symbol);
		}
		else {
			set_style(text);
		}
		return true;
	}

	bool on_token(token::Type type, const std::string &token, std::string::size_type offset) override {
		switch (type) {
		case token::line_end_token:
			return true;
		case token::assert_token:
		case token::break_token:
		case token::case_token:
		case token::catch_token:
		case token::class_token:
		case token::const_token:
		case token::continue_token:
		case token::def_token:
		case token::default_token:
		case token::defined_token:
		case token::elif_token:
		case token::else_token:
		case token::enum_token:
		case token::exit_token:
		case token::final_token:
		case token::for_token:
		case token::if_token:
		case token::in_token:
		case token::is_token:
		case token::let_token:
		case token::lib_token:
		case token::load_token:
		case token::membersof_token:
		case token::override_token:
		case token::package_token:
		case token::print_token:
		case token::raise_token:
		case token::return_token:
		case token::switch_token:
		case token::try_token:
		case token::typeof_token:
		case token::var_token:
		case token::while_token:
		case token::yield_token:
			set_style(keyword);
			break;
		case token::constant_token:
			set_style(constant);
			break;
		case token::string_token:
			set_style(string_literal);
			break;
		case token::regex_token:
			set_style(regex_literal);
			break;
		case token::number_token:
			set_style(number_literal);
			break;
		case token::module_path_token:
			set_style(module_path);
			break;
		case token::open_brace_token:
		case token::close_brace_token:
		case token::open_bracket_token:
		case token::close_bracket_token:
		case token::close_bracket_equal_token:
		case token::open_parenthesis_token:
		case token::close_parenthesis_token:
			set_style(brace);
			break;
		case token::comment_token:
			// done in on_comment
			return true;
		case token::symbol_token:
			// done in on_symbol_token
			break;
		default:
			set_style(text);
			break;
		}
		print_highlighted(token);
		return true;
	}

	bool on_white_space(const std::string &token, std::string::size_type offset) override {
		set_style(text);
		print_highlighted(token);
		return true;
	}

	bool on_comment(const std::string &token, std::string::size_type offset) override {
		set_style(comment);
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
			set_style(text);
			print_highlighted("\n");
			return false;
		}
		return true;
	}

	enum Style {
		text,
		comment,
		keyword,
		constant,
		user_type,
		module_path,
		number_literal,
		string_literal,
		regex_literal,
		standard_symbol,
		brace
	};

	void set_style(Style style) {
		switch (style) {
		case text:
			print_highlighted("\033[0m");
			break;

		case comment:
			print_highlighted("\033[1;30m");
			break;

		case keyword:
			print_highlighted("\033[0m");
			print_highlighted("\033[3;34m");
			break;

		case constant:
			print_highlighted("\033[0;33m");
			break;

		case user_type:
			print_highlighted("\033[0;36m");
			break;

		case module_path:
			print_highlighted("\033[0;35m");
			break;

		case number_literal:
			print_highlighted("\033[0;33m");
			break;

		case string_literal:
			print_highlighted("\033[0;32m");
			break;

		case regex_literal:
			print_highlighted("\033[0;31m");
			break;

		case standard_symbol:
			print_highlighted("\033[0m");
			print_highlighted("\033[3;33m");
			break;

		case brace:
			print_highlighted("\033[0;35m");
			break;
		}
	}

	void print_line_number(size_t line_number) {
		if (m_print) {

			if (line_number != m_from_line) {
				Terminal::print(stdout, "\n");
			}

			auto amount_of_digits = [] (size_t value) -> int {
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
