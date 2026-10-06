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

#include "mint/compiler/descriptions.h"
#include "mint/compiler/lexer.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/program/module.h"
#include "mint/program/node.h"
#include "mint/program/symbol.h"
#include "mint/system/data_stream.h"
#include "parser_data.h"
#include "type_annotation.h"

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <stack>
#include <utility>
#include <vector>

namespace mint {

class Branch;
class Compiler;
class MainBranch;

struct Block;
struct CaseTable;
struct Context;
struct FunctionDefinition;

constexpr inline std::size_t invalid_offset = std::numeric_limits<std::size_t>::max();
constexpr inline std::size_t invalid_index = std::numeric_limits<std::size_t>::max();

class MINT_EXPORT BuildContext {
public:
	enum class BlockType {
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

	enum class UnaryOperation {
		inc_operator,
		dec_operator,
		not_operator,
		compl_operator,
		pos_operator,
		neg_operator,
		typeof_operator,
		membersof_operator,
		defined_operator,
		await_operator,
	};

	enum class BinaryOperation {
		move_operator,
		copy_operator,
		add_operator,
		sub_operator,
		mul_operator,
		div_operator,
		pow_operator,
		mod_operator,
		is_operator,
		in_operator,
		eq_operator,
		ne_operator,
		lt_operator,
		gt_operator,
		le_operator,
		ge_operator,
		and_operator,
		or_operator,
		band_operator,
		bor_operator,
		xor_operator,
		shift_left_operator,
		shift_right_operator,
		inclusive_range_operator,
		exclusive_range_operator,
		subscript_operator,
		regex_match_operator,
		regex_unmatch_operator,
		strict_eq_operator,
		strict_ne_operator,
	};

	enum class TernaryOperation {
		conditional_operator,
		subscript_move_operator,
	};

	struct Call {
		std::vector<ParserData> parameters;
	};

	BuildContext(DataStream& stream, Compiler& compiler);
	BuildContext(BuildContext&&) = delete;
	BuildContext(const BuildContext& other) = delete;
	~BuildContext();

	BuildContext& operator=(BuildContext&&) = delete;
	BuildContext& operator=(const BuildContext& other) = delete;

	void commit_line();
	void commit_expr_result();

	[[nodiscard]] bool has_returned() const;
	[[nodiscard]] std::size_t fast_symbol_index(const std::string& name);
	ParserData create_fast_scoped_symbol(const std::string& name, TypeAnnotation type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData create_fast_symbol(const std::string& name, TypeAnnotation type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData load_fast_symbol(const std::string& name);

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
	void add_to_case_label(const std::string& token);
	void resolve_case_label();
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

	[[nodiscard]] PackageDescription& current_package() const;
	void open_package(const std::string& name);
	void close_package();

	void start_class_description(const std::string& name, Reference::Flags flags);
	void append_symbol_to_base_class_path(const std::string& symbol);
	void save_base_class_path();

	void resolve_class_description();

	void create_attribute(const std::string& name, TypeAnnotation type, Reference::Flags flags, Data* value);

	void start_enum_description(const std::string& name, Reference::Flags flags);
	void set_current_enum_value(int value);
	int next_enum_value();

	void resolve_enum_description();

	void start_function_description();
	void start_async_function_description();

	void add_parameter(const std::string& name, TypeAnnotation type, Reference::Flags flags = Reference::default_flags);
	void set_variadic();
	void set_generator();
	void set_exit_point();

	ParserData save_method_signature(const std::string& name, TypeAnnotation return_type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData save_method_signature(Class::Operator op, TypeAnnotation return_type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData save_function_signatures(const std::string& name, TypeAnnotation return_type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData save_function_signatures(TypeAnnotation return_type, Reference::Flags flags = Reference::default_flags);
	void add_function_signature();

	void resolve_method_description();
	void resolve_function_description();
	void register_function_description();

	void start_call();
	void add_to_call(ParserData parameter);
	[[nodiscard]] Call resolve_call();

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
	void build_sub_branch();
	void close_sub_branch();

	void push_node(Node::Command command);
	void push_node(int parameter);
	void push_node(Reference::Flags parameter);
	void push_node(std::size_t parameter);
	void push_node(const Symbol* symbol);
	void push_node(const Symbol& symbol);
	void push_node(const Reference* constant);
	void push_node(Data* constant);
	void push_node(const ClassDescription* desc);
	[[nodiscard]] std::size_t next_offset() const;

	void start_operator(Class::Operator op);
	[[nodiscard]] Class::Operator get_operator() const;
	[[nodiscard]] Class::Operator retrieve_operator();

	void start_modifiers(Reference::Flags flags);
	void add_modifiers(Reference::Flags flags);
	[[nodiscard]] Reference::Flags get_modifiers() const;
	[[nodiscard]] Reference::Flags retrieve_modifiers();

	void start_type_annotation(TypeAnnotation annotation);
	void add_type_annotation(TypeAnnotation annotation);
	[[nodiscard]] TypeAnnotation get_type_annotation() const;
	[[nodiscard]] TypeAnnotation retrieve_type_annotation();

	[[nodiscard]] ParserData get_member(const ParserData& object, const std::string& member) const;
	[[nodiscard]] ParserData get_operator(const ParserData& object, Class::Operator op) const;

	[[nodiscard]] ParserData check_function_call(const ParserData& function, Call call);
	[[nodiscard]] ParserData check_method_call(const ParserData& method, Call call);

	[[nodiscard]] ParserData check_operation(UnaryOperation op, const ParserData& operand) const;
	[[nodiscard]] ParserData check_operation(BinaryOperation op, const ParserData& lhs, const ParserData& rhs) const;
	[[nodiscard]] ParserData check_operation(TernaryOperation op, const ParserData& first, const ParserData& second,
	    const ParserData& third) const;

	Compiler& compiler();

	[[nodiscard]] std::string read_regex();
	[[nodiscard]] std::pair<int, std::string> next_token();
	[[noreturn]] void parse_error(const std::string& error_msg) const;

private:
	void push_branch(Branch& branch);
	void pop_branch();

	[[nodiscard]] Block* current_breakable_block();
	[[nodiscard]] const Block* current_breakable_block() const;

	[[nodiscard]] Block* current_continuable_block();
	[[nodiscard]] const Block* current_continuable_block() const;

	[[nodiscard]] Context& current_context();
	[[nodiscard]] const Context& current_context() const;

	[[nodiscard]] FunctionDefinition* current_function_definition();
	[[nodiscard]] const FunctionDefinition* current_function_definition() const;

	[[nodiscard]] std::size_t find_fast_symbol_index(const Symbol& symbol) const;
	[[nodiscard]] std::pair<const Symbol*, std::size_t> create_fast_scoped_symbol_index(const std::string& name);
	[[nodiscard]] std::pair<const Symbol*, std::size_t> create_fast_symbol_index(const std::string& name);
	void reset_scoped_symbols(const std::vector<const Symbol*>& symbols);

	void save_function_signatures(FunctionDefinition* def);
	void resolve_function_description(const FunctionDefinition* def);
	const Reference* make_function(const FunctionDefinition* def);

	[[nodiscard]] std::optional<ParserData> find_member(const ParserData& object, const Symbol& symbol) const;
	[[nodiscard]] bool match_type(const TypeAnnotation& target, const TypeAnnotation& source) const;

	std::reference_wrapper<Compiler> _compiler;
	Lexer _lexer;

	std::unique_ptr<Context> _module;
	std::unique_ptr<MainBranch> _main_branch;
	std::reference_wrapper<Branch> _branch;

	std::stack<std::reference_wrapper<PackageDescription>, std::vector<std::reference_wrapper<PackageDescription>>>
	    _packages;
	std::stack<std::unique_ptr<FunctionDefinition>, std::vector<std::unique_ptr<FunctionDefinition>>> _functions;
	std::stack<std::reference_wrapper<Branch>, std::vector<std::reference_wrapper<Branch>>> _branches;
	std::stack<Call, std::vector<Call>> _calls;

	int _next_enum_value = 0;
	SymbolPath _class_base;
	std::stack<Class::Operator> _operators;
	std::stack<Reference::Flags> _modifiers;
	std::stack<TypeAnnotation> _annotations;
};

}

#endif // MINT_COMPILER_BUILD_CONTEXT_H
