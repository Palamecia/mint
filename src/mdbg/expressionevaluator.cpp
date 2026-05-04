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
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/compiler/compiler.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/casttool.h"
#include "mint/memory/data.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

void complete_pending_calls(mint::Cursor& cursor) {
	while (cursor.call_in_progress()) {
		if (!mint::run_step(cursor)) {
			throw std::runtime_error("operator call did not complete");
		}
	}
}

void find_expression_operator(mint::Cursor& cursor) {
	mint::find_operator(cursor);
	complete_pending_calls(cursor);
	mint::find_init(cursor);
	complete_pending_calls(cursor);

	while (true) {
		mint::find_next(cursor);
		complete_pending_calls(cursor);

		auto found = std::move(cursor.stack().back());
		cursor.stack().pop_back();

		const auto& range = cursor.stack().back();
		if (range.data().format() == mint::Data::Format::boolean || mint::to_boolean(found)
		    || range.data<mint::Iterator>().ctx.empty()) {
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(found);
			return;
		}
	}
}

void find_not_expression_operator(mint::Cursor& cursor) {
	find_expression_operator(cursor);
	mint::not_operator(cursor);
	complete_pending_calls(cursor);
}

}

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
	if (_state.size() > 1) {
		parse_error("unclosed expression delimiter");
	}
	while (!_state.empty()) {
		pop_state();
	}
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
			parse_error("unexpected token \'" + token + "\'");
		default:
			parse_error("unexpected token \'" + token + "\'");
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
			parse_error("unexpected token \'" + token + "\'");
		default:
			parse_error("unexpected token \'" + token + "\'");
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
			parse_error("unexpected token \'" + token + "\'");
		default:
			parse_error("unexpected token \'" + token + "\'");
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
			parse_error("unexpected token \'" + token + "\'");
		default:
			parse_error("unexpected token \'" + token + "\'");
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
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::no_line_end_token:
		break;
	case mint::Token::line_end_token:
	case mint::Token::file_end_token:
		if (_state.size() > 1) {
			parse_error("unclosed expression delimiter");
		}
		while (!_state.empty()) {
			pop_state();
		}
		break;
	case mint::Token::comma_token:
		on_separator(token);
		break;
	case mint::Token::colon_token:
		on_hash_key_separator();
		break;
	case mint::Token::dbl_pipe_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(0, &mint::or_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::dbl_amp_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(1, &mint::and_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::pipe_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(2, &mint::bor_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::caret_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(3, &mint::xor_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::amp_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(4, &mint::band_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::in_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &find_expression_operator);
			set_state(State::read_operand);
			break;
		case State::read_in_operator:
			on_binary_operator(6, &find_not_expression_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::dbl_dot_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::inclusive_range_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::tpl_dot_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(7, &mint::exclusive_range_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::dbl_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::eq_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::exclamation_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::ne_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::is_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::is_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::equal_tilde_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::regex_match);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::exclamation_tilde_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::regex_unmatch);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::tpl_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::strict_eq_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::exclamation_dbl_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(6, &mint::strict_ne_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::left_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::lt_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::right_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::gt_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::left_angled_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::le_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::right_angled_equal_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(8, &mint::ge_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::dbl_left_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(9, &mint::shift_left_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::dbl_right_angled_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(9, &mint::shift_right_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
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
			parse_error("unexpected token \'" + token + "\'");
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
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::asterisk_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(11, &mint::mul_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::slash_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(11, &mint::div_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::percent_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(11, &mint::mod_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::exclamation_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::not_operator);
			break;
		case State::read_operator:
			set_state(State::read_in_operator);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::tilde_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::neg_operator);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::typeof_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::typeof_operator);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::membersof_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::membersof_operator);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::defined_token:
		switch (get_state()) {
		case State::read_operand:
			on_unary_operator(12, &mint::check_defined);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::dbl_asterisk_token:
		switch (get_state()) {
		case State::read_operator:
			on_binary_operator(13, &mint::pow_operator);
			set_state(State::read_operand);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::dot_token:
		if (get_state() != State::read_operator) {
			parse_error("unexpected token \'" + token + "\'");
		}
		set_state(State::read_member);
		break;
	case mint::Token::open_parenthesis_token:
		if (get_state() != State::read_operand) {
			parse_error("unexpected token \'" + token + "\'");
		}
		push_state(State::read_operand, FrameKind::group_or_iterator);
		break;
	case mint::Token::close_parenthesis_token:
		close_state(type);
		break;
	case mint::Token::open_bracket_token:
		switch (get_state()) {
		case State::read_operand:
			push_state(State::read_operand, FrameKind::array_literal);
			break;
		case State::read_operator:
			push_state(State::read_operand, FrameKind::subscript);
			break;
		default:
			parse_error("unexpected token \'" + token + "\'");
		}
		break;
	case mint::Token::close_bracket_token:
		close_state(type);
		break;
	case mint::Token::open_brace_token:
		if (get_state() != State::read_operand) {
			parse_error("unexpected token \'" + token + "\'");
		}
		push_state(State::read_operand, FrameKind::hash_literal);
		break;
	case mint::Token::close_brace_token:
		close_state(type);
		break;
	default:
		parse_error("unexpected token \'" + token + "\'");
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
	    // level  6: DBL_EQUAL_TOKEN EXCLAMATION_EQUAL_TOKEN IS_TOKEN IN_TOKEN EQUAL_TILDE_TOKEN EXCLAMATION_TILDE_TOKEN
	    Associativity::left_to_right,
	    // level  7: DBL_DOT_TOKEN TPL_DOT_TOKEN
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
			apply_operation(operation);
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
	auto push_priority = [&state, level, operation] {
		state.priority.push_back({
		    .level = level,
		    .unary_operations = {},
		    .binary_operations = {operation},
		});
	};

	if (state.priority.empty()) {
		push_priority();
		return;
	}

	Priority* priority = &state.priority.back();
	if (priority->level > level) {
		do {
			while (!priority->unary_operations.empty()) {
				apply_operation(priority->unary_operations.back());
				priority->unary_operations.pop_back();
			}
			while (!priority->binary_operations.empty()) {
				apply_operation(priority->binary_operations.back());
				priority->binary_operations.pop_back();
			}
			state.priority.pop_back();
			priority = state.priority.empty() ? nullptr : &state.priority.back();
		}
		while (priority && priority->level > level);
	}

	if (!priority) {
		push_priority();
	}
	else if (priority->level == level) {
		while (!priority->unary_operations.empty()) {
			apply_operation(priority->unary_operations.back());
			priority->unary_operations.pop_back();
		}
		if (associativity(level) == Associativity::left_to_right) {
			while (!priority->binary_operations.empty()) {
				apply_operation(priority->binary_operations.back());
				priority->binary_operations.pop_back();
			}
		}
		priority->binary_operations.push_back(operation);
	}
	else if (priority->level < level) {
		push_priority();
	}
}

void ExpressionEvaluator::apply_operation(void (*operation)(mint::Cursor&)) {
	operation(*_cursor);
	complete_pending_calls(*_cursor);
}

void ExpressionEvaluator::reduce_state(EvaluatorState& state) {
	while (!state.priority.empty()) {
		Priority& priority = state.priority.back();
		while (!priority.unary_operations.empty()) {
			apply_operation(priority.unary_operations.back());
			priority.unary_operations.pop_back();
		}
		while (!priority.binary_operations.empty()) {
			apply_operation(priority.binary_operations.back());
			priority.binary_operations.pop_back();
		}
		state.priority.pop_back();
	}
}

void ExpressionEvaluator::close_state(mint::Token close_token) {
	if (_state.empty()) {
		parse_error("unexpected closing token");
	}

	EvaluatorState& state = _state.back();
	const FrameKind kind = state.kind;
	const std::size_t stack_base = state.stack_base;

	if ((close_token == mint::Token::close_parenthesis_token && kind != FrameKind::group_or_iterator)
	    || (close_token == mint::Token::close_bracket_token && kind != FrameKind::array_literal
	        && kind != FrameKind::subscript)
	    || (close_token == mint::Token::close_brace_token && kind != FrameKind::hash_literal)) {
		parse_error("mismatched closing token");
	}

	reduce_state(state);

	switch (kind) {
	case FrameKind::root:
		parse_error("unexpected closing token");

	case FrameKind::group_or_iterator:
		if (state.saw_separator || _cursor->stack().size() == stack_base) {
			auto iterator = mint::create_iterator(_cursor->ast());
			auto& data = iterator.data<mint::Iterator>();
			for (auto it = std::next(_cursor->stack().begin(), static_cast<std::ptrdiff_t>(stack_base));
			    it != _cursor->stack().end(); ++it) {
				mint::iterator_yield(*_cursor, data, std::move(*it));
			}
			_cursor->stack().resize(stack_base);
			_cursor->stack().emplace_back(std::move(iterator));
		}
		else if (_cursor->stack().size() != stack_base + 1) {
			parse_error("parenthesized expression must contain exactly one expression");
		}
		break;

	case FrameKind::array_literal:
		{
			auto array = mint::create_array(_cursor->ast());
			auto& data = array.data<mint::Array>();
			for (auto it = std::next(_cursor->stack().begin(), static_cast<std::ptrdiff_t>(stack_base));
			    it != _cursor->stack().end(); ++it) {
				mint::array_append(data, std::move(*it));
			}
			_cursor->stack().resize(stack_base);
			_cursor->stack().emplace_back(std::move(array));
		}
		break;

	case FrameKind::hash_literal:
		if (state.pending_hash_value) {
			if (_cursor->stack().size() != stack_base + (state.hash_pairs * 2) + 2) {
				parse_error("hash item value is missing");
			}
			++state.hash_pairs;
		}
		else if (_cursor->stack().size() != stack_base + (state.hash_pairs * 2)) {
			parse_error("hash item is missing ':'");
		}
		{
			auto hash = mint::create_hash(_cursor->ast());
			auto& data = hash.data<mint::Hash>();
			for (std::size_t index = 0; index < state.hash_pairs; ++index) {
				auto& key = _cursor->stack()[stack_base + (index * 2)];
				auto& value = _cursor->stack()[stack_base + (index * 2) + 1];
				mint::hash_insert(data, mint::hash_key(key), value);
			}
			_cursor->stack().resize(stack_base);
			_cursor->stack().emplace_back(std::move(hash));
		}
		break;

	case FrameKind::subscript:
		if (_cursor->stack().size() != stack_base + 1) {
			parse_error("subscript operator expects exactly one index expression");
		}
		apply_operation(&mint::subscript_operator);
		break;
	}

	_state.pop_back();
	set_state(State::read_operator);
}

void ExpressionEvaluator::on_separator(const std::string& /*token*/) {
	if (_state.empty()) {
		parse_error("unexpected ','");
	}

	EvaluatorState& state = _state.back();
	reduce_state(state);

	switch (state.kind) {
	case FrameKind::group_or_iterator:
		if (get_state() != State::read_operator && _cursor->stack().size() == state.stack_base) {
			parse_error("iterator item is missing before ','");
		}
		state.saw_separator = true;
		set_state(State::read_operand);
		break;

	case FrameKind::array_literal:
		if (get_state() != State::read_operator) {
			parse_error("array item is missing before ','");
		}
		set_state(State::read_operand);
		break;

	case FrameKind::hash_literal:
		if (!state.pending_hash_value) {
			parse_error("hash item is missing ':' before ','");
		}
		if (_cursor->stack().size() != state.stack_base + (state.hash_pairs * 2) + 2) {
			parse_error("hash item value is missing before ','");
		}
		++state.hash_pairs;
		state.pending_hash_value = false;
		set_state(State::read_operand);
		break;

	default:
		parse_error("unexpected ','");
	}
}

void ExpressionEvaluator::on_hash_key_separator() {
	if (_state.empty() || _state.back().kind != FrameKind::hash_literal) {
		parse_error("unexpected ':'");
	}

	EvaluatorState& state = _state.back();
	reduce_state(state);

	if (state.pending_hash_value) {
		parse_error("hash item already has a key");
	}
	if (_cursor->stack().size() != state.stack_base + (state.hash_pairs * 2) + 1) {
		parse_error("hash key is missing before ':'");
	}

	state.pending_hash_value = true;
	set_state(State::read_operand);
}

void ExpressionEvaluator::parse_error(const std::string& message) const {
	throw std::runtime_error("debug expression parse error: " + message);
}

ExpressionEvaluator::State ExpressionEvaluator::get_state() const {
	if (_state.empty()) {
		return State::read_operand;
	}
	return _state.back().state;
}

void ExpressionEvaluator::push_state(State state, FrameKind kind) {
	_state.push_back({
	    .state = state,
	    .kind = kind,
	    .stack_base = _cursor->stack().size(),
	    .priority = {},
	});
}

void ExpressionEvaluator::set_state(State state) {
	if (_state.empty()) {
		_state.push_back({
		    .state = state,
		    .kind = FrameKind::root,
		    .stack_base = _cursor->stack().size(),
		    .priority = {},
		});
	}
	else {
		_state.back().state = state;
	}
}

void ExpressionEvaluator::pop_state() {
	EvaluatorState& state = _state.back();
	reduce_state(state);
	_state.pop_back();
}
