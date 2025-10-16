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

#ifndef MINT_AST_ABSTRACTSYNTAXTREEWALKER_H
#define MINT_AST_ABSTRACTSYNTAXTREEWALKER_H

#include "mint/ast/classregister.h"
#include "mint/ast/cursor.h"
#include "mint/ast/node.h"
#include "mint/memory/class.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <type_traits>

namespace mint {

template<class R, class Walker>
R walk(Cursor& cursor, Walker& walker) {
	switch (cursor.next().as_command()) {
	case Node::Command::load_module:
		return walker.on_load_module(cursor, cursor.next().as_symbol());
	case Node::Command::load_fast:
		{
			const auto& symbol = cursor.next().as_symbol();
			const auto index = static_cast<std::size_t>(cursor.next().as_parameter());
			return walker.on_load_fast(cursor, symbol, index);
		}
	case Node::Command::load_symbol:
		return walker.on_load_symbol(cursor, cursor.next().as_symbol());
	case Node::Command::load_member:
		return walker.on_load_member(cursor, cursor.next().as_symbol());
	case Node::Command::load_operator:
		return walker.on_load_operator(cursor, static_cast<Class::Operator>(cursor.next().as_parameter()));
	case Node::Command::load_constant:
		return walker.on_load_constant(cursor, cursor.next().as_constant());
	case Node::Command::load_var_symbol:
		return walker.on_load_var_symbol(cursor);
	case Node::Command::load_var_member:
		return walker.on_load_var_member(cursor);
	case Node::Command::load_defined_member:
		return walker.on_load_defined_member(cursor, cursor.next().as_symbol());
	case Node::Command::load_defined_operator:
		return walker.on_load_defined_operator(cursor, static_cast<Class::Operator>(cursor.next().as_parameter()));
	case Node::Command::load_defined_var_member:
		return walker.on_load_defined_var_member(cursor);
	case Node::Command::clone_reference:
		return walker.on_clone_reference(cursor);
	case Node::Command::reload_reference:
		return walker.on_reload_reference(cursor);
	case Node::Command::unload_reference:
		return walker.on_unload_reference(cursor);
	case Node::Command::load_extra_arguments:
		return walker.on_load_extra_arguments(cursor);
	case Node::Command::reset_symbol:
		return walker.on_reset_symbol(cursor, cursor.next().as_symbol());
	case Node::Command::reset_fast:
		{
			const auto& symbol = cursor.next().as_symbol();
			const auto index = static_cast<std::size_t>(cursor.next().as_parameter());
			return walker.on_reset_fast(cursor, symbol, index);
		}
	case Node::Command::declare_fast:
		{
			const auto& symbol = cursor.next().as_symbol();
			const auto index = static_cast<std::size_t>(cursor.next().as_parameter());
			const auto flags = static_cast<Reference::Flags>(cursor.next().as_parameter());
			return walker.on_declare_fast(cursor, symbol, index, flags);
		}
	case Node::Command::declare_symbol:
		{
			const auto& symbol = cursor.next().as_symbol();
			const auto flags = static_cast<Reference::Flags>(cursor.next().as_parameter());
			return walker.on_declare_symbol(cursor, symbol, flags);
		}
	case Node::Command::declare_function:
		{
			const auto& symbol = cursor.next().as_symbol();
			const auto flags = static_cast<Reference::Flags>(cursor.next().as_parameter());
			return walker.on_declare_function(cursor, symbol, flags);
		}
	case Node::Command::function_overload:
		return walker.on_function_overload(cursor);
	case Node::Command::alloc_iterator:
		return walker.on_alloc_iterator(cursor);
	case Node::Command::init_iterator:
		return walker.on_init_iterator(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::alloc_array:
		return walker.on_alloc_array(cursor);
	case Node::Command::init_array:
		return walker.on_init_array(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::alloc_hash:
		return walker.on_alloc_hash(cursor);
	case Node::Command::init_hash:
		return walker.on_init_hash(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::create_lib:
		return walker.on_create_lib(cursor);
	case Node::Command::regex_match:
		return walker.on_regex_match(cursor);
	case Node::Command::regex_unmatch:
		return walker.on_regex_unmatch(cursor);
	case Node::Command::strict_eq_operator:
		return walker.on_strict_eq_operator(cursor);
	case Node::Command::strict_ne_operator:
		return walker.on_strict_ne_operator(cursor);
	case Node::Command::open_package:
		return walker.on_open_package(cursor, cursor.next().as_constant().data<Package>());
	case Node::Command::close_package:
		return walker.on_close_package(cursor);
	case Node::Command::register_class:
		return walker.on_register_class(cursor, static_cast<ClassRegister::Id>(cursor.next().as_parameter()));
	case Node::Command::move_operator:
		return walker.on_move_operator(cursor);
	case Node::Command::copy_operator:
		return walker.on_copy_operator(cursor);
	case Node::Command::add_operator:
		return walker.on_add_operator(cursor);
	case Node::Command::sub_operator:
		return walker.on_sub_operator(cursor);
	case Node::Command::mod_operator:
		return walker.on_mod_operator(cursor);
	case Node::Command::mul_operator:
		return walker.on_mul_operator(cursor);
	case Node::Command::div_operator:
		return walker.on_div_operator(cursor);
	case Node::Command::pow_operator:
		return walker.on_pow_operator(cursor);
	case Node::Command::is_operator:
		return walker.on_is_operator(cursor);
	case Node::Command::eq_operator:
		return walker.on_eq_operator(cursor);
	case Node::Command::ne_operator:
		return walker.on_ne_operator(cursor);
	case Node::Command::lt_operator:
		return walker.on_lt_operator(cursor);
	case Node::Command::gt_operator:
		return walker.on_gt_operator(cursor);
	case Node::Command::le_operator:
		return walker.on_le_operator(cursor);
	case Node::Command::ge_operator:
		return walker.on_ge_operator(cursor);
	case Node::Command::inc_operator:
		return walker.on_inc_operator(cursor);
	case Node::Command::dec_operator:
		return walker.on_dec_operator(cursor);
	case Node::Command::not_operator:
		return walker.on_not_operator(cursor);
	case Node::Command::and_operator:
		return walker.on_and_operator(cursor);
	case Node::Command::or_operator:
		return walker.on_or_operator(cursor);
	case Node::Command::band_operator:
		return walker.on_band_operator(cursor);
	case Node::Command::bor_operator:
		return walker.on_bor_operator(cursor);
	case Node::Command::xor_operator:
		return walker.on_xor_operator(cursor);
	case Node::Command::compl_operator:
		return walker.on_compl_operator(cursor);
	case Node::Command::pos_operator:
		return walker.on_pos_operator(cursor);
	case Node::Command::neg_operator:
		return walker.on_neg_operator(cursor);
	case Node::Command::shift_left_operator:
		return walker.on_shift_left_operator(cursor);
	case Node::Command::shift_right_operator:
		return walker.on_shift_right_operator(cursor);
	case Node::Command::inclusive_range_operator:
		return walker.on_inclusive_range_operator(cursor);
	case Node::Command::exclusive_range_operator:
		return walker.on_exclusive_range_operator(cursor);
	case Node::Command::subscript_operator:
		return walker.on_subscript_operator(cursor);
	case Node::Command::subscript_move_operator:
		return walker.on_subscript_move_operator(cursor);
	case Node::Command::typeof_operator:
		return walker.on_typeof_operator(cursor);
	case Node::Command::membersof_operator:
		return walker.on_membersof_operator(cursor);
	case Node::Command::find_operator:
		return walker.on_find_operator(cursor);
	case Node::Command::in_operator:
		return walker.on_in_operator(cursor);
	case Node::Command::find_defined_symbol:
		return walker.on_find_defined_symbol(cursor, cursor.next().as_symbol());
	case Node::Command::find_defined_member:
		return walker.on_find_defined_member(cursor, cursor.next().as_symbol());
	case Node::Command::find_defined_var_symbol:
		return walker.on_find_defined_var_symbol(cursor);
	case Node::Command::find_defined_var_member:
		return walker.on_find_defined_var_member(cursor);
	case Node::Command::check_defined:
		return walker.on_check_defined(cursor);
	case Node::Command::find_init:
		return walker.on_find_init(cursor);
	case Node::Command::find_next:
		return walker.on_find_next(cursor);
	case Node::Command::find_check:
		return walker.on_find_check(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::range_init:
		return walker.on_range_init(cursor);
	case Node::Command::range_next:
		return walker.on_range_next(cursor);
	case Node::Command::range_check:
		return walker.on_range_check(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::range_iterator_check:
		return walker.on_range_iterator_check(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::begin_generator_expression:
		return walker.on_begin_generator_expression(cursor);
	case Node::Command::end_generator_expression:
		return walker.on_end_generator_expression(cursor);
	case Node::Command::yield_expression:
		return walker.on_yield_expression(cursor);
	case Node::Command::open_printer:
		return walker.on_open_printer(cursor);
	case Node::Command::close_printer:
		return walker.on_close_printer(cursor);
	case Node::Command::print:
		return walker.on_print(cursor);
	case Node::Command::or_pre_check:
		return walker.on_or_pre_check(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::and_pre_check:
		return walker.on_and_pre_check(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::case_jump:
		return walker.on_case_jump(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::zero_jump:
		return walker.on_zero_jump(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::jump:
		return walker.on_jump(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::set_retrieve_point:
		return walker.on_set_retrieve_point(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::unset_retrieve_point:
		return walker.on_unset_retrieve_point(cursor);
	case Node::Command::raise:
		return walker.on_raise(cursor);
	case Node::Command::yield:
		return walker.on_yield(cursor);
	case Node::Command::exit_generator:
		return walker.on_exit_generator(cursor);
	case Node::Command::yield_exit_generator:
		return walker.on_yield_exit_generator(cursor);
	case Node::Command::init_capture:
		return walker.on_init_capture(cursor);
	case Node::Command::capture_symbol:
		return walker.on_capture_symbol(cursor, cursor.next().as_symbol());
	case Node::Command::capture_as:
		return walker.on_capture_as(cursor, cursor.next().as_symbol());
	case Node::Command::capture_all:
		return walker.on_capture_all(cursor);
	case Node::Command::call:
		return walker.on_call(cursor, cursor.next().as_parameter());
	case Node::Command::call_member:
		return walker.on_call_member(cursor, cursor.next().as_parameter());
	case Node::Command::call_builtin:
		return walker.on_call_builtin(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::init_call:
		return walker.on_init_call(cursor);
	case Node::Command::init_member_call:
		return walker.on_init_member_call(cursor, cursor.next().as_symbol());
	case Node::Command::init_operator_call:
		return walker.on_init_operator_call(cursor, static_cast<Class::Operator>(cursor.next().as_parameter()));
	case Node::Command::init_var_member_call:
		return walker.on_init_var_member_call(cursor);
	case Node::Command::init_defined_member_call:
		{
			const auto& symbol = cursor.next().as_symbol();
			const auto offset = static_cast<std::size_t>(cursor.next().as_parameter());
			return walker.on_init_defined_member_call(cursor, symbol, offset);
		}
	case Node::Command::init_defined_operator_call:
		{
			const auto op = static_cast<Class::Operator>(cursor.next().as_parameter());
			const auto offset = static_cast<std::size_t>(cursor.next().as_parameter());
			return walker.on_init_defined_operator_call(cursor, op, offset);
		}
	case Node::Command::init_defined_var_member_call:
		return walker.on_init_defined_var_member_call(cursor, static_cast<std::size_t>(cursor.next().as_parameter()));
	case Node::Command::init_exception:
		return walker.on_init_exception(cursor, cursor.next().as_symbol());
	case Node::Command::reset_exception:
		return walker.on_reset_exception(cursor, cursor.next().as_symbol());
	case Node::Command::init_parameter:
		{
			const auto& symbol = cursor.next().as_symbol();
			const auto flags = static_cast<Reference::Flags>(cursor.next().as_parameter());
			const auto index = static_cast<std::size_t>(cursor.next().as_parameter());
			return walker.on_init_parameter(cursor, symbol, flags, index);
		}
	case Node::Command::exit_call:
		return walker.on_exit_call(cursor);
	case Node::Command::exit_thread:
		return walker.on_exit_thread(cursor);
	case Node::Command::exit_exec:
		return walker.on_exit_exec(cursor);
	case Node::Command::exit_module:
		return walker.on_exit_module(cursor);
	}
	if constexpr (!std::is_same_v<void, R>) {
		return {};
	}
}

}

#endif // MINT_AST_ABSTRACTSYNTAXTREEWALKER_H
