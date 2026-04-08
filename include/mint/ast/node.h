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

#ifndef MINT_AST_NODE_H
#define MINT_AST_NODE_H

#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/reference.h"
#include <cstdint>

namespace mint {

union MINT_EXPORT Node {
	enum class Command : std::uint8_t {
		load_module,

		load_fast,
		load_symbol,
		load_member,
		load_operator,
		load_constant,
		load_var_symbol,
		load_var_member,
		load_defined_member,
		load_defined_operator,
		load_defined_var_member,
		clone_reference,
		reload_reference,
		unload_reference,
		load_extra_arguments,
		reset_symbol,
		reset_fast,

		declare_fast,
		declare_symbol,
		declare_function,
		function_overload,
		alloc_iterator,
		init_iterator,
		alloc_array,
		init_array,
		alloc_hash,
		init_hash,
		create_lib,

		regex_match,
		regex_unmatch,

		strict_eq_operator,
		strict_ne_operator,

		open_package,
		close_package,
		register_class,

		move_operator,
		copy_operator,
		add_operator,
		sub_operator,
		mod_operator,
		mul_operator,
		div_operator,
		pow_operator,
		is_operator,
		eq_operator,
		ne_operator,
		lt_operator,
		gt_operator,
		le_operator,
		ge_operator,
		inc_operator,
		dec_operator,
		not_operator,
		and_operator,
		or_operator,
		band_operator,
		bor_operator,
		xor_operator,
		compl_operator,
		pos_operator,
		neg_operator,
		shift_left_operator,
		shift_right_operator,
		inclusive_range_operator,
		exclusive_range_operator,
		subscript_operator,
		subscript_move_operator,
		typeof_operator,
		membersof_operator,
		find_operator,
		in_operator,

		find_defined_symbol,
		find_defined_member,
		find_defined_var_symbol,
		find_defined_var_member,
		check_defined,

		find_init,
		find_next,
		find_check,
		range_init,
		range_next,
		range_check,
		range_iterator_check,

		begin_generator_expression,
		begin_async_generator_expression,
		end_generator_expression,
		end_async_generator_expression,

		open_printer,
		close_printer,
		print,

		or_pre_check,
		and_pre_check,
		case_jump,
		zero_jump,
		jump,

		set_retrieve_point,
		unset_retrieve_point,
		raise,

		await,
		resume_coroutine,

		yield,
		exit_generator,
		exit_async_generator,
		yield_exit_generator,
		yield_exit_async_generator,

		init_capture,
		capture_symbol,
		capture_as,
		capture_all,
		call,
		call_member,
		call_builtin,
		init_call,
		init_member_call,
		init_operator_call,
		init_var_member_call,
		init_defined_member_call,
		init_defined_operator_call,
		init_defined_var_member_call,
		init_exception,
		reset_exception,
		init_parameter,
		exit_call,
		exit_thread,
		exit_exec,
		exit_module
	};

	Node(Command command);
	Node(int parameter);
	Node(const Symbol* symbol);
	Node(const Reference* constant);

	[[nodiscard]] Command as_command() const {
		return command;
	}

	[[nodiscard]] int as_parameter() const {
		return parameter;
	}

	[[nodiscard]] const Symbol& as_symbol() const {
		return *symbol;
	}

	[[nodiscard]] const Reference& as_constant() const {
		return *constant;
	}

	Command command;
	int parameter;
	const Symbol* symbol;
	const Reference* constant;
};

}

#endif // MINT_AST_NODE_H
