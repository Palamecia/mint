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

#include "expressionevaluator.h"

#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/token.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/compiler/compiler.h"
#include "mint/memory/reference.h"
#include <array>
#include <cstddef>
#include <memory>
#include <string>

ExpressionEvaluator::ExpressionEvaluator(mint::AbstractSyntaxTree& ast) :
    _compiler(ast),
    _cursor(std::make_unique<mint::Cursor>(ast)) {}

ExpressionEvaluator::~ExpressionEvaluator() {
	_cursor->stack().clear();
}

void ExpressionEvaluator::setup_locals(const mint::SymbolTable& symbols) {
	for (const auto& [symbol, reference] : symbols) {
		_cursor->symbols().emplace(symbol, mint::WeakReference(mint::copy_from, reference));
	}
}

mint::Reference& ExpressionEvaluator::get_result() {
	return _cursor->stack().back();
}

bool ExpressionEvaluator::on_token(mint::Token type, const std::string& token, std::string::size_type /*offset*/) {
	switch (type) {
	case mint::Token::constant_token:
		switch (get_state()) {
		case State::read_operand:
			if (auto* data = _compiler.make_data(token, mint::Compiler::DataHint::data_unknown_hint)) {
				_cursor->stack().emplace_back(mint::Reference::const_address | mint::Reference::const_value, *data);
				set_state(State::read_operator);
				break;
			}
			return false;
		default:
			return false;
		}
		break;
	case mint::Token::string_token:
		switch (get_state()) {
		case State::read_operand:
			if (auto* data = _compiler.make_data(token, mint::Compiler::DataHint::data_string_hint)) {
				_cursor->stack().emplace_back(mint::Reference::const_address | mint::Reference::const_value, *data);
				set_state(State::read_operator);
				break;
			}
			return false;
		default:
			return false;
		}
		break;
	case mint::Token::number_token:
		switch (get_state()) {
		case State::read_operand:
			if (auto* data = _compiler.make_data(token, mint::Compiler::DataHint::data_number_hint)) {
				_cursor->stack().emplace_back(mint::Reference::const_address | mint::Reference::const_value, *data);
				set_state(State::read_operator);
				break;
			}
			return false;
		default:
			return false;
		}
		break;
	case mint::Token::regex_token:
		switch (get_state()) {
		case State::read_operand:
			if (auto* data = _compiler.make_data(token, mint::Compiler::DataHint::data_regex_hint)) {
				_cursor->stack().emplace_back(mint::Reference::const_address | mint::Reference::const_value, *data);
				set_state(State::read_operator);
				break;
			}
			return false;
		default:
			return false;
		}
		break;
	case mint::Token::symbol_token:
		switch (get_state()) {
		case State::read_operand:
			_cursor->stack().emplace_back(get_symbol(*_cursor, mint::Symbol(token)));
			set_state(State::read_operator);
			break;
		case State::read_member:
			reduce_member(*_cursor,
			    get_member_ignore_visibility(_cursor->ast(), _cursor->stack().back(), mint::Symbol(token)));
			set_state(State::read_operator);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::no_line_end_token:
		break;
	case mint::Token::line_end_token:
	case mint::Token::file_end_token:
		while (!_state.empty()) {
			pop_state();
		}
		break;
	case mint::Token::dbl_pipe_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(0, &mint::or_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::dbl_amp_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(1, &mint::and_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::pipe_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(2, &mint::bor_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::caret_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(3, &mint::xor_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::amp_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(4, &mint::band_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::dbl_dot_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::inclusive_range_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::tpl_dot_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::exclusive_range_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::dbl_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::eq_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::exclamation_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::ne_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::is_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::is_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::equal_tilde_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::regex_match);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::exclamation_tilde_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::regex_unmatch);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::tpl_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::strict_eq_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::exclamation_dbl_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::strict_ne_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::left_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::lt_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::right_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::gt_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::left_angled_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::le_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::right_angled_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::ge_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::dbl_left_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(9, &mint::shift_left_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::dbl_right_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(9, &mint::shift_right_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::plus_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(10, &mint::pos_operator);
			break;
		case State::read_operator:
			on_binary_operator(10, &mint::add_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::minus_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(10, &mint::neg_operator);
			break;
		case State::read_operator:
			on_binary_operator(10, &mint::sub_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::asterisk_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(11, &mint::mul_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::slash_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(11, &mint::div_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::percent_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(11, &mint::mod_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::exclamation_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::not_operator);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::tilde_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::neg_operator);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::typeof_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::typeof_operator);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::membersof_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::membersof_operator);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::defined_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::check_defined);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::dbl_asterisk_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(13, &mint::pow_operator);
			set_state(State::read_operand);
			break;
		default:
			return false;
		}
		break;
	case mint::Token::dot_token:
		if (get_state() != State::read_operator) {
			return false;
		}
		set_state(State::read_member);
		break;
	case mint::Token::open_parenthesis_token:
		push_state(State::read_operand);
		break;
	case mint::Token::close_parenthesis_token:
		pop_state();
		break;
	case mint::Token::open_bracket_token:
		push_state(State::read_operand);
		break;
	case mint::Token::close_bracket_token:
		subscript_operator(*_cursor);
		pop_state();
		break;
	case mint::Token::open_brace_token:
	case mint::Token::close_brace_token:
		break;
	default:
		return false;
	}
	return true;
}

ExpressionEvaluator::Associativity ExpressionEvaluator::associativity(std::size_t level) {
	static constexpr const std::array g_associativity {
	    // level  0: DBL_PIPE_TOKEN
	    Associativity::left_to_right,
	    // level  1: DBL_AMP_TOKEN
	    Associativity::left_to_right,
	    // level  2: PIPE_TOKEN
	    Associativity::left_to_right,
	    // level  3: CARET_TOKEN
	    Associativity::left_to_right,
	    // level  4: AMP_TOKEN
	    Associativity::left_to_right,
	    // level  5: QUESTION_TOKEN COLON_TOKEN
	    Associativity::right_to_left,
	    // level  6: DBL_DOT_TOKEN TPL_DOT_TOKEN
	    Associativity::left_to_right,
	    // level  7: DBL_EQUAL_TOKEN EXCLAMATION_EQUAL_TOKEN IS_TOKEN EQUAL_TILDE_TOKEN EXCLAMATION_TILDE_TOKEN
	    Associativity::left_to_right,
	    // level  8: LEFT_ANGLED_TOKEN RIGHT_ANGLED_TOKEN LEFT_ANGLED_EQUAL_TOKEN RIGHT_ANGLED_EQUAL_TOKEN
	    Associativity::left_to_right,
	    // level  9: DBL_LEFT_ANGLED_TOKEN DBL_RIGHT_ANGLED_TOKEN
	    Associativity::left_to_right,
	    // level 10: PLUS_TOKEN MINUS_TOKEN
	    Associativity::left_to_right,
	    // level 11: ASTERISK_TOKEN SLASH_TOKEN PERCENT_TOKEN
	    Associativity::left_to_right,
	    // level 12: EXCLAMATION_TOKEN TILDE_TOKEN TYPEOF_TOKEN MEMBERSOF_TOKEN DEFINED_TOKEN
	    Associativity::right_to_left,
	    // level 13: DBL_ASTERISK_TOKEN
	    Associativity::left_to_right,
	    // level 14: OPEN_PARENTHESIS_TOKEN CLOSE_PARENTHESIS_TOKEN OPEN_BRACKET_TOKEN CLOSE_BRACKET_TOKEN OPEN_BRACE_TOKEN CLOSE_BRACE_TOKEN
	    Associativity::left_to_right,
	};
	return g_associativity.at(level);
}

void ExpressionEvaluator::on_unary_operator(std::size_t level, void (*operation)(mint::Cursor&)) {

	EvaluatorState& state = _state.back();
	if (state.priority.empty()) {
		state.priority.push_back({
		    .level = level,
		    .unary_operations = {operation},
		    .binary_operations = {},
		});
	}
	else {
		Priority* priority = &state.priority.back();
		if (priority->level == level) {
			priority->unary_operations.push_back(operation);
		}
		else if (priority->level > level) {
			operation(*_cursor);
		}
		else if (priority->level < level) {
			state.priority.push_back({
			    .level = level,
			    .unary_operations = {operation},
			    .binary_operations = {},
			});
		}
	}
}

void ExpressionEvaluator::on_binary_operator(std::size_t level, void (*operation)(mint::Cursor&)) {

	EvaluatorState& state = _state.back();
	if (state.priority.empty()) {
		state.priority.push_back({
		    .level = level,
		    .unary_operations = {},
		    .binary_operations = {operation},
		});
	}
	else {
		Priority* priority = &state.priority.back();
		if (priority->level == level) {
			while (!priority->unary_operations.empty()) {
				priority->unary_operations.back()(*_cursor);
				priority->unary_operations.pop_back();
			}
			priority->binary_operations.push_back(operation);
		}
		else if (priority->level > level) {
			do {
				while (!priority->unary_operations.empty()) {
					priority->unary_operations.back()(*_cursor);
					priority->unary_operations.pop_back();
				}
				while (!priority->binary_operations.empty()) {
					priority->binary_operations.back()(*_cursor);
					priority->binary_operations.pop_back();
				}
				state.priority.pop_back();
				priority = state.priority.empty() ? nullptr : &state.priority.back();
			}
			while (priority && priority->level > level);
		}
		else if (priority->level < level) {
			state.priority.push_back({
			    .level = level,
			    .unary_operations = {},
			    .binary_operations = {operation},
			});
		}
	}
}

ExpressionEvaluator::State ExpressionEvaluator::get_state() const {
	if (_state.empty()) {
		return State::read_operand;
	}
	return _state.back().state;
}

void ExpressionEvaluator::push_state(State state) {
	_state.push_back({
	    .state = state,
	    .priority = {},
	});
}

void ExpressionEvaluator::set_state(State state) {
	if (_state.empty()) {
		_state.push_back({
		    .state = state,
		    .priority = {},
		});
	}
	else {
		_state.back().state = state;
	}
}

void ExpressionEvaluator::pop_state() {
	EvaluatorState& state = _state.back();
	while (!state.priority.empty()) {
		Priority& priority = state.priority.back();
		while (!priority.unary_operations.empty()) {
			priority.unary_operations.back()(*_cursor);
			priority.unary_operations.pop_back();
		}
		while (!priority.binary_operations.empty()) {
			priority.binary_operations.back()(*_cursor);
			priority.binary_operations.pop_back();
		}
		state.priority.pop_back();
	}
	_state.pop_back();
}
