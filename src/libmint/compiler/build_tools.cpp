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

#include "mint/compiler/build_tools.h"
#include "mint/compiler/abstract_syntax_tree.h"
#include "mint/program/program.h"
#include "mint/program/class_register.h"
#include "mint/program/node.h"
#include "mint/program/symbol.h"
#include "mint/compiler/compiler.h"
#include "mint/program/module.h"
#include "mint/memory/data.h"
#include "mint/memory/global_data.h"
#include "mint/memory/object.h"
#include "mint/memory/class.h"
#include "mint/memory/reference.h"
#include "mint/system/data_stream.h"
#include "catch_context.h"
#include "case_table.h"
#include "context.h"
#include "branch.h"
#include "block.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace mint;

BytecodeBuildContext::BytecodeBuildContext(DataStream& stream, Compiler& compiler, ModuleInfo& data) :
    BuildContext(stream),
    _data(data),
    _compiler(compiler),
    _module_context(std::make_unique<Context>()),
    _main_branch(std::make_unique<MainBranch>(compiler.program(), data)),
    _branch(*_main_branch) {
	stream.set_new_line_callback([this](std::size_t line_number) {
		_branch.get().set_pending_new_line(line_number);
	});
}

BytecodeBuildContext::~BytecodeBuildContext() {
	assert(_operators.empty() || std::uncaught_exceptions() > 0);
	assert(_modifiers.empty() || std::uncaught_exceptions() > 0);
	assert(_branches.empty() || std::uncaught_exceptions() > 0);
	_branch.get().build();
}

void BytecodeBuildContext::commit_line_impl() {
	_branch.get().commit_line();
}

void BytecodeBuildContext::commit_expr_result() {
	Context& context = current_context();
	if (context.meta_blocks.empty()) {
		push_node(Node::Command::unload_reference);
	}
	else {
		switch (context.meta_blocks.top()) {
		case Context::MetaBlock::printer:
			push_node(Node::Command::print);
			break;
		case Context::MetaBlock::generator_expression:
			push_node(Node::Command::unload_reference);
			break;
		}
	}
}

void BytecodeBuildContext::reduce_module_stmt_list() {
	push_node(Node::Command::exit_module);
}

void BytecodeBuildContext::reduce_load_statement(const std::string& module_path) {
	push_node(Node::Command::load_module);
	push_node(module_path.c_str());
}

void BytecodeBuildContext::reduce_try_bloc() {
	reset_scoped_symbols();
	unregister_retrieve_point();
	push_node(Node::Command::unset_retrieve_point);
	push_node(Node::Command::jump);
	start_jump_forward();
	shift_jump_forward();
	resolve_jump_forward();
	push_node(Node::Command::reset_uncaught_exception);
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_catch_bloc() {
	reset_scoped_symbols();
	reset_exception();
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_if_bloc() {
	reset_scoped_symbols();
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_elif_bloc() {
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_else_bloc() {
	reset_scoped_symbols();
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_switch_bloc() {
	reset_scoped_symbols();
	push_node(Node::Command::jump);
	start_jump_forward();
	build_case_table();
	resolve_jump_forward();
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_while_bloc() {
	reset_scoped_symbols();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_for_bloc() {
	reset_scoped_symbols();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	close_block();
}

void BytecodeBuildContext::reduce_break_statement() {
	if (!is_in_loop() && !is_in_switch()) {
		parse_error("break statement not within loop or switch");
	}
	prepare_break();
	push_node(Node::Command::jump);
	bloc_jump_forward();
}

void BytecodeBuildContext::reduce_continue_statement() {
	if (!is_in_loop()) {
		parse_error("continue statement not within loop");
	}
	prepare_continue();
	push_node(Node::Command::jump);
	bloc_jump_backward();
}

void BytecodeBuildContext::reduce_print_to_stream_statement() {
	commit_expr_result();
	close_printer();
}

void BytecodeBuildContext::reduce_print_statement() {
	push_node(Node::Command::load_constant);
	push_node(Compiler::make_number(1.));
	open_printer();
	commit_expr_result();
	close_printer();
}

void BytecodeBuildContext::reduce_print_bloc() {
	reset_scoped_symbols();
	close_block();
	close_printer();
}

void BytecodeBuildContext::reduce_yield_generator_expression_statement() {
	set_generator();
	push_node(Node::Command::jump);
	start_jump_forward();
	start_jump_backward();
	push_node(Node::Command::range_next);
	resolve_jump_forward();
	push_node(Node::Command::range_expression_check);
	start_jump_forward();
	push_node(Node::Command::yield);
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_yield_statement() {
	if (is_in_generator_expression()) {
		push_node(Node::Command::yield);
	}
	else if (is_in_function()) {
		set_generator();
		push_node(Node::Command::yield);
	}
	else {
		parse_error("unexpected 'yield' statement outside of function");
	}
}

void BytecodeBuildContext::reduce_return_generator_expression_statement() {
	set_exit_point();
	push_node(Node::Command::unpack_generator_expression);
	if (is_in_generator()) {
		if (is_in_async_function()) {
			push_node(Node::Command::yield_exit_async_generator);
		}
		else {
			push_node(Node::Command::yield_exit_generator);
		}
	}
	else {
		if (is_in_async_function()) {
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::exit_call);
		}
	}
}

void BytecodeBuildContext::reduce_return_statement() {
	set_exit_point();
	if (is_in_generator()) {
		if (is_in_async_function()) {
			push_node(Node::Command::yield_exit_async_generator);
		}
		else {
			push_node(Node::Command::yield_exit_generator);
		}
	}
	else {
		if (is_in_async_function()) {
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::exit_call);
		}
	}
}

void BytecodeBuildContext::reduce_raise_statement() {
	reset_scoped_symbols_until(BlockType::try_type);
	push_node(Node::Command::raise);
}

void BytecodeBuildContext::reduce_raise_in_statement() {
	if (is_in_catch()) {
		reset_scoped_symbols_until(BlockType::try_type);
		push_node(Node::Command::reraise_in);
	}
	else {
		parse_error("no active exception to reraise");
	}
}

void BytecodeBuildContext::reduce_reraise_statement() {
	if (is_in_catch()) {
		reset_scoped_symbols_until(BlockType::try_type);
		push_node(Node::Command::reraise);
	}
	else {
		parse_error("no active exception to reraise");
	}
}

void BytecodeBuildContext::reduce_exit_with_code_statement() {
	push_node(Node::Command::exit_exec);
}

void BytecodeBuildContext::reduce_exit_statement() {
	push_node(Node::Command::load_constant);
	push_node(Compiler::make_number(0.));
	push_node(Node::Command::exit_exec);
}

void BytecodeBuildContext::reduce_identifier_iterator_assignment_expression() {
	push_node(Node::Command::copy_operator);
	commit_expr_result();
}

void BytecodeBuildContext::reduce_identifier_iterator_assignment_generator_expression() {
	push_node(Node::Command::copy_operator);
	commit_expr_result();
}

void BytecodeBuildContext::reduce_create_identifier_iterator_assignment_expression() {
	push_node(Node::Command::copy_operator);
	commit_expr_result();
}

void BytecodeBuildContext::reduce_create_identifier_iterator_assignment_generator_expression() {
	push_node(Node::Command::copy_operator);
	commit_expr_result();
}

void BytecodeBuildContext::reduce_expression_statement() {
	commit_expr_result();
}

void BytecodeBuildContext::reduce_function_definition_with_modifiers(const std::string& name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	const auto flags = Reference::const_address | retrieve_modifiers();
	resolve_jump_forward();
	push_node(Node::Command::declare_function);
	push_node(name.c_str());
	push_node(flags);
	save_definition(name);
	push_node(Node::Command::function_overload);
	push_node(Node::Command::unload_reference);
}

void BytecodeBuildContext::reduce_function_definition(const std::string& name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	const auto flags = Reference::const_address;
	resolve_jump_forward();
	push_node(Node::Command::declare_function);
	push_node(name.c_str());
	push_node(flags);
	save_definition(name);
	push_node(Node::Command::function_overload);
	push_node(Node::Command::unload_reference);
}

void BytecodeBuildContext::reduce_empty_statement() {}

void BytecodeBuildContext::reduce_package_declaration(const std::string& package_name) {
	open_package(package_name);
}

void BytecodeBuildContext::reduce_package_block() {
	close_package();
}

void BytecodeBuildContext::reduce_class_declaration_with_modifiers(const std::string& class_name) {
	start_class_description(class_name, Reference::const_address | Reference::const_value | retrieve_modifiers());
}

void BytecodeBuildContext::reduce_class_declaration(const std::string& class_name) {
	start_class_description(class_name, Reference::const_address | Reference::const_value);
}

void BytecodeBuildContext::reduce_parent_class() {
	save_base_class_path();
}

void BytecodeBuildContext::reduce_add_parent_class() {
	save_base_class_path();
}

void BytecodeBuildContext::reduce_parent_class_name(const std::string& parent_class_name) {
	append_symbol_to_base_class_path(parent_class_name);
}

void BytecodeBuildContext::reduce_qualified_parent_class_name(const std::string& parent_class_name) {
	append_symbol_to_base_class_path(parent_class_name);
}

void BytecodeBuildContext::reduce_class_definition() {
	resolve_class_description();
}

void BytecodeBuildContext::reduce_nested_class_declaration(const std::string& class_name) {
	start_class_description(class_name, Reference::global | Reference::const_address | Reference::const_value);
}

void BytecodeBuildContext::reduce_modified_nested_class_declaration(const std::string& class_name) {
	start_class_description(class_name,
	    Reference::global | Reference::const_address | Reference::const_value | retrieve_modifiers());
}

void BytecodeBuildContext::reduce_nested_class_definition() {
	resolve_class_description();
}

void BytecodeBuildContext::reduce_nested_enum_declaration(const std::string& enum_name) {
	start_enum_description(enum_name, Reference::global | Reference::const_address | Reference::const_value);
}

void BytecodeBuildContext::reduce_modified_nested_enum_declaration(const std::string& enum_name) {
	start_enum_description(enum_name,
	    Reference::global | Reference::const_address | Reference::const_value | retrieve_modifiers());
}

void BytecodeBuildContext::reduce_nested_enum_definition() {
	resolve_enum_description();
}

void BytecodeBuildContext::reduce_member_without_initializer(const std::string& member_name) {
	create_member(retrieve_modifiers(), Symbol(member_name), Compiler::make_none());
}

void BytecodeBuildContext::reduce_member_with_constant_initializer(const std::string& member_name,
    const std::string& initializer) {
	create_member(retrieve_modifiers(), Symbol(member_name),
	    compiler().make_data(initializer, Compiler::DataHint::data_unknown_hint));
}

void BytecodeBuildContext::reduce_member_with_string_initializer(const std::string& member_name,
    const std::string& initializer) {
	create_member(retrieve_modifiers(), Symbol(member_name),
	    compiler().make_data(initializer, Compiler::DataHint::data_string_hint));
}

void BytecodeBuildContext::reduce_member_with_regex_initializer(const std::string& member_name,
    const std::string& pattern) {
	create_member(retrieve_modifiers(), Symbol(member_name),
	    compiler().make_data(pattern, Compiler::DataHint::data_regex_hint));
}

void BytecodeBuildContext::reduce_member_with_flagged_regex_initializer(const std::string& member_name,
    const std::string& pattern, const std::string& flags) {
	create_member(retrieve_modifiers(), Symbol(member_name),
	    compiler().make_data(pattern + flags, Compiler::DataHint::data_regex_hint));
}

void BytecodeBuildContext::reduce_member_with_number_initializer(const std::string& member_name,
    const std::string& initializer) {
	create_member(retrieve_modifiers(), Symbol(member_name),
	    compiler().make_data(initializer, Compiler::DataHint::data_number_hint));
}

void BytecodeBuildContext::reduce_member_with_empty_array_initializer(const std::string& member_name) {
	create_member(retrieve_modifiers(), Symbol(member_name), compiler().make_array());
}

void BytecodeBuildContext::reduce_member_with_empty_hash_initializer(const std::string& member_name) {
	create_member(retrieve_modifiers(), Symbol(member_name), compiler().make_hash());
}

void BytecodeBuildContext::reduce_member_with_library_initializer(const std::string& member_name,
    const std::string& library_name) {
	create_member(retrieve_modifiers(), Symbol(member_name), compiler().make_library(library_name));
}

void BytecodeBuildContext::reduce_member_function_definition(const std::string& function_name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	create_member(retrieve_modifiers(), Symbol(function_name), retrieve_definition(function_name));
}

void BytecodeBuildContext::reduce_member_function_update(const std::string& function_name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	update_member(retrieve_modifiers(), Symbol(function_name), retrieve_definition(function_name));
}

void BytecodeBuildContext::reduce_named_function_definition(const std::string& function_name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	update_member(Reference::default_flags, Symbol(function_name), retrieve_definition(function_name));
}

void BytecodeBuildContext::reduce_async_function_definition(const std::string& function_name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	update_member(Reference::default_flags, Symbol(function_name), retrieve_definition(function_name));
}

void BytecodeBuildContext::reduce_operator_function_definition() {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	const auto symbol = retrieve_operator_symbol();
	update_member(Reference::default_flags, symbol, retrieve_definition(symbol.str()));
}

void BytecodeBuildContext::reduce_modified_named_function_definition(const std::string& function_name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	update_member(retrieve_modifiers(), Symbol(function_name), retrieve_definition(function_name));
}

void BytecodeBuildContext::reduce_modified_async_function_definition(const std::string& function_name) {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	update_member(retrieve_modifiers(), Symbol(function_name), retrieve_definition(function_name));
}

void BytecodeBuildContext::reduce_modified_operator_function_definition() {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();

	const auto symbol = retrieve_operator_symbol();
	update_member(retrieve_modifiers(), symbol, retrieve_definition(symbol.str()));
}

void BytecodeBuildContext::reduce_empty_descriptor_line() {}

void BytecodeBuildContext::reduce_in_operator() {
	start_operator(Class::in_operator);
}

void BytecodeBuildContext::reduce_copy_operator() {
	start_operator(Class::copy_operator);
}

void BytecodeBuildContext::reduce_or_operator() {
	start_operator(Class::or_operator);
}

void BytecodeBuildContext::reduce_and_operator() {
	start_operator(Class::and_operator);
}

void BytecodeBuildContext::reduce_bor_operator() {
	start_operator(Class::bor_operator);
}

void BytecodeBuildContext::reduce_xor_operator() {
	start_operator(Class::xor_operator);
}

void BytecodeBuildContext::reduce_band_operator() {
	start_operator(Class::band_operator);
}

void BytecodeBuildContext::reduce_eq_operator() {
	start_operator(Class::eq_operator);
}

void BytecodeBuildContext::reduce_ne_operator() {
	start_operator(Class::ne_operator);
}

void BytecodeBuildContext::reduce_lt_operator() {
	start_operator(Class::lt_operator);
}

void BytecodeBuildContext::reduce_gt_operator() {
	start_operator(Class::gt_operator);
}

void BytecodeBuildContext::reduce_le_operator() {
	start_operator(Class::le_operator);
}

void BytecodeBuildContext::reduce_ge_operator() {
	start_operator(Class::ge_operator);
}

void BytecodeBuildContext::reduce_shift_left_operator() {
	start_operator(Class::shift_left_operator);
}

void BytecodeBuildContext::reduce_shift_right_operator() {
	start_operator(Class::shift_right_operator);
}

void BytecodeBuildContext::reduce_add_operator() {
	start_operator(Class::add_operator);
}

void BytecodeBuildContext::reduce_sub_operator() {
	start_operator(Class::sub_operator);
}

void BytecodeBuildContext::reduce_mul_operator() {
	start_operator(Class::mul_operator);
}

void BytecodeBuildContext::reduce_div_operator() {
	start_operator(Class::div_operator);
}

void BytecodeBuildContext::reduce_mod_operator() {
	start_operator(Class::mod_operator);
}

void BytecodeBuildContext::reduce_not_operator() {
	start_operator(Class::not_operator);
}

void BytecodeBuildContext::reduce_compl_operator() {
	start_operator(Class::compl_operator);
}

void BytecodeBuildContext::reduce_inc_operator() {
	start_operator(Class::inc_operator);
}

void BytecodeBuildContext::reduce_dec_operator() {
	start_operator(Class::dec_operator);
}

void BytecodeBuildContext::reduce_pow_operator() {
	start_operator(Class::pow_operator);
}

void BytecodeBuildContext::reduce_inclusive_range_operator() {
	start_operator(Class::inclusive_range_operator);
}

void BytecodeBuildContext::reduce_exclusive_range_operator() {
	start_operator(Class::exclusive_range_operator);
}

void BytecodeBuildContext::reduce_call_operator() {
	start_operator(Class::call_operator);
}

void BytecodeBuildContext::reduce_subscript_operator() {
	start_operator(Class::subscript_operator);
}

void BytecodeBuildContext::reduce_subscript_move_operator() {
	start_operator(Class::subscript_move_operator);
}

void BytecodeBuildContext::reduce_modified_enum_declaration(const std::string& enum_name) {
	start_enum_description(enum_name, Reference::const_address | Reference::const_value | retrieve_modifiers());
}

void BytecodeBuildContext::reduce_enum_declaration(const std::string& enum_name) {
	start_enum_description(enum_name, Reference::const_address | Reference::const_value);
}

void BytecodeBuildContext::reduce_enum_definition() {
	resolve_enum_description();
}

void BytecodeBuildContext::reduce_enum_item_with_value(const std::string& item_name, const std::string& value) {
	constexpr auto flags = Reference::const_value | Reference::const_address | Reference::global;
	create_member(flags, Symbol(item_name), compiler().make_data(value, Compiler::DataHint::data_number_hint));
	set_current_enum_value(atoi(value.c_str()));
}

void BytecodeBuildContext::reduce_enum_item_with_implicit_value(const std::string& item_name) {
	constexpr auto flags = Reference::const_value | Reference::const_address | Reference::global;
	create_member(flags, Symbol(item_name),
	    compiler().make_data(std::to_string(next_enum_value()), Compiler::DataHint::data_number_hint));
}

void BytecodeBuildContext::reduce_empty_enum_item() {}

void BytecodeBuildContext::reduce_conditional_generator_expression() {
	reset_scoped_symbols();
	resolve_jump_forward();
	close_block();
	close_generator_expression();
}

void BytecodeBuildContext::reduce_if_else_generator_expression() {
	reset_scoped_symbols();
	resolve_jump_forward();
	close_block();
	close_generator_expression();
}

void BytecodeBuildContext::reduce_if_elif_generator_expression() {
	resolve_jump_forward();
	close_block();
	close_generator_expression();
}

void BytecodeBuildContext::reduce_if_elif_else_generator_expression() {
	reset_scoped_symbols();
	resolve_jump_forward();
	close_block();
	close_generator_expression();
}

void BytecodeBuildContext::reduce_switch_generator_expression() {
	reset_scoped_symbols();
	push_node(Node::Command::jump);
	start_jump_forward();
	build_case_table();
	resolve_jump_forward();
	resolve_jump_forward();
	close_block();
	close_generator_expression();
}

void BytecodeBuildContext::reduce_while_generator_expression() {
	reset_scoped_symbols();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	close_block();
	close_generator_expression();
}

void BytecodeBuildContext::reduce_for_generator_expression() {
	reset_scoped_symbols();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	close_block();
	close_generator_expression();
}

void BytecodeBuildContext::reduce_try_keyword() {
	register_retrieve_point();
	push_node(Node::Command::set_retrieve_point);
	start_jump_forward();
	open_block(BlockType::try_type);
}

void BytecodeBuildContext::reduce_catch_clause(const std::string& exception_name) {
	close_block();
	unregister_retrieve_point();
	push_node(Node::Command::unset_retrieve_point);
	push_node(Node::Command::jump);
	start_jump_forward();
	shift_jump_forward();
	resolve_jump_forward();
	open_block(BlockType::catch_type);
	push_node(Node::Command::init_exception);
	push_node(exception_name.c_str());
	set_exception_symbol(exception_name);
}

void BytecodeBuildContext::reduce_try_block() {
	reset_scoped_symbols();
}

void BytecodeBuildContext::reduce_if_block() {
	reset_scoped_symbols();
}

void BytecodeBuildContext::reduce_if_generator_block() {
	reset_scoped_symbols();
}

void BytecodeBuildContext::reduce_elif_block() {
	reset_scoped_symbols();
	shift_jump_forward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_add_elif_block() {
	reset_scoped_symbols();
	shift_jump_forward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_elif_generator_block() {
	reset_scoped_symbols();
	shift_jump_forward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_add_elif_generator_block() {
	reset_scoped_symbols();
	shift_jump_forward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_yield_generator_block() {
	set_generator();
	push_node(Node::Command::jump);
	start_jump_forward();
	start_jump_backward();
	push_node(Node::Command::range_next);
	resolve_jump_forward();
	push_node(Node::Command::range_expression_check);
	start_jump_forward();
	push_node(Node::Command::yield);
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_yield_value_block() {
	if (is_in_generator_expression()) {
		push_node(Node::Command::yield);
	}
	else if (is_in_function()) {
		set_generator();
		push_node(Node::Command::yield);
	}
	else {
		parse_error("unexpected 'yield' statement outside of function");
	}
}

void BytecodeBuildContext::reduce_return_generator_block() {
	set_exit_point();
	push_node(Node::Command::unpack_generator_expression);
	if (is_in_generator()) {
		if (is_in_async_function()) {
			push_node(Node::Command::yield_exit_async_generator);
		}
		else {
			push_node(Node::Command::yield_exit_generator);
		}
	}
	else {
		if (is_in_async_function()) {
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::exit_call);
		}
	}
}

void BytecodeBuildContext::reduce_return_value_block() {
	set_exit_point();
	if (is_in_generator()) {
		if (is_in_async_function()) {
			push_node(Node::Command::yield_exit_async_generator);
		}
		else {
			push_node(Node::Command::yield_exit_generator);
		}
	}
	else {
		if (is_in_async_function()) {
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::exit_call);
		}
	}
}

void BytecodeBuildContext::reduce_raise_block() {
	reset_scoped_symbols_until(BlockType::try_type);
	push_node(Node::Command::raise);
}

void BytecodeBuildContext::reduce_reraise_block() {
	if (is_in_catch()) {
		reset_scoped_symbols_until(BlockType::try_type);
		push_node(Node::Command::reraise_in);
	}
	else {
		parse_error("no active exception to reraise");
	}
}

void BytecodeBuildContext::reduce_bare_raise_block() {
	if (is_in_catch()) {
		reset_scoped_symbols_until(BlockType::try_type);
		push_node(Node::Command::reraise);
	}
	else {
		parse_error("no active exception to reraise");
	}
}

void BytecodeBuildContext::reduce_expression_block() {
	commit_expr_result();
}

void BytecodeBuildContext::reduce_generator_expression_body() {
	push_node(Node::Command::yield);
}

void BytecodeBuildContext::reduce_if_condition() {
	resolve_condition();
	push_node(Node::Command::zero_jump);
	start_jump_forward();
	open_block(BlockType::if_type);
}

void BytecodeBuildContext::reduce_if_generator_condition() {
	resolve_condition();
	push_node(Node::Command::zero_jump);
	start_jump_forward();
	open_block(BlockType::if_type);
}

void BytecodeBuildContext::reduce_elif_condition() {
	resolve_condition();
	push_node(Node::Command::zero_jump);
	start_jump_forward();
	open_block(BlockType::elif_type);
}

void BytecodeBuildContext::reduce_if_keyword() {
	start_condition();
}

void BytecodeBuildContext::reduce_generator_if_keyword() {
	open_generator_expression();
	start_condition();
}

void BytecodeBuildContext::reduce_elif_keyword() {
	close_block();
	push_node(Node::Command::jump);
	start_jump_forward();
	shift_jump_forward();
	resolve_jump_forward();
	start_condition();
}

void BytecodeBuildContext::reduce_else_keyword() {
	push_node(Node::Command::jump);
	start_jump_forward();

	close_block();
	shift_jump_forward();
	resolve_jump_forward();
	open_block(BlockType::else_type);
}

void BytecodeBuildContext::reduce_switch_condition() {
	resolve_condition();
	open_block(BlockType::switch_type);
}

void BytecodeBuildContext::reduce_generator_switch_condition() {
	resolve_condition();
	open_block(BlockType::switch_type);
}

void BytecodeBuildContext::reduce_generator_switch_keyword() {
	open_generator_expression();
	start_condition();
}

void BytecodeBuildContext::reduce_switch_keyword() {
	start_condition();
}

void BytecodeBuildContext::reduce_case_keyword() {
	start_case_label();
}

std::string BytecodeBuildContext::reduce_case_symbol(const std::string& symbol_name) {
	push_node(Node::Command::load_symbol);
	push_node(symbol_name.c_str());
	return symbol_name;
}

std::string BytecodeBuildContext::reduce_qualified_case_symbol(const std::string& parent_symbol,
    const std::string& separator, const std::string& symbol_name) {
	push_node(Node::Command::load_member);
	push_node(symbol_name.c_str());
	return parent_symbol + separator + symbol_name;
}

std::string BytecodeBuildContext::reduce_case_constant(const std::string& constant) {
	if (Data* data = compiler().make_data(constant, Compiler::DataHint::data_unknown_hint)) {
		push_node(Node::Command::load_constant);
		push_node(*data);
		return constant;
	}
	else {
		parse_error("token '" + constant + "' is not constant valid constant");
	}
}

std::string BytecodeBuildContext::reduce_positive_case_number(const std::string& number) {
	if (Data* data = compiler().make_data(number, Compiler::DataHint::data_number_hint)) {
		push_node(Node::Command::load_constant);
		push_node(*data);
		push_node(Node::Command::pos_operator);
		return number;
	}
	else {
		parse_error("token '" + number + "' is not number valid constant");
	}
}

std::string BytecodeBuildContext::reduce_negative_case_number(const std::string& sign, const std::string& number) {
	if (Data* data = compiler().make_data(number, Compiler::DataHint::data_number_hint)) {
		push_node(Node::Command::load_constant);
		push_node(*data);
		push_node(Node::Command::neg_operator);
		return sign + number;
	}
	else {
		parse_error("token '" + number + "' is not a valid constant");
	}
}

std::string BytecodeBuildContext::reduce_append_case_constant(const std::string& existing_constants,
    const std::string& constant, const std::string& separator) {
	add_to_call();
	return existing_constants + constant + separator;
}

std::string BytecodeBuildContext::reduce_start_case_constant_list(const std::string& constant,
    const std::string& separator) {
	push_node(Node::Command::alloc_iterator);
	start_call();
	add_to_call();
	return constant + separator;
}

std::string BytecodeBuildContext::reduce_finish_case_constant_list(const std::string& constant) {
	push_node(Node::Command::init_iterator);
	add_to_call();
	resolve_call();
	return constant;
}

void BytecodeBuildContext::reduce_empty_case_constant_list() {
	push_node(Node::Command::init_iterator);
	resolve_call();
}

void BytecodeBuildContext::reduce_inclusive_case_range_label(const std::string& start_value,
    const std::string& range_operator, const std::string& end_value) {
	push_node(Node::Command::inclusive_range_operator);
	start_jump_backward();
	push_node(Node::Command::find_next);
	push_node(Node::Command::find_check);
	start_jump_forward();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	resolve_case_label(start_value + range_operator + end_value);
}

void BytecodeBuildContext::reduce_exclusive_case_range_label(const std::string& start_value,
    const std::string& range_operator, const std::string& end_value) {
	push_node(Node::Command::exclusive_range_operator);
	start_jump_backward();
	push_node(Node::Command::find_next);
	push_node(Node::Command::find_check);
	start_jump_forward();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	resolve_case_label(start_value + range_operator + end_value);
}

void BytecodeBuildContext::reduce_case_constant_list_label(const std::string& constants,
    const std::string& last_constant) {
	start_jump_backward();
	push_node(Node::Command::find_next);
	push_node(Node::Command::find_check);
	start_jump_forward();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	resolve_case_label(constants + last_constant);
}

void BytecodeBuildContext::reduce_case_constant_membership_label(const std::string& constant) {
	push_node(Node::Command::find_operator);
	push_node(Node::Command::find_init);
	start_jump_backward();
	push_node(Node::Command::find_next);
	push_node(Node::Command::find_check);
	start_jump_forward();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	resolve_case_label(constant);
}

void BytecodeBuildContext::reduce_case_symbol_membership_label(const std::string& symbol_name) {
	push_node(Node::Command::find_operator);
	push_node(Node::Command::find_init);
	start_jump_backward();
	push_node(Node::Command::find_next);
	push_node(Node::Command::find_check);
	start_jump_forward();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	resolve_case_label(symbol_name);
}

void BytecodeBuildContext::reduce_case_constant_identity_label(const std::string& constant) {
	push_node(Node::Command::is_operator);
	resolve_case_label(constant);
}

void BytecodeBuildContext::reduce_case_symbol_identity_label(const std::string& symbol_name) {
	push_node(Node::Command::is_operator);
	resolve_case_label(symbol_name);
}

void BytecodeBuildContext::reduce_case_constant_label(const std::string& constant) {
	push_node(Node::Command::eq_operator);
	resolve_case_label(constant);
}

void BytecodeBuildContext::reduce_case_symbol_label(const std::string& symbol_name) {
	push_node(Node::Command::eq_operator);
	resolve_case_label(symbol_name);
}

void BytecodeBuildContext::reduce_default_case_keyword() {
	set_default_label();
}

void BytecodeBuildContext::reduce_empty_case_line() {}

void BytecodeBuildContext::reduce_case_expression_body() {
	if (is_in_generator_expression()) {
		push_node(Node::Command::yield);
	}
	else {
		commit_expr_result();
	}
	prepare_break();
	push_node(Node::Command::jump);
	bloc_jump_forward();
}

void BytecodeBuildContext::reduce_add_case_expression_body() {
	if (is_in_generator_expression()) {
		push_node(Node::Command::yield);
	}
	else {
		commit_expr_result();
	}
	prepare_break();
	push_node(Node::Command::jump);
	bloc_jump_forward();
}

void BytecodeBuildContext::reduce_default_expression_body() {
	if (is_in_generator_expression()) {
		push_node(Node::Command::yield);
	}
	else {
		commit_expr_result();
	}
	prepare_break();
	push_node(Node::Command::jump);
	bloc_jump_forward();
}

void BytecodeBuildContext::reduce_add_default_expression_body() {
	if (is_in_generator_expression()) {
		push_node(Node::Command::yield);
	}
	else {
		commit_expr_result();
	}
	prepare_break();
	push_node(Node::Command::jump);
	bloc_jump_forward();
}

void BytecodeBuildContext::reduce_while_condition() {
	resolve_condition();
	push_node(Node::Command::zero_jump);
	start_jump_forward();
	open_block(BlockType::conditional_loop_type);
}

void BytecodeBuildContext::reduce_generator_while_condition() {
	resolve_condition();
	push_node(Node::Command::zero_jump);
	start_jump_forward();
	open_block(BlockType::conditional_loop_type);
}

void BytecodeBuildContext::reduce_generator_while_keyword() {
	open_generator_expression();
	start_jump_backward();
	start_condition();
}

void BytecodeBuildContext::reduce_while_keyword() {
	start_jump_backward();
	start_condition();
}

void BytecodeBuildContext::reduce_range_for_condition() {
	resolve_condition();
	open_block(BlockType::custom_range_loop_type);
}

void BytecodeBuildContext::reduce_iterator_for_condition() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::range_init);
	resolve_condition();
	push_node(Node::Command::jump);
	start_jump_forward();
	start_jump_backward();
	push_node(Node::Command::range_next);
	resolve_jump_forward();
	push_node(Node::Command::range_iterator_check);
	start_jump_forward();
	open_block(BlockType::range_loop_type);
}

void BytecodeBuildContext::reduce_for_in_condition() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::range_init);
	resolve_condition();
	push_node(Node::Command::jump);
	start_jump_forward();
	start_jump_backward();
	push_node(Node::Command::range_next);
	resolve_jump_forward();
	push_node(Node::Command::range_check);
	start_jump_forward();
	open_block(BlockType::range_loop_type);
}

void BytecodeBuildContext::reduce_generator_range_for_condition() {
	resolve_condition();
	open_block(BlockType::custom_range_loop_type);
}

void BytecodeBuildContext::reduce_generator_iterator_for_condition() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::range_init);
	resolve_condition();
	push_node(Node::Command::jump);
	start_jump_forward();
	start_jump_backward();
	push_node(Node::Command::range_next);
	resolve_jump_forward();
	push_node(Node::Command::range_iterator_check);
	start_jump_forward();
	open_block(BlockType::range_loop_type);
}

void BytecodeBuildContext::reduce_generator_for_in_condition() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::range_init);
	resolve_condition();
	push_node(Node::Command::jump);
	start_jump_forward();
	start_jump_backward();
	push_node(Node::Command::range_next);
	resolve_jump_forward();
	push_node(Node::Command::range_check);
	start_jump_forward();
	open_block(BlockType::range_loop_type);
}

void BytecodeBuildContext::reduce_generator_for_keyword() {
	open_generator_expression();
	start_range_loop();
}

void BytecodeBuildContext::reduce_for_keyword() {
	start_range_loop();
}

void BytecodeBuildContext::reduce_generator_for_identifier() {
	resolve_range_loop();
	start_condition();
}

void BytecodeBuildContext::reduce_for_identifier() {
	resolve_range_loop();
	start_condition();
}

void BytecodeBuildContext::reduce_generator_for_iterator_identifier() {
	resolve_range_loop();
	start_condition();
}

void BytecodeBuildContext::reduce_generator_for_created_iterator() {
	resolve_range_loop();
	start_condition();
}

void BytecodeBuildContext::reduce_for_iterator_identifier() {
	resolve_range_loop();
	start_condition();
}

void BytecodeBuildContext::reduce_for_created_iterator() {
	resolve_range_loop();
	start_condition();
}

void BytecodeBuildContext::reduce_range_initial_value() {
	push_node(Node::Command::unload_reference);
	push_node(Node::Command::jump);
	start_jump_forward();
	start_jump_backward();
	resolve_range_loop();
	start_condition();
	open_sub_branch();
}

void BytecodeBuildContext::reduce_range_condition_value() {
	push_node(Node::Command::zero_jump);
	start_jump_forward();
	close_sub_branch();
}

void BytecodeBuildContext::reduce_range_step_value() {
	push_node(Node::Command::unload_reference);
	resolve_jump_forward();
	build_sub_branch();
}

void BytecodeBuildContext::reduce_return_keyword() {
	if (!is_in_function()) {
		parse_error("unexpected 'return' statement outside of function");
	}
	prepare_return();
}

void BytecodeBuildContext::reduce_hash_literal_start() {
	push_node(Node::Command::alloc_hash);
	start_call();
}

void BytecodeBuildContext::reduce_hash_literal_end() {
	push_node(Node::Command::init_hash);
	resolve_call();
}

void BytecodeBuildContext::reduce_add_hash_entry() {
	add_to_call();
}

void BytecodeBuildContext::reduce_hash_entry() {
	add_to_call();
}

void BytecodeBuildContext::reduce_array_literal_start() {
	push_node(Node::Command::alloc_array);
	start_call();
}

void BytecodeBuildContext::reduce_array_literal_end() {
	push_node(Node::Command::init_array);
	resolve_call();
}

void BytecodeBuildContext::reduce_array_value() {
	add_to_call();
}

void BytecodeBuildContext::reduce_array_spread() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_array_unpack() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_array_generator() {
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_add_iterator_value() {
	add_to_call();
}

void BytecodeBuildContext::reduce_iterator_value() {
	push_node(Node::Command::alloc_iterator);
	start_call();
	add_to_call();
}

void BytecodeBuildContext::reduce_add_iterator_spread() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_add_iterator_unpack() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_iterator_spread() {
	push_node(Node::Command::alloc_iterator);
	start_call();
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_iterator_unpack() {
	push_node(Node::Command::alloc_iterator);
	start_call();
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_iterator_end_expression() {
	push_node(Node::Command::init_iterator);
	add_to_call();
	resolve_call();
}

void BytecodeBuildContext::reduce_empty_iterator_end() {
	push_node(Node::Command::init_iterator);
	resolve_call();
}

void BytecodeBuildContext::reduce_add_identifier_iterator_target() {
	add_to_call();
}

void BytecodeBuildContext::reduce_identifier_iterator_target() {
	push_node(Node::Command::alloc_iterator);
	start_call();
	add_to_call();
}

void BytecodeBuildContext::reduce_identifier_iterator_end() {
	push_node(Node::Command::init_iterator);
	add_to_call();
	resolve_call();
}

void BytecodeBuildContext::reduce_empty_identifier_iterator_end() {
	push_node(Node::Command::init_iterator);
	resolve_call();
}

void BytecodeBuildContext::reduce_let_modifier() {
	start_modifiers(Reference::default_flags);
}

void BytecodeBuildContext::reduce_add_scoped_iterator_name(const std::string& iterator_name) {
	const auto index = create_fast_scoped_symbol_index(iterator_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(iterator_name.c_str());
		push_node(index);
		push_node(get_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(iterator_name.c_str());
		push_node(get_modifiers());
	}
	add_to_call();
}

void BytecodeBuildContext::reduce_first_scoped_iterator_name(const std::string& iterator_name) {
	push_node(Node::Command::alloc_iterator);
	start_call();
	const auto index = create_fast_scoped_symbol_index(iterator_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(iterator_name.c_str());
		push_node(index);
		push_node(get_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(iterator_name.c_str());
		push_node(get_modifiers());
	}
	add_to_call();
}

void BytecodeBuildContext::reduce_scoped_iterator_end_name(const std::string& iterator_name) {
	const auto index = create_fast_scoped_symbol_index(iterator_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(iterator_name.c_str());
		push_node(index);
		push_node(retrieve_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(iterator_name.c_str());
		push_node(retrieve_modifiers());
	}
	push_node(Node::Command::init_iterator);
	add_to_call();
	resolve_call();
}

void BytecodeBuildContext::reduce_add_iterator_name(const std::string& iterator_name) {
	const auto index = create_fast_symbol_index(iterator_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(iterator_name.c_str());
		push_node(index);
		push_node(get_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(iterator_name.c_str());
		push_node(get_modifiers());
	}
	add_to_call();
}

void BytecodeBuildContext::reduce_first_iterator_name(const std::string& iterator_name) {
	push_node(Node::Command::alloc_iterator);
	start_call();
	const auto index = create_fast_symbol_index(iterator_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(iterator_name.c_str());
		push_node(index);
		push_node(get_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(iterator_name.c_str());
		push_node(get_modifiers());
	}
	add_to_call();
}

void BytecodeBuildContext::reduce_iterator_end_name(const std::string& iterator_name) {
	const auto index = create_fast_symbol_index(iterator_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(iterator_name.c_str());
		push_node(index);
		push_node(retrieve_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(iterator_name.c_str());
		push_node(retrieve_modifiers());
	}
	push_node(Node::Command::init_iterator);
	add_to_call();
	resolve_call();
}

void BytecodeBuildContext::reduce_print_argument_separator() {
	open_printer();
}

void BytecodeBuildContext::reduce_print_block_target() {
	open_printer();
	open_block(BlockType::print_type);
}

void BytecodeBuildContext::reduce_print_block_without_target() {
	push_node(Node::Command::load_constant);
	push_node(Compiler::make_number(1.));
	open_printer();
	open_block(BlockType::print_type);
}

void BytecodeBuildContext::reduce_assignment_from_generator_expression() {
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::reduce_assignment_expression() {
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::reduce_binding_from_generator_expression() {
	push_node(Node::Command::copy_operator);
}

void BytecodeBuildContext::reduce_binding_expression() {
	push_node(Node::Command::copy_operator);
}

void BytecodeBuildContext::begin_iterator_initialization_expression() {
	push_node(Node::Command::alloc_iterator);
	start_call();
	add_to_call();
	push_node(Node::Command::init_iterator);
	resolve_call();
}

void BytecodeBuildContext::reduce_iterator_initialization_expression() {
	push_node(Node::Command::copy_operator);
}

void BytecodeBuildContext::reduce_addition_expression() {
	push_node(Node::Command::add_operator);
}

void BytecodeBuildContext::reduce_subtraction_expression() {
	push_node(Node::Command::sub_operator);
}

void BytecodeBuildContext::reduce_multiplication_expression() {
	push_node(Node::Command::mul_operator);
}

void BytecodeBuildContext::reduce_division_expression() {
	push_node(Node::Command::div_operator);
}

void BytecodeBuildContext::reduce_modulo_expression() {
	push_node(Node::Command::mod_operator);
}

void BytecodeBuildContext::reduce_exponentiation_expression() {
	push_node(Node::Command::pow_operator);
}

void BytecodeBuildContext::reduce_identity_expression() {
	push_node(Node::Command::is_operator);
}

void BytecodeBuildContext::reduce_membership_expression() {
	push_node(Node::Command::find_operator);
	push_node(Node::Command::find_init);
	start_jump_backward();
	push_node(Node::Command::find_next);
	push_node(Node::Command::find_check);
	start_jump_forward();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_negated_membership_expression() {
	push_node(Node::Command::find_operator);
	push_node(Node::Command::find_init);
	start_jump_backward();
	push_node(Node::Command::find_next);
	push_node(Node::Command::find_check);
	start_jump_forward();
	push_node(Node::Command::jump);
	resolve_jump_backward();
	resolve_jump_forward();
	push_node(Node::Command::not_operator);
}

void BytecodeBuildContext::reduce_equality_expression() {
	push_node(Node::Command::eq_operator);
}

void BytecodeBuildContext::reduce_inequality_expression() {
	push_node(Node::Command::ne_operator);
}

void BytecodeBuildContext::reduce_less_than_expression() {
	push_node(Node::Command::lt_operator);
}

void BytecodeBuildContext::reduce_greater_than_expression() {
	push_node(Node::Command::gt_operator);
}

void BytecodeBuildContext::reduce_less_than_or_equal_expression() {
	push_node(Node::Command::le_operator);
}

void BytecodeBuildContext::reduce_greater_than_or_equal_expression() {
	push_node(Node::Command::ge_operator);
}

void BytecodeBuildContext::reduce_left_shift_expression() {
	push_node(Node::Command::shift_left_operator);
}

void BytecodeBuildContext::reduce_right_shift_expression() {
	push_node(Node::Command::shift_right_operator);
}

void BytecodeBuildContext::reduce_inclusive_range_expression() {
	push_node(Node::Command::inclusive_range_operator);
}

void BytecodeBuildContext::reduce_exclusive_range_expression() {
	push_node(Node::Command::exclusive_range_operator);
}

void BytecodeBuildContext::reduce_prefix_increment_expression() {
	push_node(Node::Command::inc_operator);
}

void BytecodeBuildContext::reduce_prefix_decrement_expression() {
	push_node(Node::Command::dec_operator);
}

void BytecodeBuildContext::reduce_postfix_increment_expression() {
	push_node(Node::Command::clone_reference);
	push_node(Node::Command::inc_operator);
	push_node(Node::Command::unload_reference);
}

void BytecodeBuildContext::reduce_postfix_decrement_expression() {
	push_node(Node::Command::clone_reference);
	push_node(Node::Command::dec_operator);
	push_node(Node::Command::unload_reference);
}

void BytecodeBuildContext::reduce_logical_not_expression() {
	push_node(Node::Command::not_operator);
}

void BytecodeBuildContext::begin_logical_or_expression() {
	push_node(Node::Command::or_pre_check);
	start_jump_forward();
}

void BytecodeBuildContext::reduce_logical_or_expression() {
	push_node(Node::Command::or_operator);
	resolve_jump_forward();
}

void BytecodeBuildContext::begin_logical_and_expression() {
	push_node(Node::Command::and_pre_check);
	start_jump_forward();
}

void BytecodeBuildContext::reduce_logical_and_expression() {
	push_node(Node::Command::and_operator);
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_bitwise_or_expression() {
	push_node(Node::Command::bor_operator);
}

void BytecodeBuildContext::reduce_bitwise_and_expression() {
	push_node(Node::Command::band_operator);
}

void BytecodeBuildContext::reduce_bitwise_xor_expression() {
	push_node(Node::Command::xor_operator);
}

void BytecodeBuildContext::reduce_bitwise_not_expression() {
	push_node(Node::Command::compl_operator);
}

void BytecodeBuildContext::reduce_unary_plus_expression() {
	push_node(Node::Command::pos_operator);
}

void BytecodeBuildContext::reduce_unary_minus_expression() {
	push_node(Node::Command::neg_operator);
}

void BytecodeBuildContext::reduce_await_expression() {
	if (is_in_async_function()) {
		push_node(Node::Command::await);
	}
	else {
		parse_error("unexpected 'await' statement outside of async function");
	}
}

void BytecodeBuildContext::reduce_typeof_expression() {
	push_node(Node::Command::typeof_operator);
}

void BytecodeBuildContext::reduce_membersof_expression() {
	push_node(Node::Command::membersof_operator);
}

void BytecodeBuildContext::reduce_defined_expression() {
	push_node(Node::Command::check_defined);
}

void BytecodeBuildContext::reduce_slice_assignment_expression() {
	push_node(Node::Command::subscript_move_operator);
}

void BytecodeBuildContext::begin_addition_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_addition_assignment_expression() {
	push_node(Node::Command::add_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_subtraction_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_subtraction_assignment_expression() {
	push_node(Node::Command::sub_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_multiplication_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_multiplication_assignment_expression() {
	push_node(Node::Command::mul_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_division_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_division_assignment_expression() {
	push_node(Node::Command::div_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_modulo_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_modulo_assignment_expression() {
	push_node(Node::Command::mod_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_left_shift_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_left_shift_assignment_expression() {
	push_node(Node::Command::shift_left_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_right_shift_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_right_shift_assignment_expression() {
	push_node(Node::Command::shift_right_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_bitwise_and_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_bitwise_and_assignment_expression() {
	push_node(Node::Command::band_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_bitwise_or_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_bitwise_or_assignment_expression() {
	push_node(Node::Command::bor_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::begin_bitwise_xor_assignment_expression() {
	push_node(Node::Command::reload_reference);
}

void BytecodeBuildContext::reduce_bitwise_xor_assignment_expression() {
	push_node(Node::Command::xor_operator);
	push_node(Node::Command::move_operator);
}

void BytecodeBuildContext::reduce_regex_match_expression() {
	push_node(Node::Command::regex_match);
}

void BytecodeBuildContext::reduce_regex_non_match_expression() {
	push_node(Node::Command::regex_unmatch);
}

void BytecodeBuildContext::reduce_strict_equality_expression() {
	push_node(Node::Command::strict_eq_operator);
}

void BytecodeBuildContext::reduce_strict_inequality_expression() {
	push_node(Node::Command::strict_ne_operator);
}

void BytecodeBuildContext::begin_conditional_expression() {
	push_node(Node::Command::zero_jump);
	start_jump_forward();
}

void BytecodeBuildContext::continue_conditional_expression() {
	push_node(Node::Command::jump);
	start_jump_forward();
	shift_jump_forward();
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_conditional_expression() {
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_empty_parenthesized_expression() {
	push_node(Node::Command::alloc_iterator);
	start_call();
	push_node(Node::Command::init_iterator);
	resolve_call();
}

void BytecodeBuildContext::reduce_subscript_expression() {
	push_node(Node::Command::subscript_operator);
}

void BytecodeBuildContext::reduce_defined_member_call_arguments() {
	resolve_jump_forward();
}

void BytecodeBuildContext::reduce_call_arguments_start() {
	push_node(Node::Command::init_call);
	start_call();
}

void BytecodeBuildContext::reduce_call_arguments_end() {
	push_node(Node::Command::call);
	resolve_call();
}

void BytecodeBuildContext::reduce_member_call_start(const std::string& member_name) {
	push_node(Node::Command::init_member_call);
	push_node(member_name.c_str());
	start_call();
}

void BytecodeBuildContext::reduce_operator_member_call_start() {
	push_node(Node::Command::init_operator_call);
	push_node(retrieve_operator());
	start_call();
}

void BytecodeBuildContext::reduce_variable_member_call_start() {
	push_node(Node::Command::init_var_member_call);
	start_call();
}

void BytecodeBuildContext::reduce_defined_member_call_start(const std::string& member_name) {
	push_node(Node::Command::init_defined_member_call);
	push_node(member_name.c_str());
	start_jump_forward();
	start_call();
}

void BytecodeBuildContext::reduce_defined_operator_member_call_start() {
	push_node(Node::Command::init_defined_operator_call);
	push_node(retrieve_operator());
	start_jump_forward();
	start_call();
}

void BytecodeBuildContext::reduce_defined_variable_member_call_start() {
	push_node(Node::Command::init_defined_var_member_call);
	start_jump_forward();
	start_call();
}

void BytecodeBuildContext::reduce_member_call_arguments_end() {
	push_node(Node::Command::call_member);
	resolve_call();
}

void BytecodeBuildContext::reduce_positional_call_argument() {
	add_to_call();
}

void BytecodeBuildContext::reduce_callable_call_argument() {
	add_to_call();
}

void BytecodeBuildContext::reduce_generator_call_argument() {
	add_to_call();
}

void BytecodeBuildContext::reduce_spread_call_argument() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_unpack_call_argument() {
	push_node(Node::Command::in_operator);
	push_node(Node::Command::load_extra_arguments);
}

void BytecodeBuildContext::reduce_function_definition_with_arguments() {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();
	save_definition("<unknown>");
}

void BytecodeBuildContext::reduce_function_definition_without_arguments() {
	if (is_in_generator()) {
		if (!is_in_async_function()) {
			push_node(Node::Command::exit_generator);
		}
		else if (!has_returned()) {
			push_node(Node::Command::exit_async_generator);
		}
	}
	else if (!has_returned()) {
		if (is_in_async_function()) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::resume_coroutine);
		}
		else {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_none());
			push_node(Node::Command::exit_call);
		}
	}
	resolve_jump_forward();
	save_definition("<unknown>");
}

void BytecodeBuildContext::reduce_arrow_function_definition() {
	set_exit_point();
	if (is_in_async_function()) {
		push_node(Node::Command::resume_coroutine);
	}
	else {
		push_node(Node::Command::exit_call);
	}
	resolve_jump_forward();
	save_definition("<unknown>");
}

void BytecodeBuildContext::reduce_function_definition_start() {
	push_node(Node::Command::jump);
	start_jump_forward();
	start_definition();
}

void BytecodeBuildContext::reduce_async_function_definition_start() {
	push_node(Node::Command::jump);
	start_jump_forward();
	start_async_definition();
}

void BytecodeBuildContext::reduce_capture_list_start() {
	start_capture();
}

void BytecodeBuildContext::reduce_capture_list_end() {
	resolve_capture();
}

void BytecodeBuildContext::reduce_capture_with_initializer(const std::string& capture_name) {
	capture_as(capture_name);
}

void BytecodeBuildContext::reduce_final_capture_with_initializer(const std::string& capture_name) {
	capture_as(capture_name);
}

void BytecodeBuildContext::reduce_capture(const std::string& capture_name) {
	capture(capture_name);
}

void BytecodeBuildContext::reduce_final_capture(const std::string& capture_name) {
	capture(capture_name);
}

void BytecodeBuildContext::reduce_capture_all() {
	capture_all();
}

void BytecodeBuildContext::reduce_no_function_arguments() {
	save_parameters();
}

void BytecodeBuildContext::reduce_function_arguments_end() {
	save_parameters();
}

void BytecodeBuildContext::reduce_function_argument(const std::string& argument_name) {
	add_parameter(argument_name);
}

void BytecodeBuildContext::reduce_function_argument_with_default(const std::string& argument_name) {
	add_definition_signature();
	add_parameter(argument_name);
}

void BytecodeBuildContext::reduce_modified_function_argument(const std::string& argument_name) {
	add_parameter(argument_name, retrieve_modifiers());
}

void BytecodeBuildContext::reduce_modified_function_argument_with_default(const std::string& argument_name) {
	add_definition_signature();
	add_parameter(argument_name, retrieve_modifiers());
}

void BytecodeBuildContext::reduce_function_argument_unpack() {
	set_variadic();
}

void BytecodeBuildContext::reduce_arrow_function_body_start() {
	prepare_return();
}

void BytecodeBuildContext::reduce_member_access(const std::string& member_name) {
	push_node(Node::Command::load_member);
	push_node(member_name.c_str());
}

void BytecodeBuildContext::reduce_operator_member_access() {
	push_node(Node::Command::load_operator);
	push_node(retrieve_operator());
}

void BytecodeBuildContext::reduce_variable_member_access() {
	push_node(Node::Command::load_var_member);
}

void BytecodeBuildContext::reduce_optional_member_access(const std::string& member_name) {
	push_node(Node::Command::load_defined_member);
	push_node(member_name.c_str());
}

void BytecodeBuildContext::reduce_optional_operator_member_access() {
	push_node(Node::Command::load_defined_operator);
	push_node(retrieve_operator());
}

void BytecodeBuildContext::reduce_optional_variable_member_access() {
	push_node(Node::Command::load_defined_var_member);
}

void BytecodeBuildContext::reduce_defined_symbol(const std::string& symbol_name) {
	push_node(Node::Command::find_defined_symbol);
	push_node(symbol_name.c_str());
}

void BytecodeBuildContext::reduce_qualified_defined_symbol(const std::string& symbol_name) {
	push_node(Node::Command::find_defined_member);
	push_node(symbol_name.c_str());
}

void BytecodeBuildContext::reduce_defined_variable_symbol() {
	push_node(Node::Command::find_defined_var_symbol);
}

void BytecodeBuildContext::reduce_qualified_defined_variable_symbol() {
	push_node(Node::Command::find_defined_var_member);
}

void BytecodeBuildContext::reduce_defined_constant_symbol(const std::string& constant) {
	push_node(Node::Command::load_constant);
	if (Data* data = compiler().make_data(constant, Compiler::DataHint::data_unknown_hint)) {
		push_node(*data);
	}
	else {
		parse_error("token '" + constant + "' is not constant valid constant");
	}
}

void BytecodeBuildContext::reduce_constant_identifier(const std::string& constant) {
	push_node(Node::Command::load_constant);
	if (Data* data = compiler().make_data(constant, Compiler::DataHint::data_unknown_hint)) {
		push_node(*data);
	}
	else {
		parse_error("token '" + constant + "' is not constant valid constant");
	}
}

void BytecodeBuildContext::reduce_library_identifier() {
	push_node(Node::Command::create_lib);
}

void BytecodeBuildContext::reduce_variable_identifier() {
	push_node(Node::Command::load_var_symbol);
}

void BytecodeBuildContext::reduce_identifier(const std::string& identifier_name) {
	const auto index = fast_symbol_index(identifier_name);
	if (index != invalid_index) {
		push_node(Node::Command::load_fast);
		push_node(identifier_name.c_str());
		push_node(index);
	}
	else {
		push_node(Node::Command::load_symbol);
		push_node(identifier_name.c_str());
	}
}

void BytecodeBuildContext::reduce_let_identifier(const std::string& identifier_name) {
	const auto index = create_fast_scoped_symbol_index(identifier_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(identifier_name.c_str());
		push_node(index);
		push_node(Reference::default_flags);
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(identifier_name.c_str());
		push_node(Reference::default_flags);
	}
}

void BytecodeBuildContext::reduce_modified_identifier(const std::string& identifier_name) {
	const auto index = create_fast_symbol_index(identifier_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(identifier_name.c_str());
		push_node(index);
		push_node(retrieve_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(identifier_name.c_str());
		push_node(retrieve_modifiers());
	}
}

void BytecodeBuildContext::reduce_let_modified_identifier(const std::string& identifier_name) {
	const auto index = create_fast_scoped_symbol_index(identifier_name);
	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(identifier_name.c_str());
		push_node(index);
		push_node(retrieve_modifiers());
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(identifier_name.c_str());
		push_node(retrieve_modifiers());
	}
}

std::string BytecodeBuildContext::reduce_constant_value(const std::string& value) {
	return value;
}

std::string BytecodeBuildContext::reduce_regular_expression(const std::string& pattern) {
	return pattern;
}

std::string BytecodeBuildContext::reduce_regular_expression_with_symbol(const std::string& pattern,
    const std::string& flags) {
	return pattern + flags;
}

std::string BytecodeBuildContext::reduce_string_literal(const std::string& value) {
	return value;
}

std::string BytecodeBuildContext::reduce_number_literal(const std::string& value) {
	return value;
}

std::string BytecodeBuildContext::reduce_regular_expression_start(const std::string& opening_delimiter) {
	return opening_delimiter + read_regex();
}

std::string BytecodeBuildContext::reduce_regular_expression_end(const std::string& opening_delimiter,
    const std::string& closing_delimiter) {
	return closing_delimiter + opening_delimiter;
}

void BytecodeBuildContext::reduce_default_modifier() {
	start_modifiers(Reference::default_flags);
}

void BytecodeBuildContext::reduce_const_address_modifier() {
	start_modifiers(Reference::const_address);
}

void BytecodeBuildContext::reduce_const_value_modifier() {
	start_modifiers(Reference::const_value);
}

void BytecodeBuildContext::reduce_const_modifier() {
	start_modifiers(Reference::const_address | Reference::const_value);
}

void BytecodeBuildContext::reduce_global_modifier() {
	start_modifiers(Reference::global);
}

void BytecodeBuildContext::reduce_final_modifier() {
	start_modifiers(Reference::final_member);
}

void BytecodeBuildContext::reduce_override_modifier() {
	start_modifiers(Reference::override_member);
}

void BytecodeBuildContext::reduce_public_modifier() {
	start_modifiers(Reference::default_flags);
}

void BytecodeBuildContext::reduce_protected_modifier() {
	start_modifiers(Reference::protected_visibility);
}

void BytecodeBuildContext::reduce_private_modifier() {
	start_modifiers(Reference::private_visibility);
}

void BytecodeBuildContext::reduce_package_modifier() {
	start_modifiers(Reference::package_visibility);
}

void BytecodeBuildContext::reduce_add_default_modifier() {
	add_modifiers(Reference::default_flags);
}

void BytecodeBuildContext::reduce_add_const_address_modifier() {
	add_modifiers(Reference::const_address);
}

void BytecodeBuildContext::reduce_add_const_value_modifier() {
	add_modifiers(Reference::const_value);
}

void BytecodeBuildContext::reduce_add_const_modifier() {
	add_modifiers(Reference::const_address | Reference::const_value);
}

void BytecodeBuildContext::reduce_add_global_modifier() {
	add_modifiers(Reference::global);
}

void BytecodeBuildContext::reduce_add_final_modifier() {
	add_modifiers(Reference::final_member);
}

void BytecodeBuildContext::reduce_add_override_modifier() {
	add_modifiers(Reference::override_member);
}

void BytecodeBuildContext::reduce_add_public_modifier() {
	add_modifiers(Reference::default_flags);
}

void BytecodeBuildContext::reduce_add_protected_modifier() {
	add_modifiers(Reference::protected_visibility);
}

void BytecodeBuildContext::reduce_add_private_modifier() {
	add_modifiers(Reference::private_visibility);
}

void BytecodeBuildContext::reduce_add_package_modifier() {
	add_modifiers(Reference::package_visibility);
}

void BytecodeBuildContext::reduce_statement_separator() {}

void BytecodeBuildContext::reduce_empty_line() {}

void BytecodeBuildContext::reduce_add_empty_line() {}

std::size_t BytecodeBuildContext::create_fast_scoped_symbol_index(const std::string& symbol) {

	const Symbol* module_symbol = nullptr;
	Context& context = current_context();

	if (context.condition_scoped_symbols) {
		module_symbol = _data.get().bytecode.make_symbol(symbol);
		context.condition_scoped_symbols->emplace_back(module_symbol);
	}
	else if (context.range_loop_scoped_symbols) {
		module_symbol = _data.get().bytecode.make_symbol(symbol);
		context.range_loop_scoped_symbols->emplace_back(module_symbol);
	}
	else if (!context.blocks.empty()) {
		const auto& block = context.blocks.back();
		module_symbol = _data.get().bytecode.make_symbol(symbol);
		block->block_scoped_symbols.push_back(module_symbol);
	}

	if (Definition* def = current_definition(); def && def->with_fast) {
		if (module_symbol == nullptr) {
			module_symbol = _data.get().bytecode.make_symbol(symbol);
		}
		return mint::create_fast_symbol_index(*def, *module_symbol);
	}

	return invalid_index;
}

std::size_t BytecodeBuildContext::create_fast_symbol_index(const std::string& symbol) {

	const Symbol* module_symbol = nullptr;
	if (Definition* def = current_definition(); def && def->with_fast) {
		module_symbol = _data.get().bytecode.make_symbol(symbol);
		return mint::create_fast_symbol_index(*def, *module_symbol);
	}

	return invalid_index;
}

std::size_t BytecodeBuildContext::fast_symbol_index(const std::string& symbol) {

	if (Definition* def = current_definition(); def && def->with_fast) {
		const Symbol* module_symbol = _data.get().bytecode.make_symbol(symbol);
		return mint::fast_symbol_index(*def, *module_symbol);
	}

	return invalid_index;
}

bool BytecodeBuildContext::has_returned() const {
	if (const Definition* def = current_definition()) {
		return def->returned;
	}
	return false;
}

void BytecodeBuildContext::open_block(BlockType type) {

	Context& context = current_context();
	auto block = std::make_unique<Block>(type);

	switch (type) {
	case BlockType::conditional_loop_type:
	case BlockType::custom_range_loop_type:
	case BlockType::range_loop_type:
		block->backward = _branch.get().next_jump_backward();
		block->forward = _branch.get().next_jump_forward();
		break;

	case BlockType::switch_type:
		block->case_table = std::make_unique<CaseTable>();
		push_node(Node::Command::jump);
		block->case_table->origin = _branch.get().next_node_offset();
		push_node(0);
		block->forward = _branch.get().start_empty_jump_forward();
		break;

	case BlockType::catch_type:
		block->catch_context = std::make_unique<CatchContext>();
		break;

	default:
		break;
	}

	if (context.condition_scoped_symbols) {
		std::ranges::move(*context.condition_scoped_symbols, std::back_inserter(block->block_scoped_symbols));
		block->condition_scoped_symbols = std::move(context.condition_scoped_symbols);
	}

	if (context.range_loop_scoped_symbols) {
		block->range_loop_scoped_symbols = std::move(context.range_loop_scoped_symbols);
	}

	context.blocks.emplace_back(std::move(block));
}

void BytecodeBuildContext::reset_scoped_symbols() {
	Context& context = current_context();
	reset_scoped_symbols(context.blocks.back()->block_scoped_symbols);
}

void BytecodeBuildContext::reset_scoped_symbols_until(BlockType type) {
	Context& context = current_context();
	for (const auto& block : std::views::reverse(context.blocks)) {
		reset_scoped_symbols(block->block_scoped_symbols);
		if (block->range_loop_scoped_symbols) {
			reset_scoped_symbols(*block->range_loop_scoped_symbols);
		}
		if (block->type == type) {
			break;
		}
	}
}

void BytecodeBuildContext::close_block() {

	Context& context = current_context();
	const auto& block = context.blocks.back();

	if (block->condition_scoped_symbols) {
		reset_scoped_symbols(*block->condition_scoped_symbols);
	}

	if (block->range_loop_scoped_symbols) {
		reset_scoped_symbols(*block->range_loop_scoped_symbols);
	}

	context.blocks.pop_back();
}

bool BytecodeBuildContext::is_in_catch() const {
	return std::ranges::any_of(current_context().blocks, [](const auto& block) {
		return block->type == BlockType::catch_type;
	});
}

bool BytecodeBuildContext::is_in_loop() const {
	if (const Block* block = current_continuable_block()) {
		switch (block->type) {
		case BlockType::conditional_loop_type:
		case BlockType::custom_range_loop_type:
		case BlockType::range_loop_type:
			return true;
		default:
			break;
		}
	}
	return false;
}

bool BytecodeBuildContext::is_in_switch() const {
	if (const Block* block = current_breakable_block()) {
		return block->type == BlockType::switch_type;
	}
	return false;
}

bool BytecodeBuildContext::is_in_range_loop() const {
	if (const Block* block = current_continuable_block()) {
		return block->type == BlockType::range_loop_type;
	}
	return false;
}

bool BytecodeBuildContext::is_in_function() const {
	return !_definitions.empty();
}

bool BytecodeBuildContext::is_in_nested_function() const {
	return _definitions.size() >= 2;
}

bool BytecodeBuildContext::is_in_async_function() const {
	return !_definitions.empty() && _definitions.top()->async;
}

bool BytecodeBuildContext::is_in_generator() const {
	if (const Definition* def = current_definition()) {
		return def->generator;
	}
	return false;
}

bool BytecodeBuildContext::is_in_generator_expression() const {
	const auto& context = current_context();
	if (context.meta_blocks.empty()) {
		return false;
	}
	return context.meta_blocks.top() == Context::MetaBlock::generator_expression;
}

void BytecodeBuildContext::prepare_continue() {

	if (const auto* block = current_breakable_block()) {

		for (std::size_t i = 0; i < block->retrieve_point_count; ++i) {
			push_node(Node::Command::unset_retrieve_point);
		}

		const auto& context = current_context();
		const auto& children = context.blocks;

		for (auto child = children.rbegin(); child != children.rend() && child->get() != block; ++child) {
			reset_scoped_symbols((*child)->block_scoped_symbols);
		}

		reset_scoped_symbols(block->block_scoped_symbols);
	}
}

void BytecodeBuildContext::prepare_break() {

	if (const auto* block = current_breakable_block()) {

		if (block->type == BlockType::range_loop_type) {
			// unload range
			push_node(Node::Command::unload_reference);
			// unload target
			push_node(Node::Command::unload_reference);
		}

		for (std::size_t i = 0; i < block->retrieve_point_count; ++i) {
			push_node(Node::Command::unset_retrieve_point);
		}

		const auto& context = current_context();
		const auto& children = context.blocks;

		for (auto child = children.rbegin(); child != children.rend() && child->get() != block; ++child) {
			reset_scoped_symbols((*child)->block_scoped_symbols);
		}

		reset_scoped_symbols(block->block_scoped_symbols);
	}
}

void BytecodeBuildContext::prepare_return() {

	if (Definition* def = current_definition()) {

		for (const auto& block : def->blocks) {
			if (block->type == BlockType::range_loop_type) {
				// unload range
				push_node(Node::Command::unload_reference);
				// unload target
				push_node(Node::Command::unload_reference);
			}
		}

		for (std::size_t i = 0; i < def->retrieve_point_count; ++i) {
			push_node(Node::Command::unset_retrieve_point);
		}

		if (def->blocks.empty()) {
			def->returned = true;
		}
	}
}

void BytecodeBuildContext::register_retrieve_point() {

	if (Definition* definition = current_definition()) {
		definition->retrieve_point_count++;
	}
	if (Block* block = current_breakable_block()) {
		block->retrieve_point_count++;
	}
}

void BytecodeBuildContext::unregister_retrieve_point() {

	if (Definition* definition = current_definition()) {
		definition->retrieve_point_count--;
	}
	if (Block* block = current_breakable_block()) {
		block->retrieve_point_count--;
	}
}

void BytecodeBuildContext::set_exception_symbol(const std::string& symbol) {

	Context& context = current_context();
	const auto& block = context.blocks.back();

	if (CatchContext* catch_context = block->catch_context.get()) {
		catch_context->symbol = _data.get().bytecode.make_symbol(symbol);
	}
}

void BytecodeBuildContext::reset_exception() {

	Context& context = current_context();
	const auto& block = context.blocks.back();

	if (const auto* catch_context = block->catch_context.get()) {
		push_node(Node::Command::reset_exception);
		push_node(catch_context->symbol);
	}
}

void BytecodeBuildContext::start_case_label() {
	if (CaseTable* case_table = current_breakable_block()->case_table.get()) {
		case_table->current_label = CaseTable::Label(_branch.get());
		push_branch(*case_table->current_label->condition);
	}
}

void BytecodeBuildContext::resolve_case_label(const std::string& label) {
	if (CaseTable* case_table = current_breakable_block()->case_table.get()) {
		if (!case_table->labels.emplace(label, std::move(*case_table->current_label)).second) {
			parse_error("duplicate case value");
		}
		case_table->current_label = std::nullopt;
		pop_branch();
	}
}

void BytecodeBuildContext::set_default_label() {
	if (CaseTable* case_table = current_breakable_block()->case_table.get()) {
		if (case_table->default_label) {
			parse_error("multiple default labels in one switch");
		}
		case_table->default_label = _branch.get().next_node_offset();
	}
}

void BytecodeBuildContext::build_case_table() {

	if (CaseTable* case_table = current_breakable_block()->case_table.get()) {

		_branch.get().replace_node(case_table->origin, static_cast<int>(_branch.get().next_node_offset()));

		for (const auto& [_, label] : case_table->labels) {
			push_node(Node::Command::reload_reference);
			label.condition->build();
			push_node(Node::Command::case_jump);
			push_node(static_cast<int>(label.offset));
		}

		if (case_table->default_label) {
			push_node(Node::Command::load_constant);
			push_node(Compiler::make_boolean(true));
			push_node(Node::Command::case_jump);
			push_node(static_cast<int>(*case_table->default_label));
		}
		else {
			push_node(Node::Command::unload_reference);
		}
	}
}

void BytecodeBuildContext::start_jump_forward() {
	_branch.get().start_jump_forward();
}

void BytecodeBuildContext::bloc_jump_forward() {
	const auto* block = current_breakable_block();
	assert(block && block->forward);
	block->forward->push_back(_branch.get().next_node_offset());
	push_node(0);
}

void BytecodeBuildContext::shift_jump_forward() {
	_branch.get().shift_jump_forward();
}

void BytecodeBuildContext::resolve_jump_forward() {
	_branch.get().resolve_jump_forward();
}

void BytecodeBuildContext::start_jump_backward() {
	_branch.get().start_jump_backward();
}

void BytecodeBuildContext::bloc_jump_backward() {
	const auto* block = current_continuable_block();
	assert(block && block->backward);
	push_node(static_cast<int>(*block->backward));
}

void BytecodeBuildContext::shift_jump_backward() {
	_branch.get().shift_jump_backward();
}

void BytecodeBuildContext::resolve_jump_backward() {
	_branch.get().resolve_jump_backward();
}

void BytecodeBuildContext::start_definition() {
	_definitions.push(std::make_unique<Definition>(Definition {
	    .begin_offset = _branch.get().next_node_offset(),
	    .function = _data.get().bytecode.make_constant<Function>(),
	}));
}

void BytecodeBuildContext::start_async_definition() {
	_definitions.push(std::make_unique<Definition>(Definition {
	    .begin_offset = _branch.get().next_node_offset(),
	    .function = _data.get().bytecode.make_constant<Function>(),
	    .async = true,
	}));
}

void BytecodeBuildContext::add_parameter(const std::string& symbol, Reference::Flags flags) {

	Definition* def = current_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	const auto* s = _data.get().bytecode.make_symbol(symbol);
	const auto index = static_cast<int>(def->fast_symbol_count++);
	def->fast_symbol_indexes.emplace(*s, index);
	def->parameters.push({
	    .flags = flags,
	    .symbol = s,
	});
}

void BytecodeBuildContext::set_variadic() {

	Definition* def = current_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	const auto* s = _data.get().bytecode.make_symbol("va_args");
	const auto index = static_cast<int>(def->fast_symbol_count++);
	def->fast_symbol_indexes.emplace(*s, index);
	def->parameters.push({
	    .flags = Reference::default_flags,
	    .symbol = s,
	});
	def->variadic = true;

	if (!def->function->data<Function>().mapping.empty()) {
		push_node(Node::Command::init_iterator);
		push_node(0);
	}
}

void BytecodeBuildContext::set_generator() {

	Definition* def = current_definition();
	assert(def);

	for (const auto exit_point : def->exit_points) {
		_branch.get().replace_node(exit_point, Node::Command::yield_exit_generator);
	}

	def->generator = true;
}

void BytecodeBuildContext::set_exit_point() {
	current_definition()->exit_points.emplace_back(_branch.get().next_node_offset());
}

void BytecodeBuildContext::save_parameters() {

	auto* def = current_definition();
	assert(def);

	if (def->variadic && def->parameters.empty()) {
		parse_error("expected parameter before '...' token");
	}

	const auto count = static_cast<int>(def->parameters.size());
	const int signature = def->variadic ? ~(count - 1) : count;
	FunctionHandle& handle = _data.get().bytecode.make_handle(current_package(), def->begin_offset);

	if (def->capture) {
		def->function->data<Function>().mapping.emplace(signature, std::make_unique<Function::Stateful>(handle));
	}
	else {
		def->function->data<Function>().mapping.emplace(signature, std::make_unique<Function::Stateless>(handle));
	}

	while (!def->parameters.empty()) {
		const auto& param = def->parameters.top();
		push_node(Node::Command::init_parameter);
		push_node(param.symbol);
		push_node(param.flags);
		push_node(mint::fast_symbol_index(*def, *param.symbol));
		def->parameters.pop();
	}
}

void BytecodeBuildContext::add_definition_signature() {

	auto* def = current_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	const auto signature = static_cast<int>(def->parameters.size());
	FunctionHandle& handle = _data.get().bytecode.make_handle(current_package(), def->begin_offset);

	if (def->capture) {
		def->function->data<Function>().mapping.emplace(signature, std::make_unique<Function::Stateful>(handle));
	}
	else {
		def->function->data<Function>().mapping.emplace(signature, std::make_unique<Function::Stateless>(handle));
	}

	def->begin_offset = _branch.get().next_node_offset();
}

void BytecodeBuildContext::save_definition(std::string name) {

	auto* def = current_definition();
	assert(def);

	_data.get().debug_info.register_function({
	    .name = std::move(name),
	    .begin_offset = def->begin_offset,
	    .end_offset = next_offset(),
	});

	for (auto& signature : def->function->data<Function>().mapping) {
		signature.second.handle().fast_count = def->fast_symbol_count;
		signature.second.handle().generator = def->generator;
		signature.second.handle().async = def->async;
	}

	if (def->global_data) {
		_data.get().bytecode.add_internal_register(std::move(def->global_data));
	}

	push_node(Node::Command::load_constant);
	push_node(def->function);

	if (def->capture) {
		def->capture->build();
	}

	assert(def->blocks.empty());
	_definitions.pop();
}

Function& BytecodeBuildContext::retrieve_definition(std::string name) {

	assert(!_definitions.empty());

	const auto def = std::move(_definitions.top());
	_definitions.pop();

	if (const auto& classes = current_context().classes; !classes.empty()) {
		name = classes.top().first->full_name() + '.' + std::move(name);
	}
	_data.get().debug_info.register_function({
	    .name = std::move(name),
	    .begin_offset = def->begin_offset,
	    .end_offset = next_offset(),
	});

	auto& data = def->function->data<Function>();
	for (auto& signature : data.mapping) {
		signature.second.handle().fast_count = def->fast_symbol_count;
		signature.second.handle().generator = def->generator;
		signature.second.handle().async = def->async;
	}

	if (def->global_data) {
		_data.get().bytecode.add_internal_register(std::move(def->global_data));
	}

	assert(def->blocks.empty());
	return data;
}

PackageData& BytecodeBuildContext::current_package() const {
	if (_packages.empty()) {
		return _compiler.get().program().global_data();
	}
	return _packages.top().get();
}

void BytecodeBuildContext::open_package(const std::string& name) {
	PackageData& package = current_package().get_package(Symbol(name));
	push_node(Node::Command::open_package);
	push_node(Compiler::make_package(package));
	_packages.emplace(package);
}

void BytecodeBuildContext::close_package() {
	assert(!_packages.empty());
	push_node(Node::Command::close_package);
	_packages.pop();
}

void BytecodeBuildContext::start_class_description(const std::string& name, Reference::Flags flags) {
	_class_base.clear();
	current_context().classes.emplace(_data.get().bytecode.make_class(_compiler.get().program(), name), flags);
}

void BytecodeBuildContext::append_symbol_to_base_class_path(const std::string& symbol) {
	_class_base.append_symbol(Symbol(symbol));
}

void BytecodeBuildContext::save_base_class_path() {
	current_context().classes.top().first->add_base(_class_base);
	_class_base.clear();
}

void BytecodeBuildContext::create_member(Reference::Flags flags, const Symbol& symbol, Data* value) {
	if (value == nullptr) {
		parse_error(symbol.str() + ": member value is not a valid constant");
	}
	create_member(flags, symbol, *value);
}

void BytecodeBuildContext::create_member(Reference::Flags flags, const Symbol& symbol, Data& value) {
	if (!current_context().classes.top().first->create_member(symbol, Reference(flags, value))) {
		parse_error(symbol.str() + ": member was already defined");
	}
}

void BytecodeBuildContext::update_member(Reference::Flags flags, const Symbol& symbol, Data& value) {
	if (!current_context().classes.top().first->update_member(symbol, Reference(flags, value))) {
		parse_error(symbol.str() + ": member was already defined");
	}
}

void BytecodeBuildContext::resolve_class_description() {

	auto& context = current_context();
	auto [desc, flags] = context.classes.top();
	context.classes.pop();

	if (context.classes.empty()) {
		if (flags & Reference::global) {
			current_package().register_class_description(*desc, flags);
		}
		else if (auto* def = current_definition()) {
			if (!def->global_data) {
				def->global_data = std::make_unique<FunctionData>(_compiler.get().program());
			}
			def->global_data->register_class_description(*desc, flags);
		}
		else {
			_data.get().bytecode.register_class_description(*desc, flags);
		}
		push_node(Node::Command::declare_class);
		push_node(desc);
		push_node(flags);
	}
	else {
		assert(flags & Reference::global);
		context.classes.top().first->register_class_description(*desc, flags);
	}
}

void BytecodeBuildContext::start_enum_description(const std::string& name, Reference::Flags flags) {
	start_class_description(name, flags);
	_next_enum_value = 0;
}

void BytecodeBuildContext::set_current_enum_value(int value) {
	_next_enum_value = value + 1;
}

int BytecodeBuildContext::next_enum_value() {
	return _next_enum_value++;
}

void BytecodeBuildContext::resolve_enum_description() {
	resolve_class_description();
}

void BytecodeBuildContext::start_call() {
	_calls.push(std::make_unique<Call>());
}

void BytecodeBuildContext::add_to_call() {
	_calls.top()->argc++;
}

void BytecodeBuildContext::resolve_call() {
	push_node(_calls.top()->argc);
	_calls.pop();
}

void BytecodeBuildContext::start_capture() {
	Definition* def = current_definition();
	def->capture = std::make_unique<SubBranch>(_branch);
	def->with_fast = false;
	push_branch(*def->capture);
	push_node(Node::Command::init_capture);
}

void BytecodeBuildContext::resolve_capture() {
	Definition* def = current_definition();
	def->with_fast = true;
	pop_branch();
}

void BytecodeBuildContext::capture_as(const std::string& symbol) {

	const auto* def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	push_node(Node::Command::capture_as);
	push_node(symbol.c_str());
}

void BytecodeBuildContext::capture(const std::string& symbol) {

	const auto* def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	push_node(Node::Command::capture_symbol);
	push_node(symbol.c_str());
}

void BytecodeBuildContext::capture_all() {

	Definition* def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	push_node(Node::Command::capture_all);
	def->capture_all = true;
}

void BytecodeBuildContext::open_generator_expression() {
	if (!is_in_generator_expression()) {
		if (is_in_async_function()) {
			push_node(Node::Command::begin_async_generator_expression);
			start_jump_forward();
		}
		else {
			push_node(Node::Command::begin_generator_expression);
			start_jump_forward();
		}
	}
	current_context().meta_blocks.push(Context::MetaBlock::generator_expression);
}

void BytecodeBuildContext::close_generator_expression() {
	assert(current_context().meta_blocks.top() == Context::MetaBlock::generator_expression);
	current_context().meta_blocks.pop();
	if (!is_in_generator_expression()) {
		if (is_in_async_function()) {
			push_node(Node::Command::end_async_generator_expression);
			resolve_jump_forward();
		}
		else {
			push_node(Node::Command::end_generator_expression);
			resolve_jump_forward();
		}
	}
}

void BytecodeBuildContext::open_printer() {
	push_node(Node::Command::open_printer);
	current_context().meta_blocks.push(Context::MetaBlock::printer);
}

void BytecodeBuildContext::close_printer() {
	assert(current_context().meta_blocks.top() == Context::MetaBlock::printer);
	current_context().meta_blocks.pop();
	push_node(Node::Command::close_printer);
}

void BytecodeBuildContext::force_printer() {
	current_context().meta_blocks.push(Context::MetaBlock::printer);
}

void BytecodeBuildContext::start_range_loop() {
	Context& context = current_context();
	context.range_loop_scoped_symbols = std::make_unique<std::vector<const Symbol*>>();
}

void BytecodeBuildContext::resolve_range_loop() {}

void BytecodeBuildContext::start_condition() {
	Context& context = current_context();
	context.condition_scoped_symbols = std::make_unique<std::vector<const Symbol*>>();
}

void BytecodeBuildContext::resolve_condition() {}

void BytecodeBuildContext::open_sub_branch() {
	Context& context = current_context();
	context.branches.emplace(_branch);
	push_branch(context.branches.top());
}

void BytecodeBuildContext::close_sub_branch() {
	pop_branch();
}

void BytecodeBuildContext::build_sub_branch() {
	Context& context = current_context();
	SubBranch branch = std::move(context.branches.top());
	context.branches.pop();
	branch.build();
}

void BytecodeBuildContext::push_node(Node::Command command) {
	_branch.get().push_node(command);
}

void BytecodeBuildContext::push_node(int parameter) {
	_branch.get().push_node(parameter);
}

void BytecodeBuildContext::push_node(std::size_t parameter) {
	_branch.get().push_node(static_cast<int>(parameter));
}

void BytecodeBuildContext::push_node(const char* symbol) {
	_branch.get().push_node(_data.get().bytecode.make_symbol(symbol));
}

void BytecodeBuildContext::push_node(const Symbol* symbol) {
	_branch.get().push_node(symbol);
}

void BytecodeBuildContext::push_node(Data& constant) {
	_branch.get().push_node(_data.get().bytecode.make_constant(constant));
}

void BytecodeBuildContext::push_node(ClassDescription* desc) {
	_branch.get().push_node(desc);
}

std::size_t BytecodeBuildContext::next_offset() const {
	return _branch.get().next_node_offset();
}

void BytecodeBuildContext::push_node(const Reference* constant) {
	_branch.get().push_node(constant);
}

void BytecodeBuildContext::push_branch(Branch& branch) {
	_branches.push(_branch);
	_branch = std::ref(branch);
}

void BytecodeBuildContext::pop_branch() {
	_branch = _branches.top();
	_branches.pop();
}

void BytecodeBuildContext::start_operator(Class::Operator op) {
	_operators.push(op);
}

Class::Operator BytecodeBuildContext::retrieve_operator() {
	assert(!_operators.empty());
	const auto op = _operators.top();
	_operators.pop();
	return op;
}

Symbol BytecodeBuildContext::retrieve_operator_symbol() {
	assert(!_operators.empty());
	const auto op = _operators.top();
	_operators.pop();
	return get_operator_symbol(op);
}

void BytecodeBuildContext::start_modifiers(Reference::Flags flags) {
	_modifiers.push(flags);
}

void BytecodeBuildContext::add_modifiers(Reference::Flags flags) {
	assert(!_modifiers.empty());
	_modifiers.top() |= flags;
}

Reference::Flags BytecodeBuildContext::get_modifiers() const {
	assert(!_modifiers.empty());
	return _modifiers.top();
}

Reference::Flags BytecodeBuildContext::retrieve_modifiers() {
	assert(!_modifiers.empty());
	const auto flags = _modifiers.top();
	_modifiers.pop();
	return flags;
}

Compiler& mint::BytecodeBuildContext::compiler() {
	return _compiler;
}

Block* BytecodeBuildContext::current_breakable_block() {
	const auto& current_stack = current_context().blocks;
	const auto it = std::ranges::find_if(std::views::reverse(current_stack), [](const auto& block) {
		return block->is_breakable();
	});
	if (it != current_stack.rend()) {
		return it->get();
	}
	return nullptr;
}

const Block* BytecodeBuildContext::current_breakable_block() const {
	const auto& current_stack = current_context().blocks;
	for (const auto& block : std::views::reverse(current_stack)) {
		if (block->is_breakable()) {
			return block.get();
		}
	}
	return nullptr;
}

Block* BytecodeBuildContext::current_continuable_block() {
	const auto& current_stack = current_context().blocks;
	const auto it = std::ranges::find_if(std::views::reverse(current_stack), [](const auto& block) {
		return block->is_continuable();
	});
	if (it != current_stack.rend()) {
		return it->get();
	}
	return nullptr;
}

const Block* BytecodeBuildContext::current_continuable_block() const {
	const auto& current_stack = current_context().blocks;
	for (const auto& block : std::views::reverse(current_stack)) {
		if (block->is_continuable()) {
			return block.get();
		}
	}
	return nullptr;
}

Context& BytecodeBuildContext::current_context() {
	if (_definitions.empty()) {
		return *_module_context;
	}
	return *_definitions.top();
}

const Context& BytecodeBuildContext::current_context() const {
	if (_definitions.empty()) {
		return *_module_context;
	}
	return *_definitions.top();
}

Definition* BytecodeBuildContext::current_definition() {
	if (_definitions.empty()) {
		return nullptr;
	}
	return _definitions.top().get();
}

const Definition* BytecodeBuildContext::current_definition() const {
	if (_definitions.empty()) {
		return nullptr;
	}
	return _definitions.top().get();
}

std::size_t BytecodeBuildContext::find_fast_symbol_index(const Symbol& symbol) const {
	if (const Definition* def = current_definition(); def && def->with_fast) {
		return mint::find_fast_symbol_index(*def, symbol);
	}
	return invalid_index;
}

void BytecodeBuildContext::reset_scoped_symbols(const std::vector<const Symbol*>& symbols) {
	for (const auto* symbol : std::views::reverse(symbols)) {
		const auto index = find_fast_symbol_index(*symbol);
		if (index != invalid_index) {
			push_node(Node::Command::reset_fast);
			push_node(symbol);
			push_node(index);
		}
		else {
			push_node(Node::Command::reset_symbol);
			push_node(symbol);
		}
	}
}

AbstractSyntaxTreeBuildContext::AbstractSyntaxTreeBuildContext(DataStream& stream, AbstractSyntaxTree& tree) :
    BuildContext(stream),
    _tree(tree) {
	_tree.set_root({});
}

void AbstractSyntaxTreeBuildContext::commit_line_impl() {}

AbstractSyntaxTree& AbstractSyntaxTreeBuildContext::tree() const {
	return _tree;
}

std::unique_ptr<Expression> AbstractSyntaxTreeBuildContext::take_expression() {
	if (_expressions.empty()) {
		throw std::logic_error("abstract syntax tree expression stack underflow");
	}
	auto expression = std::move(_expressions.back());
	_expressions.pop_back();
	return expression;
}

void AbstractSyntaxTreeBuildContext::reduce_binary_expression(std::string op) {
	if (_expressions.size() < 2) {
		throw std::logic_error("abstract syntax tree binary expression stack underflow");
	}
	auto right = take_expression();
	auto left = take_expression();
	_expressions.emplace_back(std::make_unique<BinaryExpression>(std::move(op), std::move(left), std::move(right)));
}

void AbstractSyntaxTreeBuildContext::reduce_unary_expression(std::string op) {
	if (_expressions.empty()) {
		throw std::logic_error("abstract syntax tree unary expression stack underflow");
	}
	auto operand = take_expression();
	_expressions.emplace_back(std::make_unique<UnaryExpression>(std::move(op), std::move(operand)));
}

void AbstractSyntaxTreeBuildContext::build_conditional_expression() {
	if (_expressions.size() < 3) {
		throw std::logic_error("abstract syntax tree conditional expression stack underflow");
	}
	auto else_expression = take_expression();
	auto then_expression = take_expression();
	auto condition = take_expression();
	_expressions.emplace_back(std::make_unique<ConditionalExpression>(std::move(condition), std::move(then_expression),
	    std::move(else_expression)));
}

void AbstractSyntaxTreeBuildContext::append_control_statement(ControlStatement::Kind kind) {
	auto statement = std::make_unique<ControlStatement>(kind);
	statement->expressions.reserve(_expressions.size());
	while (!_expressions.empty()) {
		statement->expressions.emplace_back(std::move(_expressions.back()));
		_expressions.pop_back();
	}
	std::ranges::reverse(statement->expressions);
	_statements.emplace_back(std::move(statement));
}

void AbstractSyntaxTreeBuildContext::reduce_module_stmt_list() {
	if (!_expressions.empty() || !_calls.empty()) {
		throw std::logic_error("abstract syntax tree has unconsumed expressions at module root");
	}
	auto module = std::make_unique<ModuleNode>();
	module->statements = std::move(_statements);
	_tree.set_root(std::move(module));
}

void AbstractSyntaxTreeBuildContext::reduce_load_statement(const std::string& module_path) {
	append_control_statement(ControlStatement::Kind::load);
}

void AbstractSyntaxTreeBuildContext::reduce_try_bloc() {
	append_control_statement(ControlStatement::Kind::try_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_catch_bloc() {
	append_control_statement(ControlStatement::Kind::try_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_if_bloc() {
	append_control_statement(ControlStatement::Kind::if_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_elif_bloc() {
	append_control_statement(ControlStatement::Kind::if_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_else_bloc() {
	append_control_statement(ControlStatement::Kind::if_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_switch_bloc() {
	append_control_statement(ControlStatement::Kind::switch_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_while_bloc() {
	append_control_statement(ControlStatement::Kind::while_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_for_bloc() {
	append_control_statement(ControlStatement::Kind::for_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_break_statement() {
	append_control_statement(ControlStatement::Kind::break_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_continue_statement() {
	append_control_statement(ControlStatement::Kind::continue_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_print_to_stream_statement() {
	append_control_statement(ControlStatement::Kind::print_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_print_statement() {
	append_control_statement(ControlStatement::Kind::print_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_print_bloc() {
	append_control_statement(ControlStatement::Kind::print_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_yield_generator_expression_statement() {
	append_control_statement(ControlStatement::Kind::yield_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_yield_statement() {
	append_control_statement(ControlStatement::Kind::yield_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_return_generator_expression_statement() {
	append_control_statement(ControlStatement::Kind::return_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_return_statement() {
	append_control_statement(ControlStatement::Kind::return_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_raise_statement() {
	append_control_statement(ControlStatement::Kind::raise_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_raise_in_statement() {
	append_control_statement(ControlStatement::Kind::raise_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_reraise_statement() {
	append_control_statement(ControlStatement::Kind::raise_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_exit_with_code_statement() {
	append_control_statement(ControlStatement::Kind::exit_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_exit_statement() {
	append_control_statement(ControlStatement::Kind::exit_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_identifier_iterator_assignment_expression() {
	append_control_statement(ControlStatement::Kind::assignment_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_identifier_iterator_assignment_generator_expression() {
	append_control_statement(ControlStatement::Kind::assignment_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_create_identifier_iterator_assignment_expression() {
	append_control_statement(ControlStatement::Kind::assignment_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_create_identifier_iterator_assignment_generator_expression() {
	append_control_statement(ControlStatement::Kind::assignment_statement);
}

void AbstractSyntaxTreeBuildContext::reduce_expression_statement() {
	_statements.emplace_back(std::make_unique<ExpressionStatement>(take_expression()));
}

void AbstractSyntaxTreeBuildContext::reduce_function_definition_with_modifiers(const std::string&) {
	append_control_statement(ControlStatement::Kind::definition);
}

void AbstractSyntaxTreeBuildContext::reduce_function_definition(const std::string&) {
	append_control_statement(ControlStatement::Kind::definition);
}

void AbstractSyntaxTreeBuildContext::reduce_empty_statement() {}

void AbstractSyntaxTreeBuildContext::reduce_package_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_package_block() {}

void AbstractSyntaxTreeBuildContext::reduce_class_declaration_with_modifiers(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_class_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_parent_class() {}

void AbstractSyntaxTreeBuildContext::reduce_add_parent_class() {}

void AbstractSyntaxTreeBuildContext::reduce_parent_class_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_qualified_parent_class_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_class_definition() {}

void AbstractSyntaxTreeBuildContext::reduce_nested_class_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_modified_nested_class_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_nested_class_definition() {}

void AbstractSyntaxTreeBuildContext::reduce_nested_enum_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_modified_nested_enum_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_nested_enum_definition() {}

void AbstractSyntaxTreeBuildContext::reduce_member_without_initializer(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_constant_initializer(const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_string_initializer(const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_regex_initializer(const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_flagged_regex_initializer(const std::string&,
    const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_number_initializer(const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_empty_array_initializer(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_empty_hash_initializer(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_with_library_initializer(const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_function_definition(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_member_function_update(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_named_function_definition(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_async_function_definition(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_operator_function_definition() {}

void AbstractSyntaxTreeBuildContext::reduce_modified_named_function_definition(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_modified_async_function_definition(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_modified_operator_function_definition() {}

void AbstractSyntaxTreeBuildContext::reduce_empty_descriptor_line() {}

void AbstractSyntaxTreeBuildContext::reduce_in_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_copy_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_or_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_and_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_bor_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_xor_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_band_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_eq_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_ne_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_lt_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_gt_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_le_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_ge_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_shift_left_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_shift_right_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_add_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_sub_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_mul_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_div_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_mod_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_not_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_compl_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_inc_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_dec_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_pow_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_inclusive_range_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_exclusive_range_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_call_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_subscript_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_subscript_move_operator() {}

void AbstractSyntaxTreeBuildContext::reduce_modified_enum_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_enum_declaration(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_enum_definition() {}

void AbstractSyntaxTreeBuildContext::reduce_enum_item_with_value(const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_enum_item_with_implicit_value(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_empty_enum_item() {}

void AbstractSyntaxTreeBuildContext::reduce_conditional_generator_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_if_else_generator_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_if_elif_generator_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_if_elif_else_generator_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_switch_generator_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_while_generator_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_for_generator_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_try_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_catch_clause(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_try_block() {}

void AbstractSyntaxTreeBuildContext::reduce_if_block() {}

void AbstractSyntaxTreeBuildContext::reduce_if_generator_block() {}

void AbstractSyntaxTreeBuildContext::reduce_elif_block() {}

void AbstractSyntaxTreeBuildContext::reduce_add_elif_block() {}

void AbstractSyntaxTreeBuildContext::reduce_elif_generator_block() {}

void AbstractSyntaxTreeBuildContext::reduce_add_elif_generator_block() {}

void AbstractSyntaxTreeBuildContext::reduce_yield_generator_block() {}

void AbstractSyntaxTreeBuildContext::reduce_yield_value_block() {}

void AbstractSyntaxTreeBuildContext::reduce_return_generator_block() {}

void AbstractSyntaxTreeBuildContext::reduce_return_value_block() {}

void AbstractSyntaxTreeBuildContext::reduce_raise_block() {}

void AbstractSyntaxTreeBuildContext::reduce_reraise_block() {}

void AbstractSyntaxTreeBuildContext::reduce_bare_raise_block() {}

void AbstractSyntaxTreeBuildContext::reduce_expression_block() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_expression_body() {}

void AbstractSyntaxTreeBuildContext::reduce_if_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_if_generator_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_elif_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_if_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_if_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_elif_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_else_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_switch_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_switch_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_switch_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_switch_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_case_keyword() {}

std::string AbstractSyntaxTreeBuildContext::reduce_case_symbol(const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_qualified_case_symbol(const std::string&, const std::string&,
    const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_case_constant(const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_positive_case_number(const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_negative_case_number(const std::string&, const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_append_case_constant(const std::string&, const std::string&,
    const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_start_case_constant_list(const std::string&, const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_finish_case_constant_list(const std::string&) {
	return {};
}

void AbstractSyntaxTreeBuildContext::reduce_empty_case_constant_list() {}

void AbstractSyntaxTreeBuildContext::reduce_inclusive_case_range_label(const std::string&, const std::string&,
    const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_exclusive_case_range_label(const std::string&, const std::string&,
    const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_case_constant_list_label(const std::string&, const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_case_constant_membership_label(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_case_symbol_membership_label(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_case_constant_identity_label(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_case_symbol_identity_label(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_case_constant_label(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_case_symbol_label(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_default_case_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_empty_case_line() {}

void AbstractSyntaxTreeBuildContext::reduce_case_expression_body() {}

void AbstractSyntaxTreeBuildContext::reduce_add_case_expression_body() {}

void AbstractSyntaxTreeBuildContext::reduce_default_expression_body() {}

void AbstractSyntaxTreeBuildContext::reduce_add_default_expression_body() {}

void AbstractSyntaxTreeBuildContext::reduce_while_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_while_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_while_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_while_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_range_for_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_iterator_for_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_for_in_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_range_for_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_iterator_for_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_for_in_condition() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_for_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_for_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_for_identifier() {}

void AbstractSyntaxTreeBuildContext::reduce_for_identifier() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_for_iterator_identifier() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_for_created_iterator() {}

void AbstractSyntaxTreeBuildContext::reduce_for_iterator_identifier() {}

void AbstractSyntaxTreeBuildContext::reduce_for_created_iterator() {}

void AbstractSyntaxTreeBuildContext::reduce_range_initial_value() {}

void AbstractSyntaxTreeBuildContext::reduce_range_condition_value() {}

void AbstractSyntaxTreeBuildContext::reduce_range_step_value() {}

void AbstractSyntaxTreeBuildContext::reduce_return_keyword() {}

void AbstractSyntaxTreeBuildContext::reduce_hash_literal_start() {}

void AbstractSyntaxTreeBuildContext::reduce_hash_literal_end() {}

void AbstractSyntaxTreeBuildContext::reduce_add_hash_entry() {}

void AbstractSyntaxTreeBuildContext::reduce_hash_entry() {}

void AbstractSyntaxTreeBuildContext::reduce_array_literal_start() {}

void AbstractSyntaxTreeBuildContext::reduce_array_literal_end() {}

void AbstractSyntaxTreeBuildContext::reduce_array_value() {}

void AbstractSyntaxTreeBuildContext::reduce_array_spread() {}

void AbstractSyntaxTreeBuildContext::reduce_array_unpack() {}

void AbstractSyntaxTreeBuildContext::reduce_array_generator() {}

void AbstractSyntaxTreeBuildContext::reduce_add_iterator_value() {}

void AbstractSyntaxTreeBuildContext::reduce_iterator_value() {}

void AbstractSyntaxTreeBuildContext::reduce_add_iterator_spread() {}

void AbstractSyntaxTreeBuildContext::reduce_add_iterator_unpack() {}

void AbstractSyntaxTreeBuildContext::reduce_iterator_spread() {}

void AbstractSyntaxTreeBuildContext::reduce_iterator_unpack() {}

void AbstractSyntaxTreeBuildContext::reduce_iterator_end_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_empty_iterator_end() {}

void AbstractSyntaxTreeBuildContext::reduce_add_identifier_iterator_target() {}

void AbstractSyntaxTreeBuildContext::reduce_identifier_iterator_target() {}

void AbstractSyntaxTreeBuildContext::reduce_identifier_iterator_end() {}

void AbstractSyntaxTreeBuildContext::reduce_empty_identifier_iterator_end() {}

void AbstractSyntaxTreeBuildContext::reduce_let_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_scoped_iterator_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_first_scoped_iterator_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_scoped_iterator_end_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_add_iterator_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_first_iterator_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_iterator_end_name(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_print_argument_separator() {}

void AbstractSyntaxTreeBuildContext::reduce_print_block_target() {}

void AbstractSyntaxTreeBuildContext::reduce_print_block_without_target() {}

void AbstractSyntaxTreeBuildContext::reduce_assignment_from_generator_expression() {
	if (_expressions.size() >= 2) {
		reduce_binary_expression("=");
	}
}

void AbstractSyntaxTreeBuildContext::reduce_assignment_expression() {
	reduce_binary_expression("=");
}

void AbstractSyntaxTreeBuildContext::reduce_binding_from_generator_expression() {
	if (_expressions.size() >= 2) {
		reduce_binary_expression(":=");
	}
}

void AbstractSyntaxTreeBuildContext::reduce_binding_expression() {
	reduce_binary_expression(":=");
}

void AbstractSyntaxTreeBuildContext::begin_iterator_initialization_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_iterator_initialization_expression() {
	reduce_binary_expression("=:");
}

void AbstractSyntaxTreeBuildContext::reduce_addition_expression() {
	reduce_binary_expression("+");
}

void AbstractSyntaxTreeBuildContext::reduce_subtraction_expression() {
	reduce_binary_expression("-");
}

void AbstractSyntaxTreeBuildContext::reduce_multiplication_expression() {
	reduce_binary_expression("*");
}

void AbstractSyntaxTreeBuildContext::reduce_division_expression() {
	reduce_binary_expression("/");
}

void AbstractSyntaxTreeBuildContext::reduce_modulo_expression() {
	reduce_binary_expression("%");
}

void AbstractSyntaxTreeBuildContext::reduce_exponentiation_expression() {
	reduce_binary_expression("**");
}

void AbstractSyntaxTreeBuildContext::reduce_identity_expression() {
	reduce_binary_expression("is");
}

void AbstractSyntaxTreeBuildContext::reduce_membership_expression() {
	reduce_binary_expression("in");
}

void AbstractSyntaxTreeBuildContext::reduce_negated_membership_expression() {
	reduce_binary_expression("!in");
}

void AbstractSyntaxTreeBuildContext::reduce_equality_expression() {
	reduce_binary_expression("==");
}

void AbstractSyntaxTreeBuildContext::reduce_inequality_expression() {
	reduce_binary_expression("!=");
}

void AbstractSyntaxTreeBuildContext::reduce_less_than_expression() {
	reduce_binary_expression("<");
}

void AbstractSyntaxTreeBuildContext::reduce_greater_than_expression() {
	reduce_binary_expression(">");
}

void AbstractSyntaxTreeBuildContext::reduce_less_than_or_equal_expression() {
	reduce_binary_expression("<=");
}

void AbstractSyntaxTreeBuildContext::reduce_greater_than_or_equal_expression() {
	reduce_binary_expression(">=");
}

void AbstractSyntaxTreeBuildContext::reduce_left_shift_expression() {
	reduce_binary_expression("<<");
}

void AbstractSyntaxTreeBuildContext::reduce_right_shift_expression() {
	reduce_binary_expression(">>");
}

void AbstractSyntaxTreeBuildContext::reduce_inclusive_range_expression() {
	reduce_binary_expression("..");
}

void AbstractSyntaxTreeBuildContext::reduce_exclusive_range_expression() {
	reduce_binary_expression("...");
}

void AbstractSyntaxTreeBuildContext::reduce_prefix_increment_expression() {
	reduce_unary_expression("++");
}

void AbstractSyntaxTreeBuildContext::reduce_prefix_decrement_expression() {
	reduce_unary_expression("--");
}

void AbstractSyntaxTreeBuildContext::reduce_postfix_increment_expression() {
	reduce_unary_expression("++");
}

void AbstractSyntaxTreeBuildContext::reduce_postfix_decrement_expression() {
	reduce_unary_expression("--");
}

void AbstractSyntaxTreeBuildContext::reduce_logical_not_expression() {
	reduce_unary_expression("!");
}

void AbstractSyntaxTreeBuildContext::begin_logical_or_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_logical_or_expression() {
	reduce_binary_expression("||");
}

void AbstractSyntaxTreeBuildContext::begin_logical_and_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_logical_and_expression() {
	reduce_binary_expression("&&");
}

void AbstractSyntaxTreeBuildContext::reduce_bitwise_or_expression() {
	reduce_binary_expression("|");
}

void AbstractSyntaxTreeBuildContext::reduce_bitwise_and_expression() {
	reduce_binary_expression("&");
}

void AbstractSyntaxTreeBuildContext::reduce_bitwise_xor_expression() {
	reduce_binary_expression("^");
}

void AbstractSyntaxTreeBuildContext::reduce_bitwise_not_expression() {
	reduce_unary_expression("~");
}

void AbstractSyntaxTreeBuildContext::reduce_unary_plus_expression() {
	reduce_unary_expression("+");
}

void AbstractSyntaxTreeBuildContext::reduce_unary_minus_expression() {
	reduce_unary_expression("-");
}

void AbstractSyntaxTreeBuildContext::reduce_await_expression() {
	reduce_unary_expression("await");
}

void AbstractSyntaxTreeBuildContext::reduce_typeof_expression() {
	reduce_unary_expression("typeof");
}

void AbstractSyntaxTreeBuildContext::reduce_membersof_expression() {
	reduce_unary_expression("membersof");
}

void AbstractSyntaxTreeBuildContext::reduce_defined_expression() {
	reduce_unary_expression("defined");
}

void AbstractSyntaxTreeBuildContext::reduce_slice_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::begin_addition_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_addition_assignment_expression() {
	reduce_binary_expression("+=");
}

void AbstractSyntaxTreeBuildContext::begin_subtraction_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_subtraction_assignment_expression() {
	reduce_binary_expression("-=");
}

void AbstractSyntaxTreeBuildContext::begin_multiplication_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_multiplication_assignment_expression() {
	reduce_binary_expression("*=");
}

void AbstractSyntaxTreeBuildContext::begin_division_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_division_assignment_expression() {
	reduce_binary_expression("/=");
}

void AbstractSyntaxTreeBuildContext::begin_modulo_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_modulo_assignment_expression() {
	reduce_binary_expression("%=");
}

void AbstractSyntaxTreeBuildContext::begin_left_shift_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_left_shift_assignment_expression() {
	reduce_binary_expression("<<=");
}

void AbstractSyntaxTreeBuildContext::begin_right_shift_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_right_shift_assignment_expression() {
	reduce_binary_expression(">>=");
}

void AbstractSyntaxTreeBuildContext::begin_bitwise_and_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_bitwise_and_assignment_expression() {
	reduce_binary_expression("&=");
}

void AbstractSyntaxTreeBuildContext::begin_bitwise_or_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_bitwise_or_assignment_expression() {
	reduce_binary_expression("|=");
}

void AbstractSyntaxTreeBuildContext::begin_bitwise_xor_assignment_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_bitwise_xor_assignment_expression() {
	reduce_binary_expression("^=");
}

void AbstractSyntaxTreeBuildContext::reduce_regex_match_expression() {
	reduce_binary_expression("=~");
}

void AbstractSyntaxTreeBuildContext::reduce_regex_non_match_expression() {
	reduce_binary_expression("!~");
}

void AbstractSyntaxTreeBuildContext::reduce_strict_equality_expression() {
	reduce_binary_expression("===");
}

void AbstractSyntaxTreeBuildContext::reduce_strict_inequality_expression() {
	reduce_binary_expression("!==");
}

void AbstractSyntaxTreeBuildContext::begin_conditional_expression() {}

void AbstractSyntaxTreeBuildContext::continue_conditional_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_conditional_expression() {
	build_conditional_expression();
}

void AbstractSyntaxTreeBuildContext::reduce_empty_parenthesized_expression() {}

void AbstractSyntaxTreeBuildContext::reduce_subscript_expression() {
	auto index = take_expression();
	auto object = take_expression();
	_expressions.emplace_back(std::make_unique<SubscriptExpression>(std::move(object), std::move(index)));
}

void AbstractSyntaxTreeBuildContext::reduce_defined_member_call_arguments() {}

void AbstractSyntaxTreeBuildContext::reduce_call_arguments_start() {
	_calls.emplace_back(std::make_unique<CallExpression>(take_expression()));
}

void AbstractSyntaxTreeBuildContext::reduce_call_arguments_end() {
	if (_calls.empty()) {
		throw std::logic_error("abstract syntax tree call stack underflow");
	}
	_expressions.emplace_back(std::move(_calls.back()));
	_calls.pop_back();
}

void AbstractSyntaxTreeBuildContext::reduce_member_call_start(const std::string& member) {
	auto callee = std::make_unique<MemberExpression>(take_expression(), member);
	_calls.emplace_back(std::make_unique<CallExpression>(std::move(callee)));
}

void AbstractSyntaxTreeBuildContext::reduce_operator_member_call_start() {}

void AbstractSyntaxTreeBuildContext::reduce_variable_member_call_start() {}

void AbstractSyntaxTreeBuildContext::reduce_defined_member_call_start(const std::string& member) {
	auto callee = std::make_unique<MemberExpression>(take_expression(), member);
	_calls.emplace_back(std::make_unique<CallExpression>(std::move(callee)));
}

void AbstractSyntaxTreeBuildContext::reduce_defined_operator_member_call_start() {}

void AbstractSyntaxTreeBuildContext::reduce_defined_variable_member_call_start() {}

void AbstractSyntaxTreeBuildContext::reduce_member_call_arguments_end() {
	if (_calls.empty()) {
		throw std::logic_error("abstract syntax tree member call stack underflow");
	}
	_expressions.emplace_back(std::move(_calls.back()));
	_calls.pop_back();
}

void AbstractSyntaxTreeBuildContext::reduce_positional_call_argument() {
	if (_calls.empty()) {
		throw std::logic_error("abstract syntax tree call argument has no active call");
	}
	_calls.back()->arguments.emplace_back(take_expression());
}

void AbstractSyntaxTreeBuildContext::reduce_callable_call_argument() {}

void AbstractSyntaxTreeBuildContext::reduce_generator_call_argument() {}

void AbstractSyntaxTreeBuildContext::reduce_spread_call_argument() {}

void AbstractSyntaxTreeBuildContext::reduce_unpack_call_argument() {}

void AbstractSyntaxTreeBuildContext::reduce_function_definition_with_arguments() {}

void AbstractSyntaxTreeBuildContext::reduce_function_definition_without_arguments() {}

void AbstractSyntaxTreeBuildContext::reduce_arrow_function_definition() {}

void AbstractSyntaxTreeBuildContext::reduce_function_definition_start() {}

void AbstractSyntaxTreeBuildContext::reduce_async_function_definition_start() {}

void AbstractSyntaxTreeBuildContext::reduce_capture_list_start() {}

void AbstractSyntaxTreeBuildContext::reduce_capture_list_end() {}

void AbstractSyntaxTreeBuildContext::reduce_capture_with_initializer(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_final_capture_with_initializer(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_capture(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_final_capture(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_capture_all() {}

void AbstractSyntaxTreeBuildContext::reduce_no_function_arguments() {}

void AbstractSyntaxTreeBuildContext::reduce_function_arguments_end() {}

void AbstractSyntaxTreeBuildContext::reduce_function_argument(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_function_argument_with_default(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_modified_function_argument(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_modified_function_argument_with_default(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_function_argument_unpack() {}

void AbstractSyntaxTreeBuildContext::reduce_arrow_function_body_start() {}

void AbstractSyntaxTreeBuildContext::reduce_member_access(const std::string& member) {
	_expressions.emplace_back(std::make_unique<MemberExpression>(take_expression(), member));
}

void AbstractSyntaxTreeBuildContext::reduce_operator_member_access() {}

void AbstractSyntaxTreeBuildContext::reduce_variable_member_access() {}

void AbstractSyntaxTreeBuildContext::reduce_optional_member_access(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_optional_operator_member_access() {}

void AbstractSyntaxTreeBuildContext::reduce_optional_variable_member_access() {}

void AbstractSyntaxTreeBuildContext::reduce_defined_symbol(const std::string& symbol) {
	_expressions.emplace_back(std::make_unique<Identifier>(symbol));
}

void AbstractSyntaxTreeBuildContext::reduce_qualified_defined_symbol(const std::string& symbol) {
	_expressions.emplace_back(std::make_unique<MemberExpression>(take_expression(), symbol));
}

void AbstractSyntaxTreeBuildContext::reduce_defined_variable_symbol() {}

void AbstractSyntaxTreeBuildContext::reduce_qualified_defined_variable_symbol() {}

void AbstractSyntaxTreeBuildContext::reduce_defined_constant_symbol(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_constant_identifier(const std::string&) {}

void AbstractSyntaxTreeBuildContext::reduce_library_identifier() {
	_expressions.emplace_back(std::make_unique<Identifier>("lib"));
}

void AbstractSyntaxTreeBuildContext::reduce_variable_identifier() {
	_expressions.emplace_back(std::make_unique<VariableExpression>(take_expression()));
}

void AbstractSyntaxTreeBuildContext::reduce_identifier(const std::string& name) {
	_expressions.emplace_back(std::make_unique<Identifier>(name));
}

void AbstractSyntaxTreeBuildContext::reduce_let_identifier(const std::string& name) {
	_expressions.emplace_back(std::make_unique<Identifier>(name));
}

void AbstractSyntaxTreeBuildContext::reduce_modified_identifier(const std::string& name) {
	_expressions.emplace_back(std::make_unique<Identifier>(name));
}

void AbstractSyntaxTreeBuildContext::reduce_let_modified_identifier(const std::string& name) {
	_expressions.emplace_back(std::make_unique<Identifier>(name));
}

std::string AbstractSyntaxTreeBuildContext::reduce_constant_value(const std::string& value) {
	_expressions.emplace_back(std::make_unique<Literal>(Literal::Kind::constant, value));
	return value;
}

std::string AbstractSyntaxTreeBuildContext::reduce_regular_expression(const std::string& value) {
	_expressions.emplace_back(std::make_unique<Literal>(Literal::Kind::regex, value));
	return value;
}

std::string AbstractSyntaxTreeBuildContext::reduce_regular_expression_with_symbol(const std::string& regex,
    const std::string& symbol) {
	const auto value = regex + symbol;
	_expressions.emplace_back(std::make_unique<Literal>(Literal::Kind::regex, value));
	return value;
}

std::string AbstractSyntaxTreeBuildContext::reduce_string_literal(const std::string& value) {
	_expressions.emplace_back(std::make_unique<Literal>(Literal::Kind::string, value));
	return value;
}

std::string AbstractSyntaxTreeBuildContext::reduce_number_literal(const std::string& value) {
	_expressions.emplace_back(std::make_unique<Literal>(Literal::Kind::number, value));
	return value;
}

std::string AbstractSyntaxTreeBuildContext::reduce_regular_expression_start(const std::string&) {
	return {};
}

std::string AbstractSyntaxTreeBuildContext::reduce_regular_expression_end(const std::string&, const std::string&) {
	return {};
}

void AbstractSyntaxTreeBuildContext::reduce_default_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_const_address_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_const_value_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_const_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_global_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_final_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_override_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_public_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_protected_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_private_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_package_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_default_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_const_address_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_const_value_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_const_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_global_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_final_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_override_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_public_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_protected_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_private_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_add_package_modifier() {}

void AbstractSyntaxTreeBuildContext::reduce_statement_separator() {}

void AbstractSyntaxTreeBuildContext::reduce_empty_line() {}

void AbstractSyntaxTreeBuildContext::reduce_add_empty_line() {}
