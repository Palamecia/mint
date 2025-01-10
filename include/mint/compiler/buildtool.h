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

#ifndef MINT_BUILDTOOL_H
#define MINT_BUILDTOOL_H

#include "mint/compiler/lexer.h"
#include "mint/ast/classregister.h"
#include "mint/ast/module.h"

#include <memory>
#include <string>
#include <stack>

namespace mint {

class Branch;

struct Block;
struct Context;
struct CaseTable;
struct Definition;

class MINT_EXPORT BuildContext {
public:
	enum BlockType {
		CONDITIONAL_LOOP_TYPE,
		CUSTOM_RANGE_LOOP_TYPE,
		RANGE_LOOP_TYPE,
		SWITCH_TYPE,
		IF_TYPE,
		ELIF_TYPE,
		ELSE_TYPE,
		TRY_TYPE,
		CATCH_TYPE,
		PRINT_TYPE
	};

	BuildContext(DataStream *stream, const Module::Info &data);
	~BuildContext();

	BuildContext(const BuildContext &other) = delete;
	BuildContext &operator =(const BuildContext &other) = delete;

	Lexer lexer;
	Module::Info data;

	void commit_line();
	void commit_expr_result();

	int create_fast_scoped_symbol_index(const std::string &symbol);
	int create_fast_symbol_index(const std::string &symbol);
	int fast_symbol_index(const std::string &symbol);
	bool has_returned() const;

	void open_block(BlockType type);
	void reset_scoped_symbols();
	void reset_scoped_symbols_until(BlockType type);
	void close_block();

	bool is_in_loop() const;
	bool is_in_switch() const;
	bool is_in_range_loop() const;
	bool is_in_function() const;
	bool is_in_generator() const;

	void prepare_continue();
	void prepare_break();
	void prepare_return();

	void register_retrieve_point();
	void unregister_retrieve_point();

	void set_exception_symbol(const std::string &symbol);
	void reset_exception();

	void start_case_label();
	void resolve_case_label(const std::string &label);
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
	bool add_parameter(const std::string &symbol, Reference::Flags flags = Reference::DEFAULT);
	bool set_variadic();
	void set_generator();
	void set_exit_point();
	bool save_parameters();
	bool add_definition_signature();
	void save_definition();
	Data *retrieve_definition();

	PackageData *current_package() const;
	void open_package(const std::string &name);
	void close_package();

	void start_class_description(const std::string &name, Reference::Flags flags = Reference::DEFAULT);
	void append_symbol_to_base_class_path(const std::string &symbol);
	void save_base_class_path();
	bool create_member(Reference::Flags flags, Class::Operator op, Data *value = GarbageCollector::instance().alloc<None>());
	bool create_member(Reference::Flags flags, const Symbol &symbol, Data *value = GarbageCollector::instance().alloc<None>());
	bool update_member(Reference::Flags flags, Class::Operator op, Data *value = GarbageCollector::instance().alloc<None>());
	bool update_member(Reference::Flags flags, const Symbol &symbol, Data *value = GarbageCollector::instance().alloc<None>());
	void resolve_class_description();

	void start_enum_description(const std::string &name, Reference::Flags flags = Reference::DEFAULT);
	void set_current_enum_value(int value);
	int next_enum_value();
	void resolve_enum_description();

	void start_call();
	void add_to_call();
	void resolve_call();

	void start_capture();
	void resolve_capture();
	bool capture_as(const std::string &symbol);
	bool capture(const std::string &symbol);
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

	void push_node(Node::Command command);
	void push_node(int parameter);
	void push_node(const char *symbol);
	void push_node(Data *constant);

	void start_operator(Class::Operator op);
	Class::Operator retrieve_operator();

	void start_modifiers(Reference::Flags flags);
	void add_modifiers(Reference::Flags flags);
	Reference::Flags get_modifiers() const;
	Reference::Flags retrieve_modifiers();

	int next_token(std::string *token);
	[[noreturn]] void parse_error(const char *error_msg);

protected:
	void push_node(Reference *constant);
	void push_node(Symbol *symbol);

	void push_branch(Branch *branch);
	void pop_branch();

	struct Call {
		int argc = 0;
	};

	Block *current_breakable_block();
	const Block *current_breakable_block() const;

	Block *current_continuable_block();
	const Block *current_continuable_block() const;

	Context *current_context();
	const Context *current_context() const;

	Definition *current_definition();
	const Definition *current_definition() const;

	int find_fast_symbol_index(const Symbol *symbol) const;
	void reset_scoped_symbols(const std::vector<Symbol *> *symbols);

private:
	std::unique_ptr<Context> m_module_context;
	Branch *m_branch;

	std::stack<PackageData *, std::vector<PackageData *>> m_packages;
	std::stack<Definition *, std::vector<Definition *>> m_definitions;
	std::stack<Branch *, std::vector<Branch *>> m_branches;
	std::stack<Call *, std::vector<Call *>> m_calls;

	int m_next_enum_value;
	ClassDescription::Path m_class_base;
	std::stack<Class::Operator> m_operators;
	std::stack<Reference::Flags> m_modifiers;
};

}

#endif // MINT_BUILDTOOL_H
