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

#ifndef MINT_COMPILER_BUILD_CONTEXT_H
#define MINT_COMPILER_BUILD_CONTEXT_H

#include "mint/compiler/lexer.h"
#include "mint/config.h"
#include "mint/system/data_stream.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace mint {

class MINT_EXPORT BuildContext {
	Lexer _lexer;
public:
	explicit BuildContext(DataStream& stream);
	BuildContext(BuildContext&&) = delete;
	BuildContext(const BuildContext&) = delete;
	virtual ~BuildContext() = default;

	BuildContext& operator=(BuildContext&&) = delete;
	BuildContext& operator=(const BuildContext&) = delete;

	void commit_line();
	[[nodiscard]] std::string read_regex();
	int next_token(std::string* token);
	[[noreturn]] void parse_error(const std::string& error_msg) const;

	virtual void reduce_module_stmt_list() = 0;
	virtual void reduce_load_statement(const std::string& module_path) = 0;
	virtual void reduce_try_bloc() = 0;
	virtual void reduce_catch_bloc() = 0;
	virtual void reduce_if_bloc() = 0;
	virtual void reduce_else_bloc() = 0;
	virtual void reduce_elif_bloc() = 0;
	virtual void reduce_switch_bloc() = 0;
	virtual void reduce_while_bloc() = 0;
	virtual void reduce_for_bloc() = 0;
	virtual void reduce_break_statement() = 0;
	virtual void reduce_continue_statement() = 0;
	virtual void reduce_print_to_stream_statement() = 0;
	virtual void reduce_print_statement() = 0;
	virtual void reduce_print_bloc() = 0;
	virtual void reduce_yield_generator_expression_statement() = 0;
	virtual void reduce_yield_statement() = 0;
	virtual void reduce_return_generator_expression_statement() = 0;
	virtual void reduce_return_statement() = 0;
	virtual void reduce_raise_statement() = 0;
	virtual void reduce_raise_in_statement() = 0;
	virtual void reduce_reraise_statement() = 0;
	virtual void reduce_exit_with_code_statement() = 0;
	virtual void reduce_exit_statement() = 0;
	virtual void reduce_identifier_iterator_assignment_expression() = 0;
	virtual void reduce_identifier_iterator_assignment_generator_expression() = 0;
	virtual void reduce_create_identifier_iterator_assignment_expression() = 0;
	virtual void reduce_create_identifier_iterator_assignment_generator_expression() = 0;
	virtual void reduce_expression_statement() = 0;
	virtual void reduce_function_definition_with_modifiers(const std::string& name) = 0;
	virtual void reduce_function_definition(const std::string& name) = 0;
	virtual void reduce_empty_statement() = 0;
	virtual void reduce_package_declaration(const std::string& package_name) = 0;
	virtual void reduce_package_block() = 0;
	virtual void reduce_class_declaration_with_modifiers(const std::string& class_name) = 0;
	virtual void reduce_class_declaration(const std::string& class_name) = 0;
	virtual void reduce_parent_class() = 0;
	virtual void reduce_add_parent_class() = 0;
	virtual void reduce_parent_class_name(const std::string& parent_class_name) = 0;
	virtual void reduce_qualified_parent_class_name(const std::string& parent_class_name) = 0;
	virtual void reduce_class_definition() = 0;
	virtual void reduce_nested_class_declaration(const std::string& class_name) = 0;
	virtual void reduce_modified_nested_class_declaration(const std::string& class_name) = 0;
	virtual void reduce_nested_class_definition() = 0;
	virtual void reduce_nested_enum_declaration(const std::string& enum_name) = 0;
	virtual void reduce_modified_nested_enum_declaration(const std::string& enum_name) = 0;
	virtual void reduce_nested_enum_definition() = 0;
	virtual void reduce_member_without_initializer(const std::string& member_name) = 0;
	virtual void reduce_member_with_constant_initializer(const std::string& member_name,
	    const std::string& initializer) = 0;
	virtual void reduce_member_with_string_initializer(const std::string& member_name,
	    const std::string& initializer) = 0;
	virtual void reduce_member_with_regex_initializer(const std::string& member_name, const std::string& pattern) = 0;
	virtual void reduce_member_with_flagged_regex_initializer(const std::string& member_name,
	    const std::string& pattern, const std::string& flags) = 0;
	virtual void reduce_member_with_number_initializer(const std::string& member_name,
	    const std::string& initializer) = 0;
	virtual void reduce_member_with_empty_array_initializer(const std::string& member_name) = 0;
	virtual void reduce_member_with_empty_hash_initializer(const std::string& member_name) = 0;
	virtual void reduce_member_with_library_initializer(const std::string& member_name,
	    const std::string& library_name) = 0;
	virtual void reduce_member_function_definition(const std::string& function_name) = 0;
	virtual void reduce_member_function_update(const std::string& function_name) = 0;
	virtual void reduce_named_function_definition(const std::string& function_name) = 0;
	virtual void reduce_async_function_definition(const std::string& function_name) = 0;
	virtual void reduce_operator_function_definition() = 0;
	virtual void reduce_modified_named_function_definition(const std::string& function_name) = 0;
	virtual void reduce_modified_async_function_definition(const std::string& function_name) = 0;
	virtual void reduce_modified_operator_function_definition() = 0;
	virtual void reduce_empty_descriptor_line() = 0;
	virtual void reduce_in_operator() = 0;
	virtual void reduce_copy_operator() = 0;
	virtual void reduce_or_operator() = 0;
	virtual void reduce_and_operator() = 0;
	virtual void reduce_bor_operator() = 0;
	virtual void reduce_xor_operator() = 0;
	virtual void reduce_band_operator() = 0;
	virtual void reduce_eq_operator() = 0;
	virtual void reduce_ne_operator() = 0;
	virtual void reduce_lt_operator() = 0;
	virtual void reduce_gt_operator() = 0;
	virtual void reduce_le_operator() = 0;
	virtual void reduce_ge_operator() = 0;
	virtual void reduce_shift_left_operator() = 0;
	virtual void reduce_shift_right_operator() = 0;
	virtual void reduce_add_operator() = 0;
	virtual void reduce_sub_operator() = 0;
	virtual void reduce_mul_operator() = 0;
	virtual void reduce_div_operator() = 0;
	virtual void reduce_mod_operator() = 0;
	virtual void reduce_not_operator() = 0;
	virtual void reduce_compl_operator() = 0;
	virtual void reduce_inc_operator() = 0;
	virtual void reduce_dec_operator() = 0;
	virtual void reduce_pow_operator() = 0;
	virtual void reduce_inclusive_range_operator() = 0;
	virtual void reduce_exclusive_range_operator() = 0;
	virtual void reduce_call_operator() = 0;
	virtual void reduce_subscript_operator() = 0;
	virtual void reduce_subscript_move_operator() = 0;
	virtual void reduce_modified_enum_declaration(const std::string& enum_name) = 0;
	virtual void reduce_enum_declaration(const std::string& enum_name) = 0;
	virtual void reduce_enum_definition() = 0;
	virtual void reduce_enum_item_with_value(const std::string& item_name, const std::string& value) = 0;
	virtual void reduce_enum_item_with_implicit_value(const std::string& item_name) = 0;
	virtual void reduce_empty_enum_item() = 0;
	virtual void reduce_conditional_generator_expression() = 0;
	virtual void reduce_if_else_generator_expression() = 0;
	virtual void reduce_if_elif_generator_expression() = 0;
	virtual void reduce_if_elif_else_generator_expression() = 0;
	virtual void reduce_switch_generator_expression() = 0;
	virtual void reduce_while_generator_expression() = 0;
	virtual void reduce_for_generator_expression() = 0;
	virtual void reduce_try_keyword() = 0;
	virtual void reduce_catch_clause(const std::string& exception_name) = 0;
	virtual void reduce_try_block() = 0;
	virtual void reduce_if_block() = 0;
	virtual void reduce_if_generator_block() = 0;
	virtual void reduce_elif_block() = 0;
	virtual void reduce_add_elif_block() = 0;
	virtual void reduce_elif_generator_block() = 0;
	virtual void reduce_add_elif_generator_block() = 0;
	virtual void reduce_yield_generator_block() = 0;
	virtual void reduce_yield_value_block() = 0;
	virtual void reduce_return_generator_block() = 0;
	virtual void reduce_return_value_block() = 0;
	virtual void reduce_raise_block() = 0;
	virtual void reduce_reraise_block() = 0;
	virtual void reduce_bare_raise_block() = 0;
	virtual void reduce_expression_block() = 0;
	virtual void reduce_generator_expression_body() = 0;
	virtual void reduce_if_condition() = 0;
	virtual void reduce_if_generator_condition() = 0;
	virtual void reduce_elif_condition() = 0;
	virtual void reduce_if_keyword() = 0;
	virtual void reduce_generator_if_keyword() = 0;
	virtual void reduce_elif_keyword() = 0;
	virtual void reduce_else_keyword() = 0;
	virtual void reduce_switch_condition() = 0;
	virtual void reduce_generator_switch_condition() = 0;
	virtual void reduce_generator_switch_keyword() = 0;
	virtual void reduce_switch_keyword() = 0;
	virtual void reduce_case_keyword() = 0;
	virtual std::string reduce_case_symbol(const std::string& symbol_name) = 0;
	virtual std::string reduce_qualified_case_symbol(const std::string& parent_symbol, const std::string& separator,
	    const std::string& symbol_name) = 0;
	virtual std::string reduce_case_constant(const std::string& constant) = 0;
	virtual std::string reduce_positive_case_number(const std::string& number) = 0;
	virtual std::string reduce_negative_case_number(const std::string& sign, const std::string& number) = 0;
	virtual std::string reduce_append_case_constant(const std::string& existing_constants, const std::string& constant,
	    const std::string& separator) = 0;
	virtual std::string reduce_start_case_constant_list(const std::string& constant, const std::string& separator) = 0;
	virtual std::string reduce_finish_case_constant_list(const std::string& constant) = 0;
	virtual void reduce_empty_case_constant_list() = 0;
	virtual void reduce_inclusive_case_range_label(const std::string& start_value, const std::string& range_operator,
	    const std::string& end_value) = 0;
	virtual void reduce_exclusive_case_range_label(const std::string& start_value, const std::string& range_operator,
	    const std::string& end_value) = 0;
	virtual void reduce_case_constant_list_label(const std::string& constants, const std::string& last_constant) = 0;
	virtual void reduce_case_constant_membership_label(const std::string& constant) = 0;
	virtual void reduce_case_symbol_membership_label(const std::string& symbol_name) = 0;
	virtual void reduce_case_constant_identity_label(const std::string& constant) = 0;
	virtual void reduce_case_symbol_identity_label(const std::string& symbol_name) = 0;
	virtual void reduce_case_constant_label(const std::string& constant) = 0;
	virtual void reduce_case_symbol_label(const std::string& symbol_name) = 0;
	virtual void reduce_default_case_keyword() = 0;
	virtual void reduce_empty_case_line() = 0;
	virtual void reduce_case_expression_body() = 0;
	virtual void reduce_add_case_expression_body() = 0;
	virtual void reduce_default_expression_body() = 0;
	virtual void reduce_add_default_expression_body() = 0;
	virtual void reduce_while_condition() = 0;
	virtual void reduce_generator_while_condition() = 0;
	virtual void reduce_generator_while_keyword() = 0;
	virtual void reduce_while_keyword() = 0;
	virtual void reduce_range_for_condition() = 0;
	virtual void reduce_iterator_for_condition() = 0;
	virtual void reduce_for_in_condition() = 0;
	virtual void reduce_generator_range_for_condition() = 0;
	virtual void reduce_generator_iterator_for_condition() = 0;
	virtual void reduce_generator_for_in_condition() = 0;
	virtual void reduce_generator_for_keyword() = 0;
	virtual void reduce_for_keyword() = 0;
	virtual void reduce_generator_for_identifier() = 0;
	virtual void reduce_for_identifier() = 0;
	virtual void reduce_generator_for_iterator_identifier() = 0;
	virtual void reduce_generator_for_created_iterator() = 0;
	virtual void reduce_for_iterator_identifier() = 0;
	virtual void reduce_for_created_iterator() = 0;
	virtual void reduce_range_initial_value() = 0;
	virtual void reduce_range_condition_value() = 0;
	virtual void reduce_range_step_value() = 0;
	virtual void reduce_return_keyword() = 0;
	virtual void reduce_hash_literal_start() = 0;
	virtual void reduce_hash_literal_end() = 0;
	virtual void reduce_add_hash_entry() = 0;
	virtual void reduce_hash_entry() = 0;
	virtual void reduce_array_literal_start() = 0;
	virtual void reduce_array_literal_end() = 0;
	virtual void reduce_array_value() = 0;
	virtual void reduce_array_spread() = 0;
	virtual void reduce_array_unpack() = 0;
	virtual void reduce_array_generator() = 0;
	virtual void reduce_add_iterator_value() = 0;
	virtual void reduce_iterator_value() = 0;
	virtual void reduce_add_iterator_spread() = 0;
	virtual void reduce_add_iterator_unpack() = 0;
	virtual void reduce_iterator_spread() = 0;
	virtual void reduce_iterator_unpack() = 0;
	virtual void reduce_iterator_end_expression() = 0;
	virtual void reduce_empty_iterator_end() = 0;
	virtual void reduce_add_identifier_iterator_target() = 0;
	virtual void reduce_identifier_iterator_target() = 0;
	virtual void reduce_identifier_iterator_end() = 0;
	virtual void reduce_empty_identifier_iterator_end() = 0;
	virtual void reduce_let_modifier() = 0;
	virtual void reduce_add_scoped_iterator_name(const std::string& iterator_name) = 0;
	virtual void reduce_first_scoped_iterator_name(const std::string& iterator_name) = 0;
	virtual void reduce_scoped_iterator_end_name(const std::string& iterator_name) = 0;
	virtual void reduce_add_iterator_name(const std::string& iterator_name) = 0;
	virtual void reduce_first_iterator_name(const std::string& iterator_name) = 0;
	virtual void reduce_iterator_end_name(const std::string& iterator_name) = 0;
	virtual void reduce_print_argument_separator() = 0;
	virtual void reduce_print_block_target() = 0;
	virtual void reduce_print_block_without_target() = 0;
	virtual void reduce_assignment_from_generator_expression() = 0;
	virtual void reduce_assignment_expression() = 0;
	virtual void reduce_binding_from_generator_expression() = 0;
	virtual void reduce_binding_expression() = 0;
	virtual void begin_iterator_initialization_expression() = 0;
	virtual void reduce_iterator_initialization_expression() = 0;
	virtual void reduce_addition_expression() = 0;
	virtual void reduce_subtraction_expression() = 0;
	virtual void reduce_multiplication_expression() = 0;
	virtual void reduce_division_expression() = 0;
	virtual void reduce_modulo_expression() = 0;
	virtual void reduce_exponentiation_expression() = 0;
	virtual void reduce_identity_expression() = 0;
	virtual void reduce_membership_expression() = 0;
	virtual void reduce_negated_membership_expression() = 0;
	virtual void reduce_equality_expression() = 0;
	virtual void reduce_inequality_expression() = 0;
	virtual void reduce_less_than_expression() = 0;
	virtual void reduce_greater_than_expression() = 0;
	virtual void reduce_less_than_or_equal_expression() = 0;
	virtual void reduce_greater_than_or_equal_expression() = 0;
	virtual void reduce_left_shift_expression() = 0;
	virtual void reduce_right_shift_expression() = 0;
	virtual void reduce_inclusive_range_expression() = 0;
	virtual void reduce_exclusive_range_expression() = 0;
	virtual void reduce_prefix_increment_expression() = 0;
	virtual void reduce_prefix_decrement_expression() = 0;
	virtual void reduce_postfix_increment_expression() = 0;
	virtual void reduce_postfix_decrement_expression() = 0;
	virtual void reduce_logical_not_expression() = 0;
	virtual void begin_logical_or_expression() = 0;
	virtual void reduce_logical_or_expression() = 0;
	virtual void begin_logical_and_expression() = 0;
	virtual void reduce_logical_and_expression() = 0;
	virtual void reduce_bitwise_or_expression() = 0;
	virtual void reduce_bitwise_and_expression() = 0;
	virtual void reduce_bitwise_xor_expression() = 0;
	virtual void reduce_bitwise_not_expression() = 0;
	virtual void reduce_unary_plus_expression() = 0;
	virtual void reduce_unary_minus_expression() = 0;
	virtual void reduce_await_expression() = 0;
	virtual void reduce_typeof_expression() = 0;
	virtual void reduce_membersof_expression() = 0;
	virtual void reduce_defined_expression() = 0;
	virtual void reduce_slice_assignment_expression() = 0;
	virtual void begin_addition_assignment_expression() = 0;
	virtual void reduce_addition_assignment_expression() = 0;
	virtual void begin_subtraction_assignment_expression() = 0;
	virtual void reduce_subtraction_assignment_expression() = 0;
	virtual void begin_multiplication_assignment_expression() = 0;
	virtual void reduce_multiplication_assignment_expression() = 0;
	virtual void begin_division_assignment_expression() = 0;
	virtual void reduce_division_assignment_expression() = 0;
	virtual void begin_modulo_assignment_expression() = 0;
	virtual void reduce_modulo_assignment_expression() = 0;
	virtual void begin_left_shift_assignment_expression() = 0;
	virtual void reduce_left_shift_assignment_expression() = 0;
	virtual void begin_right_shift_assignment_expression() = 0;
	virtual void reduce_right_shift_assignment_expression() = 0;
	virtual void begin_bitwise_and_assignment_expression() = 0;
	virtual void reduce_bitwise_and_assignment_expression() = 0;
	virtual void begin_bitwise_or_assignment_expression() = 0;
	virtual void reduce_bitwise_or_assignment_expression() = 0;
	virtual void begin_bitwise_xor_assignment_expression() = 0;
	virtual void reduce_bitwise_xor_assignment_expression() = 0;
	virtual void reduce_regex_match_expression() = 0;
	virtual void reduce_regex_non_match_expression() = 0;
	virtual void reduce_strict_equality_expression() = 0;
	virtual void reduce_strict_inequality_expression() = 0;
	virtual void begin_conditional_expression() = 0;
	virtual void continue_conditional_expression() = 0;
	virtual void reduce_conditional_expression() = 0;
	virtual void reduce_empty_parenthesized_expression() = 0;
	virtual void reduce_subscript_expression() = 0;
	virtual void reduce_defined_member_call_arguments() = 0;
	virtual void reduce_call_arguments_start() = 0;
	virtual void reduce_call_arguments_end() = 0;
	virtual void reduce_member_call_start(const std::string& member_name) = 0;
	virtual void reduce_operator_member_call_start() = 0;
	virtual void reduce_variable_member_call_start() = 0;
	virtual void reduce_defined_member_call_start(const std::string& member_name) = 0;
	virtual void reduce_defined_operator_member_call_start() = 0;
	virtual void reduce_defined_variable_member_call_start() = 0;
	virtual void reduce_member_call_arguments_end() = 0;
	virtual void reduce_positional_call_argument() = 0;
	virtual void reduce_callable_call_argument() = 0;
	virtual void reduce_generator_call_argument() = 0;
	virtual void reduce_spread_call_argument() = 0;
	virtual void reduce_unpack_call_argument() = 0;
	virtual void reduce_function_definition_with_arguments() = 0;
	virtual void reduce_function_definition_without_arguments() = 0;
	virtual void reduce_arrow_function_definition() = 0;
	virtual void reduce_function_definition_start() = 0;
	virtual void reduce_async_function_definition_start() = 0;
	virtual void reduce_capture_list_start() = 0;
	virtual void reduce_capture_list_end() = 0;
	virtual void reduce_capture_with_initializer(const std::string& capture_name) = 0;
	virtual void reduce_final_capture_with_initializer(const std::string& capture_name) = 0;
	virtual void reduce_capture(const std::string& capture_name) = 0;
	virtual void reduce_final_capture(const std::string& capture_name) = 0;
	virtual void reduce_capture_all() = 0;
	virtual void reduce_no_function_arguments() = 0;
	virtual void reduce_function_arguments_end() = 0;
	virtual void reduce_function_argument(const std::string& argument_name) = 0;
	virtual void reduce_function_argument_with_default(const std::string& argument_name) = 0;
	virtual void reduce_modified_function_argument(const std::string& argument_name) = 0;
	virtual void reduce_modified_function_argument_with_default(const std::string& argument_name) = 0;
	virtual void reduce_function_argument_unpack() = 0;
	virtual void reduce_arrow_function_body_start() = 0;
	virtual void reduce_member_access(const std::string& member_name) = 0;
	virtual void reduce_operator_member_access() = 0;
	virtual void reduce_variable_member_access() = 0;
	virtual void reduce_optional_member_access(const std::string& member_name) = 0;
	virtual void reduce_optional_operator_member_access() = 0;
	virtual void reduce_optional_variable_member_access() = 0;
	virtual void reduce_defined_symbol(const std::string& symbol_name) = 0;
	virtual void reduce_qualified_defined_symbol(const std::string& symbol_name) = 0;
	virtual void reduce_defined_variable_symbol() = 0;
	virtual void reduce_qualified_defined_variable_symbol() = 0;
	virtual void reduce_defined_constant_symbol(const std::string& constant) = 0;
	virtual void reduce_constant_identifier(const std::string& constant) = 0;
	virtual void reduce_library_identifier() = 0;
	virtual void reduce_variable_identifier() = 0;
	virtual void reduce_identifier(const std::string& identifier_name) = 0;
	virtual void reduce_let_identifier(const std::string& identifier_name) = 0;
	virtual void reduce_modified_identifier(const std::string& identifier_name) = 0;
	virtual void reduce_let_modified_identifier(const std::string& identifier_name) = 0;
	virtual std::string reduce_constant_value(const std::string& value) = 0;
	virtual std::string reduce_regular_expression(const std::string& pattern) = 0;
	virtual std::string reduce_regular_expression_with_symbol(const std::string& pattern, const std::string& flags) = 0;
	virtual std::string reduce_string_literal(const std::string& value) = 0;
	virtual std::string reduce_number_literal(const std::string& value) = 0;
	virtual std::string reduce_regular_expression_start(const std::string& opening_delimiter) = 0;
	virtual std::string reduce_regular_expression_end(const std::string& opening_delimiter,
	    const std::string& closing_delimiter) = 0;
	virtual void reduce_default_modifier() = 0;
	virtual void reduce_const_address_modifier() = 0;
	virtual void reduce_const_value_modifier() = 0;
	virtual void reduce_const_modifier() = 0;
	virtual void reduce_global_modifier() = 0;
	virtual void reduce_final_modifier() = 0;
	virtual void reduce_override_modifier() = 0;
	virtual void reduce_public_modifier() = 0;
	virtual void reduce_protected_modifier() = 0;
	virtual void reduce_private_modifier() = 0;
	virtual void reduce_package_modifier() = 0;
	virtual void reduce_add_default_modifier() = 0;
	virtual void reduce_add_const_address_modifier() = 0;
	virtual void reduce_add_const_value_modifier() = 0;
	virtual void reduce_add_const_modifier() = 0;
	virtual void reduce_add_global_modifier() = 0;
	virtual void reduce_add_final_modifier() = 0;
	virtual void reduce_add_override_modifier() = 0;
	virtual void reduce_add_public_modifier() = 0;
	virtual void reduce_add_protected_modifier() = 0;
	virtual void reduce_add_private_modifier() = 0;
	virtual void reduce_add_package_modifier() = 0;
	virtual void reduce_statement_separator() = 0;
	virtual void reduce_empty_line() = 0;
	virtual void reduce_add_empty_line() = 0;

protected:
	virtual void commit_line_impl() = 0;
};

}

#endif // MINT_COMPILER_BUILD_CONTEXT_H
