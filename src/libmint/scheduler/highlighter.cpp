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
#include "mint/system/terminal.h"

#define is_standard_symbol(_token) ((_token == "self") || (_token == "va_args"))

using namespace mint;

Highlighter::Highlighter(std::string &output, std::string_view::size_type offset) :
	m_output(output),
	m_offset(offset) {}

bool Highlighter::on_script_begin() {
	m_output.clear();
	return true;
}

bool Highlighter::on_script_end() {
	set_style(TEXT);
	return true;
}

bool Highlighter::on_symbol_token(const std::vector<std::string> &context, const std::string &token,
								  [[maybe_unused]] std::string::size_type offset) {
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

bool Highlighter::on_token(token::Type type, const std::string &token, std::string::size_type offset) {
	switch (type) {
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
		for (std::string::size_type from = 0, to = token.find('\n'); from != std::string::npos;
			 from = std::max(to, to + 1), to = token.find('\n', to + 1)) {
			set_style(STRING_LITERAL);
			m_output.append(token.substr(from, to - from));
			if (to != std::string::npos) {
				set_style(TEXT);
				m_output.append("\n");
			}
		}
		return true;
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
		m_brace_depth++;
		if (m_offset == offset) {
			m_brace_match = m_brace_depth;
			set_style(BRACE_MATCH);
		}
		else {
			set_style(BRACE);
		}
		break;
	case token::CLOSE_BRACE_TOKEN:
		if (m_brace_match && *m_brace_match == m_brace_depth) {
			m_brace_match = std::nullopt;
			set_style(BRACE_MATCH);
		}
		else {
			set_style(BRACE);
		}
		m_brace_depth--;
		break;
	case token::OPEN_BRACKET_TOKEN:
		m_bracket_depth++;
		if (m_offset == offset) {
			m_bracket_match = m_bracket_depth;
			set_style(BRACE_MATCH);
		}
		else {
			set_style(BRACE);
		}
		break;
	case token::CLOSE_BRACKET_TOKEN:
	case token::CLOSE_BRACKET_EQUAL_TOKEN:
		if (m_bracket_match && *m_bracket_match == m_bracket_depth) {
			m_bracket_match = std::nullopt;
			set_style(BRACE_MATCH);
		}
		else {
			set_style(BRACE);
		}
		m_bracket_depth--;
		break;
	case token::OPEN_PARENTHESIS_TOKEN:
		m_parenthesis_depth++;
		if (m_offset == offset) {
			m_parenthesis_match = m_parenthesis_depth;
			set_style(BRACE_MATCH);
		}
		else {
			set_style(BRACE);
		}
		break;
	case token::CLOSE_PARENTHESIS_TOKEN:
		if (m_parenthesis_match && *m_parenthesis_match == m_parenthesis_depth) {
			m_parenthesis_match = std::nullopt;
			set_style(BRACE_MATCH);
		}
		else {
			set_style(BRACE);
		}
		m_parenthesis_depth--;
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
	m_output.append(token);
	return true;
}

bool Highlighter::on_white_space(const std::string &token, [[maybe_unused]] std::string::size_type offset) {
	set_style(TEXT);
	m_output.append(token);
	return true;
}

bool Highlighter::on_comment(const std::string &token, [[maybe_unused]] std::string::size_type offset) {
	if (token.empty() || token.back() != '\n') {
		set_style(COMMENT);
		m_output.append(token);
	}
	else {
		set_style(COMMENT);
		m_output.append(token.substr(0, token.size() - 1));
		set_style(TEXT);
		m_output.append("\n");
	}
	return true;
}

void Highlighter::set_style(Style style) {
	switch (style) {
	case TEXT:
		m_output.append(MINT_TERM_RESET);
		break;

	case COMMENT:
		m_output.append(MINT_TERM_FG_GREY_WITH(MINT_TERM_BOLD_OPTION));
		break;

	case KEYWORD:
		m_output.append(MINT_TERM_RESET);
		m_output.append(MINT_TERM_FG_BLUE_WITH(MINT_TERM_ITALIC_OPTION));
		break;

	case CONSTANT:
		m_output.append(MINT_TERM_FG_YELLOW_WITH(MINT_TERM_RESET_OPTION));
		break;

	case USER_TYPE:
		m_output.append(MINT_TERM_FG_CYAN_WITH(MINT_TERM_RESET_OPTION));
		break;

	case MODULE_PATH:
		m_output.append(MINT_TERM_FG_MAGENTA_WITH(MINT_TERM_RESET_OPTION));
		break;

	case NUMBER_LITERAL:
		m_output.append(MINT_TERM_FG_YELLOW_WITH(MINT_TERM_RESET_OPTION));
		break;

	case STRING_LITERAL:
		m_output.append(MINT_TERM_FG_GREEN_WITH(MINT_TERM_RESET_OPTION));
		break;

	case REGEX_LITERAL:
		m_output.append(MINT_TERM_FG_RED_WITH(MINT_TERM_RESET_OPTION));
		break;

	case STANDARD_SYMBOL:
		m_output.append(MINT_TERM_RESET);
		m_output.append(MINT_TERM_FG_YELLOW_WITH(MINT_TERM_ITALIC_OPTION));
		break;

	case BRACE:
		m_output.append(MINT_TERM_FG_MAGENTA_WITH(MINT_TERM_RESET_OPTION));
		break;

	case BRACE_MATCH:
		m_output.append(MINT_TERM_RESET);
		m_output.append(MINT_TERM_FG_RED_WITH(MINT_TERM_BOLD_OPTION));
		break;
	}
}

bool Highlighter::is_defined_class(const std::vector<std::string> &context, const std::string &token) {

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

bool Highlighter::is_defined_symbol(const std::vector<std::string> &context, const std::string &token) {

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

bool Highlighter::resolve_path(const std::vector<std::string> &context, PackageData *&pack, ClassDescription *&desc) {

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
