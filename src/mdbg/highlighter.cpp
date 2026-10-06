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
#include "mint/program/symbol.h"
#include "mint/compiler/lexical_handler.h"
#include "mint/compiler/token.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/global_data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/string.h"
#include "mint/system/terminal.h"
#include "utils.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace {

constexpr bool is_standard_symbol(std::string_view token) {
	return token == "self" || token == "va_args";
}

mint::SymbolPath make_path(const std::vector<std::string>& context, const std::string& token) {
	return {context | std::views::transform([](const auto& symbol) {
		        return mint::Symbol(symbol);
	        }) | std::ranges::to<mint::SymbolPath>(),
	    mint::Symbol(token)};
}

class Highlighter : public mint::LexicalHandler {
public:
	Highlighter(std::size_t from_line, std::size_t to_line, std::size_t current_line,
	    mint::GlobalDataDescription& global_data_description) :
	    _from_line(from_line),
	    _to_line(to_line),
	    _current_line(current_line),
	    _global_data_description(global_data_description) {}

protected:
	bool on_script_end() override {
		set_style(Style::text);
		print_highlighted("\n");
		return true;
	}

	bool on_symbol_token(const std::vector<std::string>& context, const std::string& token,
	    std::string::size_type /*offset*/) override {
		std::visit(mint::Overloaded {
		               [&](std::monostate) {
			               if (is_standard_symbol(token)) {
				               set_style(Style::standard_symbol);
			               }
			               else {
				               set_style(Style::text);
			               }
		               },
		               [&](std::reference_wrapper<const mint::PackageDescription> /*definition*/) {
			               set_style(Style::user_type);
		               },
		               [&](std::reference_wrapper<const mint::ClassDescription> /*definition*/) {
			               set_style(Style::user_type);
		               },
		               [&](std::reference_wrapper<const mint::FunctionDescription> /*definition*/) {
			               set_style(Style::function);
		               },
		               [&](std::reference_wrapper<const mint::VariableDescription> /*definition*/) {
			               set_style(Style::constant);
		               },
		           },
		    find_defined_symbol(make_path(context, token)));
		return true;
	}

	bool on_token(mint::Token type, const std::string& token, std::string::size_type /*offset*/) override {
		switch (type) {
		case mint::Token::line_end_token:
			return true;
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
		case mint::Token::constant_token:
			set_style(Style::constant);
			break;
		case mint::Token::string_token:
			set_style(Style::string_literal);
			break;
		case mint::Token::regex_token:
			set_style(Style::regex_literal);
			break;
		case mint::Token::number_token:
			set_style(Style::number_literal);
			break;
		case mint::Token::module_path_token:
			set_style(Style::module_path);
			break;
		case mint::Token::open_brace_token:
		case mint::Token::close_brace_token:
		case mint::Token::open_bracket_token:
		case mint::Token::close_bracket_token:
		case mint::Token::close_bracket_equal_token:
		case mint::Token::open_parenthesis_token:
		case mint::Token::close_parenthesis_token:
			set_style(Style::brace);
			break;
		case mint::Token::no_line_end_token:
			set_style(Style::line_continuation);
			print_highlighted("\\");
			return true;
		case mint::Token::comment_token:
			// done in on_comment
			return true;
		case mint::Token::symbol_token:
			// done in on_symbol_token
			break;
		default:
			set_style(Style::text);
			break;
		}
		print_highlighted(token);
		return true;
	}

	bool on_white_space(const std::string& token, std::string::size_type /*offset*/) override {
		set_style(Style::text);
		print_highlighted(token);
		return true;
	}

	bool on_comment(const std::string& token, std::string::size_type /*offset*/) override {
		set_style(Style::comment);
		print_highlighted(token.substr(0, token.rfind('\n')));
		return true;
	}

	bool on_new_line(std::size_t line_number, std::string::size_type /*offset*/) override {
		if (line_number == _from_line) {
			_print = true;
		}
		if (line_number <= _to_line) {
			print_line_number(line_number);
		}
		else {
			set_style(Style::text);
			print_highlighted("\n");
			return false;
		}
		return true;
	}

	enum class Style : std::uint8_t {
		text,
		comment,
		keyword,
		operator_keyword,
		controle_keyword,
		constant,
		function,
		user_type,
		number_literal,
		string_literal,
		regex_literal,
		standard_symbol,
		module_path,
		brace,
		line_continuation,
	};

	void set_style(Style style) {
		switch (style) {
		case Style::text:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET));
			break;
		case Style::comment:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_FG_DARK_GREEN));
			break;
		case Style::keyword:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_BLUE));
			break;
		case Style::operator_keyword:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_WHITE));
			break;
		case Style::controle_keyword:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_MAGENTA));
			break;
		case Style::constant:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_BLUE));
			break;
		case Style::function:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_DARK_YELLOW));
			break;
		case Style::user_type:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_CYAN));
			break;
		case Style::number_literal:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_GREEN));
			break;
		case Style::string_literal:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_YELLOW));
			break;
		case Style::regex_literal:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_RED));
			break;
		case Style::standard_symbol:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_ITALIC, MINT_TERM_FG_BLUE));
			break;
		case Style::module_path:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_GREEN));
			break;
		case Style::brace:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_MAGENTA));
			break;
		case Style::line_continuation:
			print_highlighted(MINT_TERM_OPT(MINT_TERM_RESET, MINT_TERM_FG_GREY));
			break;
		}
	}

	void print_line_number(std::size_t line_number) const {
		if (_print) {

			if (line_number != _from_line) {
				mint::Terminal::print(stdout, "\n");
			}

			const auto amount_of_digits = [](std::size_t value) -> int {
				int amount = 1;
				while (value /= mint::decimal_base) {
					amount++;
				}
				return amount;
			};

			const int digits = (amount_of_digits(line_number + static_cast<std::size_t>(_to_line)) / 4) + 3;
			if (line_number == _current_line) {
				mint::Terminal::print(stdout, std::format("\033[0;39m {:{}} ⮞ \033[0m ", line_number, digits));
			}
			else {
				mint::Terminal::print(stdout, std::format("\033[0;90m {:{}}   \033[0m ", line_number, digits));
			}
		}
	}

	void print_highlighted(const std::string& str) const {
		if (_print) {
			mint::Terminal::print(stdout, str);
		}
	}

	[[nodiscard]] mint::SymbolDefinition find_defined_symbol(const mint::SymbolPath& symbols) const {
		return _global_data_description.get().locate(symbols);
	}

private:
	bool _print = false;
	std::size_t _from_line;
	std::size_t _to_line;
	std::size_t _current_line;
	std::reference_wrapper<mint::GlobalDataDescription> _global_data_description;
};

}

void print_highlighted(std::size_t from_line, std::size_t to_line, std::size_t current_line,
    mint::GlobalDataDescription& global_data_description, std::ifstream&& script) {
	auto highlighter = Highlighter(from_line, to_line, current_line, global_data_description);
	std::ifstream stream = std::move(script);
	highlighter.parse(stream);
}
