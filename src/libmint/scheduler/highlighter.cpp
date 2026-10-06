/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/compiler/descriptions.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/config.h"
#include "mint/program/program.h"
#include "mint/program/symbol.h"
#include "mint/compiler/token.h"
#include "mint/memory/class.h"
#include "mint/memory/global_data.h"
#include "mint/system/terminal.h"
#include <algorithm>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace mint;

namespace {

constexpr bool is_standard_symbol(std::string_view token) {
	return (token == "self") || (token == "va_args");
}

SymbolPath make_path(const std::vector<std::string>& context, const std::string& token) {
	return {context | std::views::transform([](const auto& symbol) {
		        return Symbol(symbol);
	        }) | std::ranges::to<SymbolPath>(),
	    Symbol(token)};
}

}

Highlighter::Highlighter(const Program& program, std::string_view::size_type offset) :
    _program(program),
    _offset(offset) {}

std::string Highlighter::output() const {
	return _output;
}

bool Highlighter::on_script_begin() {
	_output.clear();
	return true;
}

bool Highlighter::on_script_end() {
	set_style(Style::text);
	return true;
}

bool Highlighter::on_symbol_token(const std::vector<std::string>& context, const std::string& token,
    std::string::size_type /*offset*/) {
	std::visit(Overloaded {
	               [&](std::monostate) {
		               if (is_standard_symbol(token)) {
			               set_style(Style::standard_symbol);
		               }
		               else {
			               set_style(Style::text);
		               }
	               },
	               [&](const PackageDescription& /*definition*/) {
		               set_style(Style::user_type);
	               },
	               [&](const ClassDescription& /*definition*/) {
		               set_style(Style::user_type);
	               },
	               [&](const FunctionDefinition& /*definition*/) {
		               set_style(Style::function);
	               },
	               [&](const VariableDescription& /*definition*/) {
		               set_style(Style::constant);
	               },
	           },
	    find_defined_symbol(make_path(context, token)));
	return true;
}

bool Highlighter::on_token(Token type, const std::string& token, std::string::size_type offset) {
	switch (type) {
	case mint::Token::assert_token:
	case mint::Token::async_token:
	case mint::Token::class_token:
	case mint::Token::const_token:
	case mint::Token::def_token:
	case mint::Token::defined_token:
	case mint::Token::enum_token:
	case mint::Token::final_token:
	case mint::Token::in_token:
	case mint::Token::is_token:
	case mint::Token::let_token:
	case mint::Token::lib_token:
	case mint::Token::membersof_token:
	case mint::Token::override_token:
	case mint::Token::package_token:
	case mint::Token::typeof_token:
	case mint::Token::var_token:
		set_style(Style::keyword);
		break;
	case mint::Token::exclamation_token:
	case mint::Token::exclamation_equal_token:
	case mint::Token::exclamation_dbl_equal_token:
	case mint::Token::exclamation_tilde_token:
	case mint::Token::sharp_token:
	case mint::Token::dollar_token:
	case mint::Token::percent_token:
	case mint::Token::percent_equal_token:
	case mint::Token::amp_token:
	case mint::Token::dbl_amp_token:
	case mint::Token::amp_equal_token:
	case mint::Token::asterisk_token:
	case mint::Token::dbl_asterisk_token:
	case mint::Token::asterisk_equal_token:
	case mint::Token::plus_token:
	case mint::Token::dbl_plus_token:
	case mint::Token::plus_equal_token:
	case mint::Token::minus_token:
	case mint::Token::dbl_minus_token:
	case mint::Token::minus_equal_token:
	case mint::Token::dot_token:
	case mint::Token::dbl_dot_token:
	case mint::Token::tpl_dot_token:
	case mint::Token::slash_token:
	case mint::Token::slash_equal_token:
	case mint::Token::colon_token:
	case mint::Token::colon_equal_token:
	case mint::Token::left_angled_token:
	case mint::Token::dbl_left_angled_token:
	case mint::Token::dbl_left_angled_equal_token:
	case mint::Token::left_angled_equal_token:
	case mint::Token::equal_token:
	case mint::Token::dbl_equal_token:
	case mint::Token::tpl_equal_token:
	case mint::Token::equal_right_angled_token:
	case mint::Token::equal_tilde_token:
	case mint::Token::right_angled_token:
	case mint::Token::right_angled_equal_token:
	case mint::Token::dbl_right_angled_token:
	case mint::Token::dbl_right_angled_equal_token:
	case mint::Token::question_token:
	case mint::Token::question_dot_token:
	case mint::Token::at_token:
	case mint::Token::back_slash_token:
	case mint::Token::caret_token:
	case mint::Token::caret_equal_token:
	case mint::Token::pipe_token:
	case mint::Token::pipe_equal_token:
	case mint::Token::dbl_pipe_token:
	case mint::Token::tilde_token:
		set_style(Style::operator_keyword);
		break;
	case mint::Token::await_token:
	case mint::Token::break_token:
	case mint::Token::case_token:
	case mint::Token::catch_token:
	case mint::Token::continue_token:
	case mint::Token::default_token:
	case mint::Token::elif_token:
	case mint::Token::else_token:
	case mint::Token::exit_token:
	case mint::Token::for_token:
	case mint::Token::if_token:
	case mint::Token::load_token:
	case mint::Token::print_token:
	case mint::Token::raise_token:
	case mint::Token::return_token:
	case mint::Token::switch_token:
	case mint::Token::try_token:
	case mint::Token::while_token:
	case mint::Token::yield_token:
		set_style(Style::controle_keyword);
		break;
	case Token::constant_token:
		set_style(Style::constant);
		break;
	case Token::string_token:
		for (std::string::size_type from = 0, to = token.find('\n'); from != std::string::npos;
		    from = std::max(to, to + 1), to = token.find('\n', to + 1)) {
			set_style(Style::string_literal);
			_output.append(token.substr(from, to - from));
			if (to != std::string::npos) {
				set_style(Style::text);
				_output.append("\n");
			}
		}
		return true;
	case Token::regex_token:
		set_style(Style::regex_literal);
		break;
	case Token::number_token:
		set_style(Style::number_literal);
		break;
	case Token::module_path_token:
		set_style(Style::module_path);
		break;
	case Token::open_brace_token:
		_brace_depth++;
		if (_offset == offset) {
			_brace_match = _brace_depth;
			set_style(Style::brace_match);
		}
		else {
			set_style(Style::brace);
		}
		break;
	case Token::close_brace_token:
		if (_brace_match && *_brace_match == _brace_depth) {
			_brace_match = std::nullopt;
			set_style(Style::brace_match);
		}
		else {
			set_style(Style::brace);
		}
		_brace_depth--;
		break;
	case Token::open_bracket_token:
		_bracket_depth++;
		if (_offset == offset) {
			_bracket_match = _bracket_depth;
			set_style(Style::brace_match);
		}
		else {
			set_style(Style::brace);
		}
		break;
	case Token::close_bracket_token:
	case Token::close_bracket_equal_token:
		if (_bracket_match && *_bracket_match == _bracket_depth) {
			_bracket_match = std::nullopt;
			set_style(Style::brace_match);
		}
		else {
			set_style(Style::brace);
		}
		_bracket_depth--;
		break;
	case Token::open_parenthesis_token:
		_parenthesis_depth++;
		if (_offset == offset) {
			_parenthesis_match = _parenthesis_depth;
			set_style(Style::brace_match);
		}
		else {
			set_style(Style::brace);
		}
		break;
	case Token::close_parenthesis_token:
		if (_parenthesis_match && *_parenthesis_match == _parenthesis_depth) {
			_parenthesis_match = std::nullopt;
			set_style(Style::brace_match);
		}
		else {
			set_style(Style::brace);
		}
		_parenthesis_depth--;
		break;
	case Token::no_line_end_token:
		set_style(Style::line_continuation);
		break;
	case Token::comment_token:
		// done in on_comment
		return true;
	case Token::symbol_token:
		// done in on_symbol_token
		break;
	default:
		set_style(Style::text);
		break;
	}
	_output.append(token);
	return true;
}

bool Highlighter::on_white_space(const std::string& token, std::string::size_type /*offset*/) {
	set_style(Style::text);
	_output.append(token);
	return true;
}

bool Highlighter::on_comment(const std::string& token, std::string::size_type /*offset*/) {
	set_style(Style::comment);
	_output.append(token.substr(0, token.rfind('\n')));
	if (token.ends_with('\n')) {
		set_style(Style::text);
		_output.append("\n");
	}
	return true;
}

void Highlighter::set_style(Style style) {
	switch (style) {
	case Style::text:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET));
		break;
	case Style::comment:
		_output.append(MINT_TERM_OPT(MINT_TERM_FG_DARK_GREEN));
		break;
	case Style::operator_keyword:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_WHITE));
		break;
	case Style::keyword:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_BLUE));
		break;
	case Style::controle_keyword:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_MAGENTA));
		break;
	case Style::constant:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_BLUE));
		break;
	case Style::function:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_DARK_YELLOW));
		break;
	case Style::user_type:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_CYAN));
		break;
	case Style::number_literal:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_GREEN));
		break;
	case Style::string_literal:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_YELLOW));
		break;
	case Style::regex_literal:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_RED));
		break;
	case Style::standard_symbol:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_ITALIC, MINT_TERM_FG_BLUE));
		break;
	case Style::module_path:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_GREEN));
		break;
	case Style::brace:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_MAGENTA));
		break;
	case Style::brace_match:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_BOLD, MINT_TERM_FG_RED));
		break;
	case Style::line_continuation:
		_output.append(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_GREY));
		break;
	}
}

SymbolDefinition Highlighter::find_defined_symbol(const SymbolPath& symbols) const {
	return _program.get().global_data_description().locate(symbols);
}
