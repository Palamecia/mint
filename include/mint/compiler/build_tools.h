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

#ifndef MINT_COMPILER_BUILD_TOOLS_H
#define MINT_COMPILER_BUILD_TOOLS_H

#include "mint/compiler/abstract_syntax_tree.h"
#include "mint/compiler/build_context.h"
#include "mint/program/class_description.h"
#include "mint/program/module.h"
#include "mint/program/node.h"
#include "mint/program/symbol.h"
#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/data_stream.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <stack>
#include <vector>

namespace mint {

class Branch;
class Compiler;
class MainBranch;

struct Block;
struct Context;
struct CaseTable;
struct Definition;

constexpr inline std::size_t invalid_offset = std::numeric_limits<std::size_t>::max();
constexpr inline std::size_t invalid_index = std::numeric_limits<std::size_t>::max();

class MINT_EXPORT BytecodeBuildContext final : public BuildContext {
public:
	enum class BlockType : std::uint8_t {
		conditional_loop_type,
		custom_range_loop_type,
		range_loop_type,
		switch_type,
		if_type,
		elif_type,
		else_type,
		try_type,
		catch_type,
		print_type,
	};

	BytecodeBuildContext(DataStream& stream, Compiler& compiler, ModuleInfo& data);
	BytecodeBuildContext(BytecodeBuildContext&&) = delete;
	BytecodeBuildContext(const BytecodeBuildContext& other) = delete;
	~BytecodeBuildContext() override;

	BytecodeBuildContext& operator=(BytecodeBuildContext&&) = delete;
	BytecodeBuildContext& operator=(const BytecodeBuildContext& other) = delete;

	void commit_expr_result();

	void reduce_module_stmt_list() override;
	void reduce_load_statement(const std::string& module_path) override;
	void reduce_try_bloc() override;
	void reduce_catch_bloc() override;
	void reduce_if_bloc() override;
	void reduce_elif_bloc() override;
	void reduce_else_bloc() override;
	void reduce_switch_bloc() override;
	void reduce_while_bloc() override;
	void reduce_for_bloc() override;
	void reduce_break_statement() override;
	void reduce_continue_statement() override;
	void reduce_print_to_stream_statement() override;
	void reduce_print_statement() override;
	void reduce_print_bloc() override;
	void reduce_yield_generator_expression_statement() override;
	void reduce_yield_statement() override;
	void reduce_return_generator_expression_statement() override;
	void reduce_return_statement() override;
	void reduce_raise_statement() override;
	void reduce_raise_in_statement() override;
	void reduce_reraise_statement() override;
	void reduce_exit_with_code_statement() override;
	void reduce_exit_statement() override;
	void reduce_identifier_iterator_assignment_expression() override;
	void reduce_identifier_iterator_assignment_generator_expression() override;
	void reduce_create_identifier_iterator_assignment_expression() override;
	void reduce_create_identifier_iterator_assignment_generator_expression() override;
	void reduce_expression_statement() override;
	void reduce_function_definition_with_modifiers(const std::string& name) override;
	void reduce_function_definition(const std::string& name) override;
	void reduce_empty_statement() override;
	void reduce_package_declaration(const std::string& package_name) override;
	void reduce_package_block() override;
	void reduce_class_declaration_with_modifiers(const std::string& class_name) override;
	void reduce_class_declaration(const std::string& class_name) override;
	void reduce_parent_class() override;
	void reduce_add_parent_class() override;
	void reduce_parent_class_name(const std::string& parent_class_name) override;
	void reduce_qualified_parent_class_name(const std::string& parent_class_name) override;
	void reduce_class_definition() override;
	void reduce_nested_class_declaration(const std::string& class_name) override;
	void reduce_modified_nested_class_declaration(const std::string& class_name) override;
	void reduce_nested_class_definition() override;
	void reduce_nested_enum_declaration(const std::string& enum_name) override;
	void reduce_modified_nested_enum_declaration(const std::string& enum_name) override;
	void reduce_nested_enum_definition() override;
	void reduce_member_without_initializer(const std::string& member_name) override;
	void reduce_member_with_constant_initializer(const std::string& member_name,
	    const std::string& initializer) override;
	void reduce_member_with_string_initializer(const std::string& member_name, const std::string& initializer) override;
	void reduce_member_with_regex_initializer(const std::string& member_name, const std::string& pattern) override;
	void reduce_member_with_flagged_regex_initializer(const std::string& member_name, const std::string& pattern,
	    const std::string& flags) override;
	void reduce_member_with_number_initializer(const std::string& member_name, const std::string& initializer) override;
	void reduce_member_with_empty_array_initializer(const std::string& member_name) override;
	void reduce_member_with_empty_hash_initializer(const std::string& member_name) override;
	void reduce_member_with_library_initializer(const std::string& member_name,
	    const std::string& library_name) override;
	void reduce_member_function_definition(const std::string& function_name) override;
	void reduce_member_function_update(const std::string& function_name) override;
	void reduce_named_function_definition(const std::string& function_name) override;
	void reduce_async_function_definition(const std::string& function_name) override;
	void reduce_operator_function_definition() override;
	void reduce_modified_named_function_definition(const std::string& function_name) override;
	void reduce_modified_async_function_definition(const std::string& function_name) override;
	void reduce_modified_operator_function_definition() override;
	void reduce_empty_descriptor_line() override;
	void reduce_in_operator() override;
	void reduce_copy_operator() override;
	void reduce_or_operator() override;
	void reduce_and_operator() override;
	void reduce_bor_operator() override;
	void reduce_xor_operator() override;
	void reduce_band_operator() override;
	void reduce_eq_operator() override;
	void reduce_ne_operator() override;
	void reduce_lt_operator() override;
	void reduce_gt_operator() override;
	void reduce_le_operator() override;
	void reduce_ge_operator() override;
	void reduce_shift_left_operator() override;
	void reduce_shift_right_operator() override;
	void reduce_add_operator() override;
	void reduce_sub_operator() override;
	void reduce_mul_operator() override;
	void reduce_div_operator() override;
	void reduce_mod_operator() override;
	void reduce_not_operator() override;
	void reduce_compl_operator() override;
	void reduce_inc_operator() override;
	void reduce_dec_operator() override;
	void reduce_pow_operator() override;
	void reduce_inclusive_range_operator() override;
	void reduce_exclusive_range_operator() override;
	void reduce_call_operator() override;
	void reduce_subscript_operator() override;
	void reduce_subscript_move_operator() override;
	void reduce_modified_enum_declaration(const std::string& enum_name) override;
	void reduce_enum_declaration(const std::string& enum_name) override;
	void reduce_enum_definition() override;
	void reduce_enum_item_with_value(const std::string& item_name, const std::string& value) override;
	void reduce_enum_item_with_implicit_value(const std::string& item_name) override;
	void reduce_empty_enum_item() override;
	void reduce_conditional_generator_expression() override;
	void reduce_if_else_generator_expression() override;
	void reduce_if_elif_generator_expression() override;
	void reduce_if_elif_else_generator_expression() override;
	void reduce_switch_generator_expression() override;
	void reduce_while_generator_expression() override;
	void reduce_for_generator_expression() override;
	void reduce_try_keyword() override;
	void reduce_catch_clause(const std::string& exception_name) override;
	void reduce_try_block() override;
	void reduce_if_block() override;
	void reduce_if_generator_block() override;
	void reduce_elif_block() override;
	void reduce_add_elif_block() override;
	void reduce_elif_generator_block() override;
	void reduce_add_elif_generator_block() override;
	void reduce_yield_generator_block() override;
	void reduce_yield_value_block() override;
	void reduce_return_generator_block() override;
	void reduce_return_value_block() override;
	void reduce_raise_block() override;
	void reduce_reraise_block() override;
	void reduce_bare_raise_block() override;
	void reduce_expression_block() override;
	void reduce_generator_expression_body() override;
	void reduce_if_condition() override;
	void reduce_if_generator_condition() override;
	void reduce_elif_condition() override;
	void reduce_if_keyword() override;
	void reduce_generator_if_keyword() override;
	void reduce_elif_keyword() override;
	void reduce_else_keyword() override;
	void reduce_switch_condition() override;
	void reduce_generator_switch_condition() override;
	void reduce_generator_switch_keyword() override;
	void reduce_switch_keyword() override;
	void reduce_case_keyword() override;
	std::string reduce_case_symbol(const std::string& symbol_name) override;
	std::string reduce_qualified_case_symbol(const std::string& parent_symbol, const std::string& separator,
	    const std::string& symbol_name) override;
	std::string reduce_case_constant(const std::string& constant) override;
	std::string reduce_positive_case_number(const std::string& number) override;
	std::string reduce_negative_case_number(const std::string& sign, const std::string& number) override;
	std::string reduce_append_case_constant(const std::string& existing_constants, const std::string& constant,
	    const std::string& separator) override;
	std::string reduce_start_case_constant_list(const std::string& constant, const std::string& separator) override;
	std::string reduce_finish_case_constant_list(const std::string& constant) override;
	void reduce_empty_case_constant_list() override;
	void reduce_inclusive_case_range_label(const std::string& start_value, const std::string& range_operator,
	    const std::string& end_value) override;
	void reduce_exclusive_case_range_label(const std::string& start_value, const std::string& range_operator,
	    const std::string& end_value) override;
	void reduce_case_constant_list_label(const std::string& constants, const std::string& last_constant) override;
	void reduce_case_constant_membership_label(const std::string& constant) override;
	void reduce_case_symbol_membership_label(const std::string& symbol_name) override;
	void reduce_case_constant_identity_label(const std::string& constant) override;
	void reduce_case_symbol_identity_label(const std::string& symbol_name) override;
	void reduce_case_constant_label(const std::string& constant) override;
	void reduce_case_symbol_label(const std::string& symbol_name) override;
	void reduce_default_case_keyword() override;
	void reduce_empty_case_line() override;
	void reduce_case_expression_body() override;
	void reduce_add_case_expression_body() override;
	void reduce_default_expression_body() override;
	void reduce_add_default_expression_body() override;
	void reduce_while_condition() override;
	void reduce_generator_while_condition() override;
	void reduce_generator_while_keyword() override;
	void reduce_while_keyword() override;
	void reduce_range_for_condition() override;
	void reduce_iterator_for_condition() override;
	void reduce_for_in_condition() override;
	void reduce_generator_range_for_condition() override;
	void reduce_generator_iterator_for_condition() override;
	void reduce_generator_for_in_condition() override;
	void reduce_generator_for_keyword() override;
	void reduce_for_keyword() override;
	void reduce_generator_for_identifier() override;
	void reduce_for_identifier() override;
	void reduce_generator_for_iterator_identifier() override;
	void reduce_generator_for_created_iterator() override;
	void reduce_for_iterator_identifier() override;
	void reduce_for_created_iterator() override;
	void reduce_range_initial_value() override;
	void reduce_range_condition_value() override;
	void reduce_range_step_value() override;
	void reduce_return_keyword() override;
	void reduce_hash_literal_start() override;
	void reduce_hash_literal_end() override;
	void reduce_add_hash_entry() override;
	void reduce_hash_entry() override;
	void reduce_array_literal_start() override;
	void reduce_array_literal_end() override;
	void reduce_array_value() override;
	void reduce_array_spread() override;
	void reduce_array_unpack() override;
	void reduce_array_generator() override;
	void reduce_add_iterator_value() override;
	void reduce_iterator_value() override;
	void reduce_add_iterator_spread() override;
	void reduce_add_iterator_unpack() override;
	void reduce_iterator_spread() override;
	void reduce_iterator_unpack() override;
	void reduce_iterator_end_expression() override;
	void reduce_empty_iterator_end() override;
	void reduce_add_identifier_iterator_target() override;
	void reduce_identifier_iterator_target() override;
	void reduce_identifier_iterator_end() override;
	void reduce_empty_identifier_iterator_end() override;
	void reduce_let_modifier() override;
	void reduce_add_scoped_iterator_name(const std::string& iterator_name) override;
	void reduce_first_scoped_iterator_name(const std::string& iterator_name) override;
	void reduce_scoped_iterator_end_name(const std::string& iterator_name) override;
	void reduce_add_iterator_name(const std::string& iterator_name) override;
	void reduce_first_iterator_name(const std::string& iterator_name) override;
	void reduce_iterator_end_name(const std::string& iterator_name) override;
	void reduce_print_argument_separator() override;
	void reduce_print_block_target() override;
	void reduce_print_block_without_target() override;
	void reduce_assignment_from_generator_expression() override;
	void reduce_assignment_expression() override;
	void reduce_binding_from_generator_expression() override;
	void reduce_binding_expression() override;
	void begin_iterator_initialization_expression() override;
	void reduce_iterator_initialization_expression() override;
	void reduce_addition_expression() override;
	void reduce_subtraction_expression() override;
	void reduce_multiplication_expression() override;
	void reduce_division_expression() override;
	void reduce_modulo_expression() override;
	void reduce_exponentiation_expression() override;
	void reduce_identity_expression() override;
	void reduce_membership_expression() override;
	void reduce_negated_membership_expression() override;
	void reduce_equality_expression() override;
	void reduce_inequality_expression() override;
	void reduce_less_than_expression() override;
	void reduce_greater_than_expression() override;
	void reduce_less_than_or_equal_expression() override;
	void reduce_greater_than_or_equal_expression() override;
	void reduce_left_shift_expression() override;
	void reduce_right_shift_expression() override;
	void reduce_inclusive_range_expression() override;
	void reduce_exclusive_range_expression() override;
	void reduce_prefix_increment_expression() override;
	void reduce_prefix_decrement_expression() override;
	void reduce_postfix_increment_expression() override;
	void reduce_postfix_decrement_expression() override;
	void reduce_logical_not_expression() override;
	void begin_logical_or_expression() override;
	void reduce_logical_or_expression() override;
	void begin_logical_and_expression() override;
	void reduce_logical_and_expression() override;
	void reduce_bitwise_or_expression() override;
	void reduce_bitwise_and_expression() override;
	void reduce_bitwise_xor_expression() override;
	void reduce_bitwise_not_expression() override;
	void reduce_unary_plus_expression() override;
	void reduce_unary_minus_expression() override;
	void reduce_await_expression() override;
	void reduce_typeof_expression() override;
	void reduce_membersof_expression() override;
	void reduce_defined_expression() override;
	void reduce_slice_assignment_expression() override;
	void begin_addition_assignment_expression() override;
	void reduce_addition_assignment_expression() override;
	void begin_subtraction_assignment_expression() override;
	void reduce_subtraction_assignment_expression() override;
	void begin_multiplication_assignment_expression() override;
	void reduce_multiplication_assignment_expression() override;
	void begin_division_assignment_expression() override;
	void reduce_division_assignment_expression() override;
	void begin_modulo_assignment_expression() override;
	void reduce_modulo_assignment_expression() override;
	void begin_left_shift_assignment_expression() override;
	void reduce_left_shift_assignment_expression() override;
	void begin_right_shift_assignment_expression() override;
	void reduce_right_shift_assignment_expression() override;
	void begin_bitwise_and_assignment_expression() override;
	void reduce_bitwise_and_assignment_expression() override;
	void begin_bitwise_or_assignment_expression() override;
	void reduce_bitwise_or_assignment_expression() override;
	void begin_bitwise_xor_assignment_expression() override;
	void reduce_bitwise_xor_assignment_expression() override;
	void reduce_regex_match_expression() override;
	void reduce_regex_non_match_expression() override;
	void reduce_strict_equality_expression() override;
	void reduce_strict_inequality_expression() override;
	void begin_conditional_expression() override;
	void continue_conditional_expression() override;
	void reduce_conditional_expression() override;
	void reduce_empty_parenthesized_expression() override;
	void reduce_subscript_expression() override;
	void reduce_defined_member_call_arguments() override;
	void reduce_call_arguments_start() override;
	void reduce_call_arguments_end() override;
	void reduce_member_call_start(const std::string& member_name) override;
	void reduce_operator_member_call_start() override;
	void reduce_variable_member_call_start() override;
	void reduce_defined_member_call_start(const std::string& member_name) override;
	void reduce_defined_operator_member_call_start() override;
	void reduce_defined_variable_member_call_start() override;
	void reduce_member_call_arguments_end() override;
	void reduce_positional_call_argument() override;
	void reduce_callable_call_argument() override;
	void reduce_generator_call_argument() override;
	void reduce_spread_call_argument() override;
	void reduce_unpack_call_argument() override;
	void reduce_function_definition_with_arguments() override;
	void reduce_function_definition_without_arguments() override;
	void reduce_arrow_function_definition() override;
	void reduce_function_definition_start() override;
	void reduce_async_function_definition_start() override;
	void reduce_capture_list_start() override;
	void reduce_capture_list_end() override;
	void reduce_capture_with_initializer(const std::string& capture_name) override;
	void reduce_final_capture_with_initializer(const std::string& capture_name) override;
	void reduce_capture(const std::string& capture_name) override;
	void reduce_final_capture(const std::string& capture_name) override;
	void reduce_capture_all() override;
	void reduce_no_function_arguments() override;
	void reduce_function_arguments_end() override;
	void reduce_function_argument(const std::string& argument_name) override;
	void reduce_function_argument_with_default(const std::string& argument_name) override;
	void reduce_modified_function_argument(const std::string& argument_name) override;
	void reduce_modified_function_argument_with_default(const std::string& argument_name) override;
	void reduce_function_argument_unpack() override;
	void reduce_arrow_function_body_start() override;
	void reduce_member_access(const std::string& member_name) override;
	void reduce_operator_member_access() override;
	void reduce_variable_member_access() override;
	void reduce_optional_member_access(const std::string& member_name) override;
	void reduce_optional_operator_member_access() override;
	void reduce_optional_variable_member_access() override;
	void reduce_defined_symbol(const std::string& symbol_name) override;
	void reduce_qualified_defined_symbol(const std::string& symbol_name) override;
	void reduce_defined_variable_symbol() override;
	void reduce_qualified_defined_variable_symbol() override;
	void reduce_defined_constant_symbol(const std::string& constant) override;
	void reduce_constant_identifier(const std::string& constant) override;
	void reduce_library_identifier() override;
	void reduce_variable_identifier() override;
	void reduce_identifier(const std::string& identifier_name) override;
	void reduce_let_identifier(const std::string& identifier_name) override;
	void reduce_modified_identifier(const std::string& identifier_name) override;
	void reduce_let_modified_identifier(const std::string& identifier_name) override;
	std::string reduce_constant_value(const std::string& value) override;
	std::string reduce_regular_expression(const std::string& pattern) override;
	std::string reduce_regular_expression_with_symbol(const std::string& pattern, const std::string& flags) override;
	std::string reduce_string_literal(const std::string& value) override;
	std::string reduce_number_literal(const std::string& value) override;
	std::string reduce_regular_expression_start(const std::string& opening_delimiter) override;
	std::string reduce_regular_expression_end(const std::string& opening_delimiter,
	    const std::string& closing_delimiter) override;

	void reduce_default_modifier() override;
	void reduce_const_address_modifier() override;
	void reduce_const_value_modifier() override;
	void reduce_const_modifier() override;
	void reduce_global_modifier() override;
	void reduce_final_modifier() override;
	void reduce_override_modifier() override;
	void reduce_public_modifier() override;
	void reduce_protected_modifier() override;
	void reduce_private_modifier() override;
	void reduce_package_modifier() override;
	void reduce_add_default_modifier() override;
	void reduce_add_const_address_modifier() override;
	void reduce_add_const_value_modifier() override;
	void reduce_add_const_modifier() override;
	void reduce_add_global_modifier() override;
	void reduce_add_final_modifier() override;
	void reduce_add_override_modifier() override;
	void reduce_add_public_modifier() override;
	void reduce_add_protected_modifier() override;
	void reduce_add_private_modifier() override;
	void reduce_add_package_modifier() override;

	void reduce_statement_separator() override;
	void reduce_empty_line() override;
	void reduce_add_empty_line() override;

	[[nodiscard]] std::size_t create_fast_scoped_symbol_index(const std::string& symbol);
	[[nodiscard]] std::size_t create_fast_symbol_index(const std::string& symbol);
	[[nodiscard]] std::size_t fast_symbol_index(const std::string& symbol);
	[[nodiscard]] bool has_returned() const;

	void open_block(BlockType type);
	void reset_scoped_symbols();
	void reset_scoped_symbols_until(BlockType type);
	void close_block();

	[[nodiscard]] bool is_in_catch() const;
	[[nodiscard]] bool is_in_loop() const;
	[[nodiscard]] bool is_in_switch() const;
	[[nodiscard]] bool is_in_range_loop() const;
	[[nodiscard]] bool is_in_function() const;
	[[nodiscard]] bool is_in_nested_function() const;
	[[nodiscard]] bool is_in_async_function() const;
	[[nodiscard]] bool is_in_generator() const;
	[[nodiscard]] bool is_in_generator_expression() const;

	void prepare_continue();
	void prepare_break();
	void prepare_return();

	void register_retrieve_point();
	void unregister_retrieve_point();

	void set_exception_symbol(const std::string& symbol);
	void reset_exception();

	void start_case_label();
	void resolve_case_label(const std::string& label);
	void set_default_label();
	void build_case_table();

	void start_jump_forward();
	void bloc_jump_forward();
	void shift_jump_forward();
	void resolve_jump_forward();

	void start_jump_backward();
	void bloc_jump_backward();
	void shift_jump_backward();
	void resolve_jump_backward();

	void start_definition();
	void start_async_definition();
	void add_parameter(const std::string& symbol, Reference::Flags flags = Reference::default_flags);
	void set_variadic();
	void set_generator();
	void set_exit_point();
	void save_parameters();
	void add_definition_signature();
	void save_definition(std::string name);
	Function& retrieve_definition(std::string name);

	[[nodiscard]] PackageData& current_package() const;
	void open_package(const std::string& name);
	void close_package();

	void start_class_description(const std::string& name, Reference::Flags flags);
	void append_symbol_to_base_class_path(const std::string& symbol);
	void save_base_class_path();
	void create_member(Reference::Flags flags, const Symbol& symbol, Data* value);
	void create_member(Reference::Flags flags, const Symbol& symbol, Data& value);
	void update_member(Reference::Flags flags, const Symbol& symbol, Data& value);
	void resolve_class_description();

	void start_enum_description(const std::string& name, Reference::Flags flags);
	void set_current_enum_value(int value);
	int next_enum_value();
	void resolve_enum_description();

	void start_call();
	void add_to_call();
	void resolve_call();

	void start_capture();
	void resolve_capture();
	void capture_as(const std::string& symbol);
	void capture(const std::string& symbol);
	void capture_all();

	void open_generator_expression();
	void close_generator_expression();

	void open_printer();
	void close_printer();
	void force_printer();

	void start_range_loop();
	void resolve_range_loop();

	void start_condition();
	void resolve_condition();

	void open_sub_branch();
	void close_sub_branch();
	void build_sub_branch();

	void push_node(Node::Command command);
	void push_node(int parameter);
	void push_node(std::size_t parameter);
	void push_node(const char* symbol);
	void push_node(Data& constant);
	void push_node(ClassDescription* desc);
	[[nodiscard]] std::size_t next_offset() const;

	void start_operator(Class::Operator op);
	Class::Operator retrieve_operator();
	Symbol retrieve_operator_symbol();

	void start_modifiers(Reference::Flags flags);
	void add_modifiers(Reference::Flags flags);
	[[nodiscard]] Reference::Flags get_modifiers() const;
	Reference::Flags retrieve_modifiers();

	Compiler& compiler();
protected:
	void commit_line_impl() override;

	void push_node(const Reference* constant);
	void push_node(const Symbol* symbol);

	void push_branch(Branch& branch);
	void pop_branch();

	struct Call {
		int argc = 0;
	};

	Block* current_breakable_block();
	[[nodiscard]] const Block* current_breakable_block() const;

	Block* current_continuable_block();
	[[nodiscard]] const Block* current_continuable_block() const;

	Context& current_context();
	[[nodiscard]] const Context& current_context() const;

	Definition* current_definition();
	[[nodiscard]] const Definition* current_definition() const;

	[[nodiscard]] std::size_t find_fast_symbol_index(const Symbol& symbol) const;
	void reset_scoped_symbols(const std::vector<const Symbol*>& symbols);

private:
	std::reference_wrapper<ModuleInfo> _data;
	std::reference_wrapper<Compiler> _compiler;
	std::unique_ptr<Context> _module_context;
	std::unique_ptr<MainBranch> _main_branch;
	std::reference_wrapper<Branch> _branch;

	std::stack<std::reference_wrapper<PackageData>, std::vector<std::reference_wrapper<PackageData>>> _packages;
	std::stack<std::unique_ptr<Definition>, std::vector<std::unique_ptr<Definition>>> _definitions;
	std::stack<std::reference_wrapper<Branch>, std::vector<std::reference_wrapper<Branch>>> _branches;
	std::stack<std::unique_ptr<Call>, std::vector<std::unique_ptr<Call>>> _calls;

	int _next_enum_value = 0;
	ClassDescription::Path _class_base;
	std::stack<Class::Operator> _operators;
	std::stack<Reference::Flags> _modifiers;
};

class MINT_EXPORT AbstractSyntaxTreeBuildContext final : public BuildContext {
public:
	AbstractSyntaxTreeBuildContext(DataStream& stream, AbstractSyntaxTree& tree);
	AbstractSyntaxTreeBuildContext(AbstractSyntaxTreeBuildContext&&) = delete;
	AbstractSyntaxTreeBuildContext(const AbstractSyntaxTreeBuildContext&) = delete;
	~AbstractSyntaxTreeBuildContext() override = default;

	AbstractSyntaxTreeBuildContext& operator=(AbstractSyntaxTreeBuildContext&&) = delete;
	AbstractSyntaxTreeBuildContext& operator=(const AbstractSyntaxTreeBuildContext&) = delete;

protected:
	void commit_line_impl() override;

public:
	void reduce_module_stmt_list() override;
	void reduce_load_statement(const std::string& module_path) override;
	void reduce_try_bloc() override;
	void reduce_catch_bloc() override;
	void reduce_if_bloc() override;
	void reduce_elif_bloc() override;
	void reduce_else_bloc() override;
	void reduce_switch_bloc() override;
	void reduce_while_bloc() override;
	void reduce_for_bloc() override;
	void reduce_break_statement() override;
	void reduce_continue_statement() override;
	void reduce_print_to_stream_statement() override;
	void reduce_print_statement() override;
	void reduce_print_bloc() override;
	void reduce_yield_generator_expression_statement() override;
	void reduce_yield_statement() override;
	void reduce_return_generator_expression_statement() override;
	void reduce_return_statement() override;
	void reduce_raise_statement() override;
	void reduce_raise_in_statement() override;
	void reduce_reraise_statement() override;
	void reduce_exit_with_code_statement() override;
	void reduce_exit_statement() override;
	void reduce_identifier_iterator_assignment_expression() override;
	void reduce_identifier_iterator_assignment_generator_expression() override;
	void reduce_create_identifier_iterator_assignment_expression() override;
	void reduce_create_identifier_iterator_assignment_generator_expression() override;
	void reduce_expression_statement() override;
	void reduce_function_definition_with_modifiers(const std::string& name) override;
	void reduce_function_definition(const std::string& name) override;
	void reduce_empty_statement() override;
	void reduce_package_declaration(const std::string& package_name) override;
	void reduce_package_block() override;
	void reduce_class_declaration_with_modifiers(const std::string& class_name) override;
	void reduce_class_declaration(const std::string& class_name) override;
	void reduce_parent_class() override;
	void reduce_add_parent_class() override;
	void reduce_parent_class_name(const std::string& parent_class_name) override;
	void reduce_qualified_parent_class_name(const std::string& parent_class_name) override;
	void reduce_class_definition() override;
	void reduce_nested_class_declaration(const std::string& class_name) override;
	void reduce_modified_nested_class_declaration(const std::string& class_name) override;
	void reduce_nested_class_definition() override;
	void reduce_nested_enum_declaration(const std::string& enum_name) override;
	void reduce_modified_nested_enum_declaration(const std::string& enum_name) override;
	void reduce_nested_enum_definition() override;
	void reduce_member_without_initializer(const std::string& member_name) override;
	void reduce_member_with_constant_initializer(const std::string& member_name,
	    const std::string& initializer) override;
	void reduce_member_with_string_initializer(const std::string& member_name, const std::string& initializer) override;
	void reduce_member_with_regex_initializer(const std::string& member_name, const std::string& pattern) override;
	void reduce_member_with_flagged_regex_initializer(const std::string& member_name, const std::string& pattern,
	    const std::string& flags) override;
	void reduce_member_with_number_initializer(const std::string& member_name, const std::string& initializer) override;
	void reduce_member_with_empty_array_initializer(const std::string& member_name) override;
	void reduce_member_with_empty_hash_initializer(const std::string& member_name) override;
	void reduce_member_with_library_initializer(const std::string& member_name,
	    const std::string& library_name) override;
	void reduce_member_function_definition(const std::string& function_name) override;
	void reduce_member_function_update(const std::string& function_name) override;
	void reduce_named_function_definition(const std::string& function_name) override;
	void reduce_async_function_definition(const std::string& function_name) override;
	void reduce_operator_function_definition() override;
	void reduce_modified_named_function_definition(const std::string& function_name) override;
	void reduce_modified_async_function_definition(const std::string& function_name) override;
	void reduce_modified_operator_function_definition() override;
	void reduce_empty_descriptor_line() override;
	void reduce_in_operator() override;
	void reduce_copy_operator() override;
	void reduce_or_operator() override;
	void reduce_and_operator() override;
	void reduce_bor_operator() override;
	void reduce_xor_operator() override;
	void reduce_band_operator() override;
	void reduce_eq_operator() override;
	void reduce_ne_operator() override;
	void reduce_lt_operator() override;
	void reduce_gt_operator() override;
	void reduce_le_operator() override;
	void reduce_ge_operator() override;
	void reduce_shift_left_operator() override;
	void reduce_shift_right_operator() override;
	void reduce_add_operator() override;
	void reduce_sub_operator() override;
	void reduce_mul_operator() override;
	void reduce_div_operator() override;
	void reduce_mod_operator() override;
	void reduce_not_operator() override;
	void reduce_compl_operator() override;
	void reduce_inc_operator() override;
	void reduce_dec_operator() override;
	void reduce_pow_operator() override;
	void reduce_inclusive_range_operator() override;
	void reduce_exclusive_range_operator() override;
	void reduce_call_operator() override;
	void reduce_subscript_operator() override;
	void reduce_subscript_move_operator() override;
	void reduce_modified_enum_declaration(const std::string& enum_name) override;
	void reduce_enum_declaration(const std::string& enum_name) override;
	void reduce_enum_definition() override;
	void reduce_enum_item_with_value(const std::string& item_name, const std::string& value) override;
	void reduce_enum_item_with_implicit_value(const std::string& item_name) override;
	void reduce_empty_enum_item() override;
	void reduce_conditional_generator_expression() override;
	void reduce_if_else_generator_expression() override;
	void reduce_if_elif_generator_expression() override;
	void reduce_if_elif_else_generator_expression() override;
	void reduce_switch_generator_expression() override;
	void reduce_while_generator_expression() override;
	void reduce_for_generator_expression() override;
	void reduce_try_keyword() override;
	void reduce_catch_clause(const std::string& exception_name) override;
	void reduce_try_block() override;
	void reduce_if_block() override;
	void reduce_if_generator_block() override;
	void reduce_elif_block() override;
	void reduce_add_elif_block() override;
	void reduce_elif_generator_block() override;
	void reduce_add_elif_generator_block() override;
	void reduce_yield_generator_block() override;
	void reduce_yield_value_block() override;
	void reduce_return_generator_block() override;
	void reduce_return_value_block() override;
	void reduce_raise_block() override;
	void reduce_reraise_block() override;
	void reduce_bare_raise_block() override;
	void reduce_expression_block() override;
	void reduce_generator_expression_body() override;
	void reduce_if_condition() override;
	void reduce_if_generator_condition() override;
	void reduce_elif_condition() override;
	void reduce_if_keyword() override;
	void reduce_generator_if_keyword() override;
	void reduce_elif_keyword() override;
	void reduce_else_keyword() override;
	void reduce_switch_condition() override;
	void reduce_generator_switch_condition() override;
	void reduce_generator_switch_keyword() override;
	void reduce_switch_keyword() override;
	void reduce_case_keyword() override;
	std::string reduce_case_symbol(const std::string& symbol_name) override;
	std::string reduce_qualified_case_symbol(const std::string& parent_symbol, const std::string& separator,
	    const std::string& symbol_name) override;
	std::string reduce_case_constant(const std::string& constant) override;
	std::string reduce_positive_case_number(const std::string& number) override;
	std::string reduce_negative_case_number(const std::string& sign, const std::string& number) override;
	std::string reduce_append_case_constant(const std::string& existing_constants, const std::string& constant,
	    const std::string& separator) override;
	std::string reduce_start_case_constant_list(const std::string& constant, const std::string& separator) override;
	std::string reduce_finish_case_constant_list(const std::string& constant) override;
	void reduce_empty_case_constant_list() override;
	void reduce_inclusive_case_range_label(const std::string& start_value, const std::string& range_operator,
	    const std::string& end_value) override;
	void reduce_exclusive_case_range_label(const std::string& start_value, const std::string& range_operator,
	    const std::string& end_value) override;
	void reduce_case_constant_list_label(const std::string& constants, const std::string& last_constant) override;
	void reduce_case_constant_membership_label(const std::string& constant) override;
	void reduce_case_symbol_membership_label(const std::string& symbol_name) override;
	void reduce_case_constant_identity_label(const std::string& constant) override;
	void reduce_case_symbol_identity_label(const std::string& symbol_name) override;
	void reduce_case_constant_label(const std::string& constant) override;
	void reduce_case_symbol_label(const std::string& symbol_name) override;
	void reduce_default_case_keyword() override;
	void reduce_empty_case_line() override;
	void reduce_case_expression_body() override;
	void reduce_add_case_expression_body() override;
	void reduce_default_expression_body() override;
	void reduce_add_default_expression_body() override;
	void reduce_while_condition() override;
	void reduce_generator_while_condition() override;
	void reduce_generator_while_keyword() override;
	void reduce_while_keyword() override;
	void reduce_range_for_condition() override;
	void reduce_iterator_for_condition() override;
	void reduce_for_in_condition() override;
	void reduce_generator_range_for_condition() override;
	void reduce_generator_iterator_for_condition() override;
	void reduce_generator_for_in_condition() override;
	void reduce_generator_for_keyword() override;
	void reduce_for_keyword() override;
	void reduce_generator_for_identifier() override;
	void reduce_for_identifier() override;
	void reduce_generator_for_iterator_identifier() override;
	void reduce_generator_for_created_iterator() override;
	void reduce_for_iterator_identifier() override;
	void reduce_for_created_iterator() override;
	void reduce_range_initial_value() override;
	void reduce_range_condition_value() override;
	void reduce_range_step_value() override;
	void reduce_return_keyword() override;
	void reduce_hash_literal_start() override;
	void reduce_hash_literal_end() override;
	void reduce_add_hash_entry() override;
	void reduce_hash_entry() override;
	void reduce_array_literal_start() override;
	void reduce_array_literal_end() override;
	void reduce_array_value() override;
	void reduce_array_spread() override;
	void reduce_array_unpack() override;
	void reduce_array_generator() override;
	void reduce_add_iterator_value() override;
	void reduce_iterator_value() override;
	void reduce_add_iterator_spread() override;
	void reduce_add_iterator_unpack() override;
	void reduce_iterator_spread() override;
	void reduce_iterator_unpack() override;
	void reduce_iterator_end_expression() override;
	void reduce_empty_iterator_end() override;
	void reduce_add_identifier_iterator_target() override;
	void reduce_identifier_iterator_target() override;
	void reduce_identifier_iterator_end() override;
	void reduce_empty_identifier_iterator_end() override;
	void reduce_let_modifier() override;
	void reduce_add_scoped_iterator_name(const std::string& iterator_name) override;
	void reduce_first_scoped_iterator_name(const std::string& iterator_name) override;
	void reduce_scoped_iterator_end_name(const std::string& iterator_name) override;
	void reduce_add_iterator_name(const std::string& iterator_name) override;
	void reduce_first_iterator_name(const std::string& iterator_name) override;
	void reduce_iterator_end_name(const std::string& iterator_name) override;
	void reduce_print_argument_separator() override;
	void reduce_print_block_target() override;
	void reduce_print_block_without_target() override;
	void reduce_assignment_from_generator_expression() override;
	void reduce_assignment_expression() override;
	void reduce_binding_from_generator_expression() override;
	void reduce_binding_expression() override;
	void begin_iterator_initialization_expression() override;
	void reduce_iterator_initialization_expression() override;
	void reduce_addition_expression() override;
	void reduce_subtraction_expression() override;
	void reduce_multiplication_expression() override;
	void reduce_division_expression() override;
	void reduce_modulo_expression() override;
	void reduce_exponentiation_expression() override;
	void reduce_identity_expression() override;
	void reduce_membership_expression() override;
	void reduce_negated_membership_expression() override;
	void reduce_equality_expression() override;
	void reduce_inequality_expression() override;
	void reduce_less_than_expression() override;
	void reduce_greater_than_expression() override;
	void reduce_less_than_or_equal_expression() override;
	void reduce_greater_than_or_equal_expression() override;
	void reduce_left_shift_expression() override;
	void reduce_right_shift_expression() override;
	void reduce_inclusive_range_expression() override;
	void reduce_exclusive_range_expression() override;
	void reduce_prefix_increment_expression() override;
	void reduce_prefix_decrement_expression() override;
	void reduce_postfix_increment_expression() override;
	void reduce_postfix_decrement_expression() override;
	void reduce_logical_not_expression() override;
	void begin_logical_or_expression() override;
	void reduce_logical_or_expression() override;
	void begin_logical_and_expression() override;
	void reduce_logical_and_expression() override;
	void reduce_bitwise_or_expression() override;
	void reduce_bitwise_and_expression() override;
	void reduce_bitwise_xor_expression() override;
	void reduce_bitwise_not_expression() override;
	void reduce_unary_plus_expression() override;
	void reduce_unary_minus_expression() override;
	void reduce_await_expression() override;
	void reduce_typeof_expression() override;
	void reduce_membersof_expression() override;
	void reduce_defined_expression() override;
	void reduce_slice_assignment_expression() override;
	void begin_addition_assignment_expression() override;
	void reduce_addition_assignment_expression() override;
	void begin_subtraction_assignment_expression() override;
	void reduce_subtraction_assignment_expression() override;
	void begin_multiplication_assignment_expression() override;
	void reduce_multiplication_assignment_expression() override;
	void begin_division_assignment_expression() override;
	void reduce_division_assignment_expression() override;
	void begin_modulo_assignment_expression() override;
	void reduce_modulo_assignment_expression() override;
	void begin_left_shift_assignment_expression() override;
	void reduce_left_shift_assignment_expression() override;
	void begin_right_shift_assignment_expression() override;
	void reduce_right_shift_assignment_expression() override;
	void begin_bitwise_and_assignment_expression() override;
	void reduce_bitwise_and_assignment_expression() override;
	void begin_bitwise_or_assignment_expression() override;
	void reduce_bitwise_or_assignment_expression() override;
	void begin_bitwise_xor_assignment_expression() override;
	void reduce_bitwise_xor_assignment_expression() override;
	void reduce_regex_match_expression() override;
	void reduce_regex_non_match_expression() override;
	void reduce_strict_equality_expression() override;
	void reduce_strict_inequality_expression() override;
	void begin_conditional_expression() override;
	void continue_conditional_expression() override;
	void reduce_conditional_expression() override;
	void reduce_empty_parenthesized_expression() override;
	void reduce_subscript_expression() override;
	void reduce_defined_member_call_arguments() override;
	void reduce_call_arguments_start() override;
	void reduce_call_arguments_end() override;
	void reduce_member_call_start(const std::string& member_name) override;
	void reduce_operator_member_call_start() override;
	void reduce_variable_member_call_start() override;
	void reduce_defined_member_call_start(const std::string& member_name) override;
	void reduce_defined_operator_member_call_start() override;
	void reduce_defined_variable_member_call_start() override;
	void reduce_member_call_arguments_end() override;
	void reduce_positional_call_argument() override;
	void reduce_callable_call_argument() override;
	void reduce_generator_call_argument() override;
	void reduce_spread_call_argument() override;
	void reduce_unpack_call_argument() override;
	void reduce_function_definition_with_arguments() override;
	void reduce_function_definition_without_arguments() override;
	void reduce_arrow_function_definition() override;
	void reduce_function_definition_start() override;
	void reduce_async_function_definition_start() override;
	void reduce_capture_list_start() override;
	void reduce_capture_list_end() override;
	void reduce_capture_with_initializer(const std::string& capture_name) override;
	void reduce_final_capture_with_initializer(const std::string& capture_name) override;
	void reduce_capture(const std::string& capture_name) override;
	void reduce_final_capture(const std::string& capture_name) override;
	void reduce_capture_all() override;
	void reduce_no_function_arguments() override;
	void reduce_function_arguments_end() override;
	void reduce_function_argument(const std::string& argument_name) override;
	void reduce_function_argument_with_default(const std::string& argument_name) override;
	void reduce_modified_function_argument(const std::string& argument_name) override;
	void reduce_modified_function_argument_with_default(const std::string& argument_name) override;
	void reduce_function_argument_unpack() override;
	void reduce_arrow_function_body_start() override;
	void reduce_member_access(const std::string& member_name) override;
	void reduce_operator_member_access() override;
	void reduce_variable_member_access() override;
	void reduce_optional_member_access(const std::string& member_name) override;
	void reduce_optional_operator_member_access() override;
	void reduce_optional_variable_member_access() override;
	void reduce_defined_symbol(const std::string& symbol_name) override;
	void reduce_qualified_defined_symbol(const std::string& symbol_name) override;
	void reduce_defined_variable_symbol() override;
	void reduce_qualified_defined_variable_symbol() override;
	void reduce_defined_constant_symbol(const std::string& constant) override;
	void reduce_constant_identifier(const std::string& constant) override;
	void reduce_library_identifier() override;
	void reduce_variable_identifier() override;
	void reduce_identifier(const std::string& identifier_name) override;
	void reduce_let_identifier(const std::string& identifier_name) override;
	void reduce_modified_identifier(const std::string& identifier_name) override;
	void reduce_let_modified_identifier(const std::string& identifier_name) override;
	std::string reduce_constant_value(const std::string& value) override;
	std::string reduce_regular_expression(const std::string& pattern) override;
	std::string reduce_regular_expression_with_symbol(const std::string& pattern, const std::string& flags) override;
	std::string reduce_string_literal(const std::string& value) override;
	std::string reduce_number_literal(const std::string& value) override;
	std::string reduce_regular_expression_start(const std::string& opening_delimiter) override;
	std::string reduce_regular_expression_end(const std::string& opening_delimiter,
	    const std::string& closing_delimiter) override;
	void reduce_default_modifier() override;
	void reduce_const_address_modifier() override;
	void reduce_const_value_modifier() override;
	void reduce_const_modifier() override;
	void reduce_global_modifier() override;
	void reduce_final_modifier() override;
	void reduce_override_modifier() override;
	void reduce_public_modifier() override;
	void reduce_protected_modifier() override;
	void reduce_private_modifier() override;
	void reduce_package_modifier() override;
	void reduce_add_default_modifier() override;
	void reduce_add_const_address_modifier() override;
	void reduce_add_const_value_modifier() override;
	void reduce_add_const_modifier() override;
	void reduce_add_global_modifier() override;
	void reduce_add_final_modifier() override;
	void reduce_add_override_modifier() override;
	void reduce_add_public_modifier() override;
	void reduce_add_protected_modifier() override;
	void reduce_add_private_modifier() override;
	void reduce_add_package_modifier() override;
	void reduce_statement_separator() override;
	void reduce_empty_line() override;
	void reduce_add_empty_line() override;
	[[nodiscard]] AbstractSyntaxTree& tree() const;

private:
	[[nodiscard]] std::unique_ptr<Expression> take_expression();
	void reduce_binary_expression(std::string op);
	void reduce_unary_expression(std::string op);
	void build_conditional_expression();
	void append_control_statement(ControlStatement::Kind kind);

	AbstractSyntaxTree& _tree;
	std::vector<std::unique_ptr<Expression>> _expressions;
	std::vector<std::unique_ptr<Statement>> _statements;
	std::vector<std::unique_ptr<CallExpression>> _calls;
};

}

#endif // MINT_COMPILER_BUILD_TOOLS_H
