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
#include <string>
#include <stack>
#include <utility>
#include <vector>

namespace mint {

class Branch;
class Compiler;
class MainBranch;

struct BreakableBlock;
struct CaseTable;
struct ConditionContext;
struct Context;
struct ContinuableBlock;
struct FunctionContext;

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

	struct Call {
		std::vector<ParserData> parameters;
		std::size_t with_unpack = 0;
	};

	BuildContext(DataStream& stream, Compiler& compiler);
	BuildContext(BuildContext&&) = delete;
	BuildContext(const BuildContext& other) = delete;
	~BuildContext();

	BuildContext& operator=(BuildContext&&) = delete;
	BuildContext& operator=(const BuildContext& other) = delete;

	void commit_line();
	void commit_expr_result(const ParserData& data);
	void resolve_deferred_checks() const;

	void load_module(const std::string& module_path);

	[[nodiscard]] bool has_returned() const;
	[[nodiscard]] std::size_t fast_symbol_index(const std::string& name);
	ParserData create_fast_scoped_symbol(const std::string& name, TypeAnnotation type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData create_fast_symbol(const std::string& name, TypeAnnotation type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData load_fast_symbol(const std::string& name);

	void check_for_alias(const ParserData& alias, const ParserData& value);

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

	[[nodiscard]] GlobalDataDescription& global_data_description() const;
	[[nodiscard]] ModuleDescription& module_description() const;
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
	void set_exit_point(const ParserData& data);
	void set_generator();
	void set_variadic();

	ParserData save_method_signatures(const std::string& name, TypeAnnotation return_type,
	    Reference::Flags flags = Reference::default_flags);
	ParserData save_method_signatures(Class::Operator op, TypeAnnotation return_type,
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
	void add_to_call_unpacked(ParserData parameter);
	[[nodiscard]] Call resolve_call();

	void start_capture();
	void resolve_capture();
	void capture_as(const std::string& symbol, TypeAnnotation type);
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
	void resolve_condition(const ParserData& data);

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

	[[nodiscard]] ParserData check_function_call(const ParserData& function, const Call& call) const;
	[[nodiscard]] ParserData check_method_call(const ParserData& object, const std::string& member,
	    const Call& call) const;
	[[nodiscard]] ParserData check_operator_call(const ParserData& object, Class::Operator op, const Call& call) const;

	void check_expression(const ParserData& result);
	[[nodiscard]] ParserData check_init_iterator(const Call& call);
	[[nodiscard]] ParserData check_operation(UnaryOperation op, const ParserData& operand) const;
	[[nodiscard]] ParserData check_operation(BinaryOperation op, const ParserData& lhs, const ParserData& rhs) const;
	[[nodiscard]] ParserData check_operation(TernaryOperation op, const ParserData& first, const ParserData& second,
	    const ParserData& third) const;

	Compiler& compiler();

	[[nodiscard]] std::string read_regex();
	[[nodiscard]] std::pair<int, std::string> next_token();
	[[noreturn]] void parse_error(const std::string& error_msg) const;

private:
	[[noreturn]] void parse_error(const std::string& error_msg, const DataStream::Position& position) const;
	[[nodiscard]] std::pair<std::size_t, std::string> line_error(const DataStream::Position& position) const;

	void push_branch(Branch& branch);
	void pop_branch();

	[[nodiscard]] Context& current_context();
	[[nodiscard]] const Context& current_context() const;

	[[nodiscard]] FunctionContext* current_function_context();
	[[nodiscard]] const FunctionContext* current_function_context() const;

	[[nodiscard]] BreakableBlock* current_breakable_block();
	[[nodiscard]] const BreakableBlock* current_breakable_block() const;

	[[nodiscard]] ContinuableBlock* current_continuable_block();
	[[nodiscard]] const ContinuableBlock* current_continuable_block() const;

	[[nodiscard]] std::size_t find_fast_symbol_index(const Symbol& symbol) const;
	[[nodiscard]] std::pair<std::size_t, const VariableDescription*> create_fast_scoped_symbol_index(const Symbol* symbol,
	    TypeAnnotation type, Reference::Flags flags);
	[[nodiscard]] std::pair<std::size_t, const VariableDescription*> create_fast_symbol_index(const Symbol& symbol,
	    TypeAnnotation type, Reference::Flags flags);
	void reset_scoped_symbols(const std::vector<const Symbol*>& symbols);

	void save_function_signatures(FunctionContext* def);
	void save_function_parameters(FunctionContext* def);
	void resolve_function_description(const FunctionContext* def);
	const Reference* make_function(const FunctionContext* def);

	enum class DeferPolicy {
		enabled,
		disabled,
	};

	[[nodiscard]] ParserData locate_data(const SymbolPath& path) const;
	[[nodiscard]] ParserData locate_data(BuiltinTypeKind kind) const;
	[[nodiscard]] ParserData locate_data(const TypeAnnotation& type) const;

	void check_return_type(const TypeAnnotation& type, const ParserData& data,
	    const DataStream::Position& position) const;

	[[nodiscard]] ParserData check_operator_call(const ParserData& object, Class::Operator op, const Call& call,
	    const DataStream::Position& position, DeferPolicy defer_policy = DeferPolicy::enabled) const;
	[[nodiscard]] ParserData check_function_call(const ParserData& function, const Call& call,
	    const DataStream::Position& position, DeferPolicy defer_policy = DeferPolicy::enabled) const;
	[[nodiscard]] ParserData check_method_call(const ParserData& object, const Symbol& method, const Call& call,
	    const DataStream::Position& position, DeferPolicy defer_policy = DeferPolicy::enabled) const;
	[[nodiscard]] ParserData check_operation(UnaryOperation op, const ParserData& operand,
	    const DataStream::Position& position, DeferPolicy defer_policy = DeferPolicy::enabled) const;
	[[nodiscard]] ParserData check_operation(BinaryOperation op, const ParserData& lhs, const ParserData& rhs,
	    const DataStream::Position& position, DeferPolicy defer_policy = DeferPolicy::enabled) const;
	[[nodiscard]] ParserData check_operation(TernaryOperation op, const ParserData& first, const ParserData& second,
	    const ParserData& third, const DataStream::Position& position,
	    DeferPolicy defer_policy = DeferPolicy::enabled) const;

	[[nodiscard]] ParserData find_symbol(const Symbol& symbol, std::size_t index) const;
	[[nodiscard]] FunctionDefinition find_method(const ParserData& member) const;
	[[nodiscard]] ParserData find_member(const TypeAnnotation& type, const Symbol& symbol,
	    DeferPolicy defer_policy = DeferPolicy::enabled) const;
	[[nodiscard]] ParserData find_member(const ParserData& object, const Symbol& symbol,
	    DeferPolicy defer_policy = DeferPolicy::enabled) const;
	[[nodiscard]] Reference::Flags member_flags(const ParserData& object, const Symbol& symbol) const;
	[[nodiscard]] const FunctionDescription& find_overload(const FunctionDefinition& functions,
	    std::size_t parameter_count, const DataStream::Position& position) const;
	[[nodiscard]] FunctionDefinition find_overloads(const FunctionDefinition& functions,
	    std::size_t min_parameter_count, const DataStream::Position& position) const;
	void try_extract_type_narrowing(ConditionContext& condition, const ParserData& lhs, const ParserData& rhs) const;

	[[nodiscard]] DeferredCheck defer_check(DeferredCheck::Definition definition) const;
	[[nodiscard]] ParserData resolve_deferred_check(const DeferredCheck& check) const;
	[[nodiscard]] ParserData resolve_deferred_data(const ParserData& data) const;
	[[nodiscard]] bool match_type(const TypeAnnotation& target, const TypeAnnotation& source) const;
	[[nodiscard]] bool can_cast(PrimitiveTypeKind kind, const TypeAnnotation& type) const;
	[[nodiscard]] bool can_cast(BuiltinTypeKind kind, const TypeAnnotation& type) const;

	std::reference_wrapper<Compiler> _compiler;
	std::reference_wrapper<DataStream> _stream;
	Lexer _lexer;

	std::unique_ptr<Context> _module;
	std::unique_ptr<MainBranch> _main_branch;
	std::reference_wrapper<Branch> _branch;

	std::stack<std::reference_wrapper<PackageDescription>, std::vector<std::reference_wrapper<PackageDescription>>>
	    _packages;
	std::stack<std::unique_ptr<FunctionContext>, std::vector<std::unique_ptr<FunctionContext>>> _functions;
	std::stack<std::reference_wrapper<Branch>, std::vector<std::reference_wrapper<Branch>>> _branches;
	std::stack<Call, std::vector<Call>> _calls;

	int _next_enum_value = 0;
	SymbolPath _class_base;
	std::stack<Class::Operator> _operators;
	std::stack<Reference::Flags> _modifiers;
	std::stack<TypeAnnotation> _annotations;

	std::vector<DeferredCheck> _deferred_checks;
};

}

#endif // MINT_COMPILER_BUILD_CONTEXT_H
