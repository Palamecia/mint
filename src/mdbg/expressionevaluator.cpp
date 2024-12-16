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

#include "expressionevaluator.h"

#include <mint/ast/abstractsyntaxtree.h>
#include <mint/memory/functiontool.h>
#include <mint/memory/operatortool.h>
#include <mint/memory/memorytool.h>
#include <mint/compiler/compiler.h>

using namespace mint;

ExpressionEvaluator::ExpressionEvaluator(AbstractSyntaxTree *ast) :
	m_cursor(ast->create_cursor()) {

}

ExpressionEvaluator::~ExpressionEvaluator() {
	m_cursor->stack().clear();
}

void ExpressionEvaluator::setup_locals(const SymbolTable &symbols) {
	for (auto &[symbol, reference] : symbols) {
		m_cursor->symbols().emplace(symbol, WeakReference::clone(reference));
	}
}

Reference &ExpressionEvaluator::get_result() {
	return m_cursor->stack().back();
}

bool ExpressionEvaluator::on_token(token::Type type, const std::string &token, std::string::size_type offset) {
	switch (type) {
	case token::constant_token:
		switch (get_state()) {
		case read_operand:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::data_unknown_hint)));
			set_state(read_operator);
			break;
		default:
			return false;
		}
		break;
	case token::string_token:
		switch (get_state()) {
		case read_operand:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::data_string_hint)));
			set_state(read_operator);
			break;
		default:
			return false;
		}
		break;
	case token::number_token:
		switch (get_state()) {
		case read_operand:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::data_number_hint)));
			set_state(read_operator);
			break;
		default:
			return false;
		}
		break;
	case token::regex_token:
		switch (get_state()) {
		case read_operand:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::data_regex_hint)));
			set_state(read_operator);
			break;
		default:
			return false;
		}
		break;
	case token::symbol_token:
		switch (get_state()) {
		case read_operand:
			m_cursor->stack().emplace_back(get_symbol(&m_cursor->symbols(), Symbol(token)));
			set_state(read_operator);
			break;
		case read_member:
			reduce_member(m_cursor.get(), get_member_ignore_visibility(m_cursor->stack().back(), Symbol(token)));
			set_state(read_operator);
			break;
		default:
			return false;
		}
		break;
	case token::no_line_end_token:
		break;
	case token::line_end_token:
	case token::file_end_token:
		while (!m_state.empty()) {
			pop_state();
		}
		break;
	case token::dbl_pipe_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(0, &or_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::dbl_amp_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(1, &and_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::pipe_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(2, &bor_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::caret_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(3, &xor_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::amp_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(4, &band_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::dot_dot_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(6, &inclusive_range_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::tpl_dot_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(6, &exclusive_range_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::dbl_equal_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(7, &eq_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::exclamation_equal_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(7, &ne_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::is_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(7, &is_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::equal_tilde_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(7, &regex_match);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::exclamation_tilde_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(7, &regex_unmatch);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::tpl_equal_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(7, &strict_eq_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::exclamation_dbl_equal_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(7, &strict_ne_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::left_angled_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(8, &lt_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::right_angled_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(8, &gt_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::left_angled_equal_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(8, &le_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::right_angled_equal_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(8, &ge_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::dbl_left_angled_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(9, &shift_left_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::dbl_right_angled_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(9, &shift_right_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::plus_token:
		switch (get_state()) {
		case read_operand:
			on_unary_operator(10, &pos_operator);
			break;
		case read_operator:
			on_binary_operator(10, &add_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::minus_token:
		switch (get_state()) {
		case read_operand:
			on_unary_operator(10, &neg_operator);
			break;
		case read_operator:
			on_binary_operator(10, &sub_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::asterisk_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(11, &mul_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::slash_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(11, &div_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::percent_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(11, &mod_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::exclamation_token:
		switch (get_state()) {
		case read_operand:
			on_unary_operator(12, &not_operator);
			break;
		default:
			return false;
		}
		break;
	case token::tilde_token:
		switch (get_state()) {
		case read_operand:
			on_unary_operator(12, &neg_operator);
			break;
		default:
			return false;
		}
		break;
	case token::typeof_token:
		switch (get_state()) {
		case read_operand:
			on_unary_operator(12, &typeof_operator);
			break;
		default:
			return false;
		}
		break;
	case token::membersof_token:
		switch (get_state()) {
		case read_operand:
			on_unary_operator(12, &membersof_operator);
			break;
		default:
			return false;
		}
		break;
	case token::defined_token:
		switch (get_state()) {
		case read_operand:
			on_unary_operator(12, &check_defined);
			break;
		default:
			return false;
		}
		break;
	case token::dbl_asterisk_token:
		switch (get_state()) {
		case read_operator:
			on_binary_operator(13, &pow_operator);
			set_state(read_operand);
			break;
		default:
			return false;
		}
		break;
	case token::dot_token:
		if (get_state() != read_operator) {
			return false;
		}
		set_state(read_member);
		break;
	case token::open_parenthesis_token:
		push_state(read_operand);
		break;
	case token::close_parenthesis_token:
		pop_state();
		break;
	case token::open_bracket_token:
		push_state(read_operand);
		break;
	case token::close_bracket_token:
		subscript_operator(m_cursor.get());
		pop_state();
		break;
	case token::open_brace_token:
		break;
	case token::close_brace_token:
		break;
	default:
		return false;
	}
	return true;
}

ExpressionEvaluator::Associativity ExpressionEvaluator::associativity(int level) {
	static constexpr const Associativity g_associativity[] = {
		left_to_right, // level  0: dbl_pipe_token
		left_to_right, // level  1: dbl_amp_token
		left_to_right, // level  2: pipe_token
		left_to_right, // level  3: caret_token
		left_to_right, // level  4: amp_token
		right_to_left, // level  5: question_token dbldot_token
		left_to_right, // level  6: dot_dot_token tpl_dot_token
		left_to_right, // level  7: dbl_equal_token exclamation_equal_token is_token equal_tilde_token exclamation_tilde_token
		left_to_right, // level  8: left_angled_token right_angled_token left_angled_equal_token right_angled_equal_token
		left_to_right, // level  9: dbl_left_angled_token dbl_right_angled_token
		left_to_right, // level 10: plus_token minus_token
		left_to_right, // level 11: asterisk_token slash_token percent_token
		right_to_left, // level 12: exclamation_token tilde_token typeof_token membersof_token defined_token
		left_to_right, // level 13: dbl_asterisk_token
		left_to_right  // level 14: open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token
	};
	return g_associativity[level];
}

void ExpressionEvaluator::on_unary_operator(int level, void(*operation)(Cursor*)) {

	state_t &state = m_state.back();
	if (state.priority.empty()) {
		state.priority.push_back(priority_t { level, { operation }, {} });
	}
	else {
		priority_t *priority = &state.priority.back();
		if (priority->level == level) {
			priority->unary_operations.push_back(operation);
		}
		else if (priority->level > level) {
			operation(m_cursor.get());
		}
		else if (priority->level < level) {
			state.priority.push_back(priority_t { level, { operation }, {} });
		}
	}
}

void ExpressionEvaluator::on_binary_operator(int level, void(*operation)(Cursor*)) {

	state_t &state = m_state.back();
	if (state.priority.empty()) {
		state.priority.push_back(priority_t { level, {}, { operation } });
	}
	else {
		priority_t *priority = &state.priority.back();
		if (priority->level == level) {
			while (!priority->unary_operations.empty()) {
				priority->unary_operations.back()(m_cursor.get());
				priority->unary_operations.pop_back();
			}
			priority->binary_operations.push_back(operation);
		}
		else if (priority->level > level) {
			do {
				while (!priority->unary_operations.empty()) {
					priority->unary_operations.back()(m_cursor.get());
					priority->unary_operations.pop_back();
				}
				while (!priority->binary_operations.empty()) {
					priority->binary_operations.back()(m_cursor.get());
					priority->binary_operations.pop_back();
				}
				state.priority.pop_back();
				priority = state.priority.empty() ? nullptr : &state.priority.back();
			}
			while (priority && priority->level > level);
		}
		else if (priority->level < level) {
			state.priority.push_back(priority_t { level, {}, { operation } });
		}
	}
}

ExpressionEvaluator::State ExpressionEvaluator::get_state() const {
	if (m_state.empty()) {
		return read_operand;
	}
	return m_state.back().state;
}

void ExpressionEvaluator::push_state(State state) {
	m_state.push_back(state_t { state, {} });
}

void ExpressionEvaluator::set_state(State state) {
	if (m_state.empty()) {
		m_state.push_back(state_t { state, {} });
	}
	else {
		m_state.back().state = state;
	}
}

void ExpressionEvaluator::pop_state() {
	state_t &state = m_state.back();
	while (!state.priority.empty()) {
		priority_t &priority = state.priority.back();
		while (!priority.unary_operations.empty()) {
			priority.unary_operations.back()(m_cursor.get());
			priority.unary_operations.pop_back();
		}
		while (!priority.binary_operations.empty()) {
			priority.binary_operations.back()(m_cursor.get());
			priority.binary_operations.pop_back();
		}
		state.priority.pop_back();
	}
	m_state.pop_back();
}
