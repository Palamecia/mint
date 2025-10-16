/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/ast/classregister.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/lexicalhandler.h"
#include "mint/compiler/token.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/string.h"
#include "mint/system/terminal.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr bool is_standard_symbol(std::string_view token) {
	return token == "self" || token == "va_args";
}

}

class Highlighter : public mint::LexicalHandler {
public:
	Highlighter(std::size_t from_line, std::size_t to_line, std::size_t current_line, mint::GlobalData& global_data) :
	    _from_line(from_line),
	    _to_line(to_line),
	    _current_line(current_line),
	    _global_data(global_data) {}

protected:
	bool on_script_end() override {
		set_style(Style::text);
		print_highlighted("\n");
		return true;
	}

	bool on_symbol_token(const std::vector<std::string>& context, const std::string& token,
	    std::string::size_type /*offset*/) override {
		if (const auto* reference = find_defined_symbol(context, token)) {
			switch (reference->data().format()) {
			case mint::Data::none_format:
			case mint::Data::null_format:
			case mint::Data::number_format:
			case mint::Data::boolean_format:
				set_style(Style::constant);
				break;
			case mint::Data::function_format:
				set_style(Style::function);
				break;
			case mint::Data::object_format:
				if (mint::is_instance_of(*reference, mint::Class::object)
				    && mint::is_class(reference->data<mint::Object>())) {
					set_style(Style::user_type);
				}
				else {
					set_style(Style::constant);
				}
				break;
			case mint::Data::package_format:
				set_style(Style::user_type);
				break;
			}
		}
		else if (is_standard_symbol(token)) {
			set_style(Style::standard_symbol);
		}
		else {
			set_style(Style::text);
		}
		return true;
	}

	bool on_token(mint::Token type, const std::string& token, std::string::size_type /*offset*/) override {
		switch (type) {
		case mint::Token::line_end_token:
			return true;
		case mint::Token::assert_token:
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
		brace
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
		}
	}

	void print_line_number(std::size_t line_number) const {
		if (_print) {

			if (line_number != _from_line) {
				mint::Terminal::print(stdout, "\n");
			}

			auto amount_of_digits = [](std::size_t value) -> int {
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

	[[nodiscard]] const mint::Reference* find_defined_symbol(const std::vector<std::string>& context,
	    const std::string& token) const {

		const auto symbol = mint::Symbol(token);

		auto location = resolve_path(context);
		if (!location) {
			return nullptr;
		}

		auto [pack, desc] = *location;
		if (desc) {
			if (const auto* reference = desc->find_member(symbol)) {
				return reference;
			}
			return nullptr;
		}
		if (pack) {
			if (auto it = pack->symbols().find(symbol); it != pack->symbols().end()) {
				return std::addressof(it->second);
			}
			return nullptr;
		}

		const mint::GlobalData& global_data = _global_data;
		if (auto it = global_data.symbols().find(symbol); it != global_data.symbols().end()) {
			return std::addressof(it->second);
		}
		return nullptr;
	}

	[[nodiscard]] std::optional<std::tuple<const mint::PackageData*, const mint::ClassDescription*>> resolve_path(
	    const std::vector<std::string>& context) const {

		const mint::PackageData* pack = nullptr;
		const mint::ClassDescription* desc = nullptr;

		for (const std::string& token : context) {
			const auto symbol = mint::Symbol(token);
			if (desc) {
				desc = desc->find_class_description(symbol);
				if (desc == nullptr) {
					return std::nullopt;
				}
			}
			else if (pack) {
				desc = pack->find_class_description(symbol);
				if (desc == nullptr) {
					pack = pack->find_package(symbol);
					if (pack == nullptr) {
						return std::nullopt;
					}
				}
			}
			else {
				const mint::GlobalData& global_data = _global_data;
				desc = global_data.find_class_description(symbol);
				if (desc == nullptr) {
					pack = global_data.find_package(symbol);
					if (pack == nullptr) {
						return std::nullopt;
					}
				}
			}
		}

		return std::make_tuple(pack, desc);
	}

private:
	bool _print = false;
	std::size_t _from_line;
	std::size_t _to_line;
	std::size_t _current_line;
	std::reference_wrapper<mint::GlobalData> _global_data;
};

void print_highlighted(std::size_t from_line, std::size_t to_line, std::size_t current_line,
    mint::GlobalData& global_data, std::ifstream&& script) {
	Highlighter highlighter(from_line, to_line, current_line, global_data);
	std::ifstream stream = std::move(script);
	highlighter.parse(stream);
}
