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

#include "mint/memory/globaldata.h"

#define is_standard_symbol(_token) \
	((_token == "self") || (_token == "va_args"))

using namespace mint;
using namespace std;

Highlighter::Highlighter(string &output, string_view::size_type offset) :
	m_output(output),
	m_offset(offset) {

}

bool Highlighter::on_script_begin() {
	m_output.clear();
	return true;
}

bool Highlighter::on_script_end() {
	set_style(text);
	return true;
}

bool Highlighter::on_symbol_token(const vector<string> &context, const string &token, string::size_type offset) {
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

bool Highlighter::on_token(token::Type type, const string &token, string::size_type offset) {
	switch (type) {
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
	case token::for_token:
	case token::if_token:
	case token::in_token:
	case token::is_token:
	case token::let_token:
	case token::lib_token:
	case token::load_token:
	case token::membersof_token:
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
		for (string::size_type from = 0, to = token.find('\n'); from != string::npos; from = std::max(to, to + 1), to = token.find('\n', to + 1)) {
			set_style(string_literal);
			m_output.append(token.substr(from, to - from));
			if (to != string::npos) {
				set_style(text);
				m_output.append("\n");
			}
		}
		return true;
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
		m_brace_depth++;
		if (m_offset == offset) {
			m_brace_match = m_brace_depth;
			set_style(brace_match);
		}
		else {
			set_style(brace);
		}
		break;
	case token::close_brace_token:
		if (m_brace_match && *m_brace_match == m_brace_depth) {
			m_brace_match = nullopt;
			set_style(brace_match);
		}
		else {
			set_style(brace);
		}
		m_brace_depth--;
		break;
	case token::open_bracket_token:
		m_bracket_depth++;
		if (m_offset == offset) {
			m_bracket_match = m_bracket_depth;
			set_style(brace_match);
		}
		else {
			set_style(brace);
		}
		break;
	case token::close_bracket_token:
	case token::close_bracket_equal_token:
		if (m_bracket_match && *m_bracket_match == m_bracket_depth) {
			m_bracket_match = nullopt;
			set_style(brace_match);
		}
		else {
			set_style(brace);
		}
		m_bracket_depth--;
		break;
	case token::open_parenthesis_token:
		m_parenthesis_depth++;
		if (m_offset == offset) {
			m_parenthesis_match = m_parenthesis_depth;
			set_style(brace_match);
		}
		else {
			set_style(brace);
		}
		break;
	case token::close_parenthesis_token:
		if (m_parenthesis_match && *m_parenthesis_match == m_parenthesis_depth) {
			m_parenthesis_match = nullopt;
			set_style(brace_match);
		}
		else {
			set_style(brace);
		}
		m_parenthesis_depth--;
		break;
	case token::symbol_token:
		// done in on_symbol_token
		break;
	default:
		set_style(text);
		break;
	}
	m_output.append(token);
	return true;
}

bool Highlighter::on_white_space(const string &token, string::size_type offset) {
	set_style(text);
	m_output.append(token);
	return true;
}

bool Highlighter::on_comment(const string &token, string::size_type offset) {
	if (token.empty() || token.back() != '\n') {
		set_style(comment);
		m_output.append(token);
	}
	else {
		set_style(comment);
		m_output.append(token.substr(0, token.size() - 1));
		set_style(text);
		m_output.append("\n");
	}
	return true;
}

void Highlighter::set_style(Style style) {
	switch (style) {
	case text:
		m_output.append("\033[0m");
		break;

	case comment:
		m_output.append("\033[1;30m");
		break;

	case keyword:
		m_output.append("\033[0m");
		m_output.append("\033[3;34m");
		break;

	case constant:
		m_output.append("\033[0;33m");
		break;

	case user_type:
		m_output.append("\033[0;36m");
		break;

	case module_path:
		m_output.append("\033[0;35m");
		break;

	case number_literal:
		m_output.append("\033[0;33m");
		break;

	case string_literal:
		m_output.append("\033[0;32m");
		break;

	case regex_literal:
		m_output.append("\033[0;31m");
		break;

	case standard_symbol:
		m_output.append("\033[0m");
		m_output.append("\033[3;33m");
		break;

	case brace:
		m_output.append("\033[0;35m");
		break;

	case brace_match:
		m_output.append("\033[0m");
		m_output.append("\033[1;31m");
		break;
	}
}

bool Highlighter::is_defined_class(const vector<string> &context, const string &token) {

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

bool Highlighter::is_defined_symbol(const vector<string> &context, const string &token) {

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

bool Highlighter::resolve_path(const vector<string> &context, PackageData *&pack, ClassDescription *&desc) {

	for (const string &token : context) {
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
			GlobalData *global_data = GlobalData::instance();
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
