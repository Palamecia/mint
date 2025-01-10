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
	case token::CONSTANT_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::DATA_UNKNOWN_HINT)));
			set_state(READ_OPERATOR);
			break;
		default:
			return false;
		}
		break;
	case token::STRING_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::DATA_STRING_HINT)));
			set_state(READ_OPERATOR);
			break;
		default:
			return false;
		}
		break;
	case token::NUMBER_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::DATA_NUMBER_HINT)));
			set_state(READ_OPERATOR);
			break;
		default:
			return false;
		}
		break;
	case token::REGEX_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			m_cursor->stack().emplace_back(WeakReference::create(Compiler::make_data(token, Compiler::DATA_REGEX_HINT)));
			set_state(READ_OPERATOR);
			break;
		default:
			return false;
		}
		break;
	case token::SYMBOL_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			m_cursor->stack().emplace_back(get_symbol(&m_cursor->symbols(), Symbol(token)));
			set_state(READ_OPERATOR);
			break;
		case READ_MEMBER:
			reduce_member(m_cursor.get(), get_member_ignore_visibility(m_cursor->stack().back(), Symbol(token)));
			set_state(READ_OPERATOR);
			break;
		default:
			return false;
		}
		break;
	case token::NO_LINE_END_TOKEN:
		break;
	case token::LINE_END_TOKEN:
	case token::FILE_END_TOKEN:
		while (!m_state.empty()) {
			pop_state();
		}
		break;
	case token::DBL_PIPE_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(0, &or_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::DBL_AMP_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(1, &and_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::PIPE_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(2, &bor_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::CARET_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(3, &xor_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::AMP_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(4, &band_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::DBL_DOT_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(6, &inclusive_range_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::TPL_DOT_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(6, &exclusive_range_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::DBL_EQUAL_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(7, &eq_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::EXCLAMATION_EQUAL_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(7, &ne_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::IS_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(7, &is_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::EQUAL_TILDE_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(7, &regex_match);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::EXCLAMATION_TILDE_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(7, &regex_unmatch);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::TPL_EQUAL_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(7, &strict_eq_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::EXCLAMATION_DBL_EQUAL_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(7, &strict_ne_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::LEFT_ANGLED_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(8, &lt_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::RIGHT_ANGLED_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(8, &gt_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::LEFT_ANGLED_EQUAL_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(8, &le_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::RIGHT_ANGLED_EQUAL_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(8, &ge_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::DBL_LEFT_ANGLED_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(9, &shift_left_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::DBL_RIGHT_ANGLED_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(9, &shift_right_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::PLUS_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			on_unary_operator(10, &pos_operator);
			break;
		case READ_OPERATOR:
			on_binary_operator(10, &add_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::MINUS_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			on_unary_operator(10, &neg_operator);
			break;
		case READ_OPERATOR:
			on_binary_operator(10, &sub_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::ASTERISK_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(11, &mul_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::SLASH_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(11, &div_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::PERCENT_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(11, &mod_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::EXCLAMATION_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			on_unary_operator(12, &not_operator);
			break;
		default:
			return false;
		}
		break;
	case token::TILDE_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			on_unary_operator(12, &neg_operator);
			break;
		default:
			return false;
		}
		break;
	case token::TYPEOF_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			on_unary_operator(12, &typeof_operator);
			break;
		default:
			return false;
		}
		break;
	case token::MEMBERSOF_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			on_unary_operator(12, &membersof_operator);
			break;
		default:
			return false;
		}
		break;
	case token::DEFINED_TOKEN:
		switch (get_state()) {
		case READ_OPERAND:
			on_unary_operator(12, &check_defined);
			break;
		default:
			return false;
		}
		break;
	case token::DBL_ASTERISK_TOKEN:
		switch (get_state()) {
		case READ_OPERATOR:
			on_binary_operator(13, &pow_operator);
			set_state(READ_OPERAND);
			break;
		default:
			return false;
		}
		break;
	case token::DOT_TOKEN:
		if (get_state() != READ_OPERATOR) {
			return false;
		}
		set_state(READ_MEMBER);
		break;
	case token::OPEN_PARENTHESIS_TOKEN:
		push_state(READ_OPERAND);
		break;
	case token::CLOSE_PARENTHESIS_TOKEN:
		pop_state();
		break;
	case token::OPEN_BRACKET_TOKEN:
		push_state(READ_OPERAND);
		break;
	case token::CLOSE_BRACKET_TOKEN:
		subscript_operator(m_cursor.get());
		pop_state();
		break;
	case token::OPEN_BRACE_TOKEN:
		break;
	case token::CLOSE_BRACE_TOKEN:
		break;
	default:
		return false;
	}
	return true;
}

ExpressionEvaluator::Associativity ExpressionEvaluator::associativity(int level) {
	static constexpr const Associativity g_associativity[] = {
		LEFT_TO_RIGHT, // level  0: DBL_PIPE_TOKEN
		LEFT_TO_RIGHT, // level  1: DBL_AMP_TOKEN
		LEFT_TO_RIGHT, // level  2: PIPE_TOKEN
		LEFT_TO_RIGHT, // level  3: CARET_TOKEN
		LEFT_TO_RIGHT, // level  4: AMP_TOKEN
		RIGHT_TO_LEFT, // level  5: QUESTION_TOKEN COLON_TOKEN
		LEFT_TO_RIGHT, // level  6: DBL_DOT_TOKEN TPL_DOT_TOKEN
		LEFT_TO_RIGHT, // level  7: DBL_EQUAL_TOKEN EXCLAMATION_EQUAL_TOKEN IS_TOKEN EQUAL_TILDE_TOKEN EXCLAMATION_TILDE_TOKEN
		LEFT_TO_RIGHT, // level  8: LEFT_ANGLED_TOKEN RIGHT_ANGLED_TOKEN LEFT_ANGLED_EQUAL_TOKEN RIGHT_ANGLED_EQUAL_TOKEN
		LEFT_TO_RIGHT, // level  9: DBL_LEFT_ANGLED_TOKEN DBL_RIGHT_ANGLED_TOKEN
		LEFT_TO_RIGHT, // level 10: PLUS_TOKEN MINUS_TOKEN
		LEFT_TO_RIGHT, // level 11: ASTERISK_TOKEN SLASH_TOKEN PERCENT_TOKEN
		RIGHT_TO_LEFT, // level 12: EXCLAMATION_TOKEN TILDE_TOKEN TYPEOF_TOKEN MEMBERSOF_TOKEN DEFINED_TOKEN
		LEFT_TO_RIGHT, // level 13: DBL_ASTERISK_TOKEN
		LEFT_TO_RIGHT  // level 14: OPEN_PARENTHESIS_TOKEN CLOSE_PARENTHESIS_TOKEN OPEN_BRACKET_TOKEN CLOSE_BRACKET_TOKEN OPEN_BRACE_TOKEN CLOSE_BRACE_TOKEN
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
		return READ_OPERAND;
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
