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

#ifndef MINT_COMPILER_BUILD_TOOLS_H
#define MINT_COMPILER_BUILD_TOOLS_H

#include "mint/ast/class_register.h"
#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/lexer.h"
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

class MINT_EXPORT BuildContext {
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
		print_type
	};

	BuildContext(DataStream& stream, Compiler& compiler, ModuleInfo& data);
	BuildContext(BuildContext&&) = delete;
	BuildContext(const BuildContext& other) = delete;
	~BuildContext();

	BuildContext& operator=(BuildContext&&) = delete;
	BuildContext& operator=(const BuildContext& other) = delete;

	void commit_line();
	void commit_expr_result();

	[[nodiscard]] std::size_t create_fast_scoped_symbol_index(const std::string& symbol);
	[[nodiscard]] std::size_t create_fast_symbol_index(const std::string& symbol);
	[[nodiscard]] std::size_t fast_symbol_index(const std::string& symbol);
	[[nodiscard]] bool has_returned() const;

	void open_block(BlockType type);
	void reset_scoped_symbols();
	void reset_scoped_symbols_until(BlockType type);
	void close_block();

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
	bool add_parameter(const std::string& symbol, Reference::Flags flags = Reference::default_flags);
	bool set_variadic();
	void set_generator();
	void set_exit_point();
	bool save_parameters();
	bool add_definition_signature();
	void save_definition();
	Function& retrieve_definition();

	[[nodiscard]] PackageData& current_package() const;
	void open_package(const std::string& name);
	void close_package();

	void start_class_description(const std::string& name, Reference::Flags flags);
	void append_symbol_to_base_class_path(const std::string& symbol);
	void save_base_class_path();
	bool create_member(Reference::Flags flags, const Symbol& symbol, Data* value);
	bool create_member(Reference::Flags flags, const Symbol& symbol, Data& value);
	bool update_member(Reference::Flags flags, const Symbol& symbol, Data& value);
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
	bool capture_as(const std::string& symbol);
	bool capture(const std::string& symbol);
	bool capture_all();

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

	std::string read_regex();
	int next_token(std::string* token);
	[[noreturn]] void parse_error(const std::string& error_msg) const;

protected:
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
	Lexer _lexer;

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

}

#endif // MINT_COMPILER_BUILD_TOOLS_H
