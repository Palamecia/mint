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

#include "mint/compiler/buildtool.h"
#include "mint/compiler/compiler.h"
#include "mint/ast/module.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/object.h"
#include "mint/memory/class.h"
#include "mint/system/error.h"
#include "catchcontext.h"
#include "casetable.h"
#include "context.h"
#include "branch.h"
#include "block.h"

#include <iterator>

using namespace std;
using namespace mint;

static const SymbolMapping<Class::Operator> Operators = {
	{ builtin_symbols::new_method, Class::new_operator },
	{ builtin_symbols::delete_method, Class::delete_operator },
};

BuildContext::BuildContext(DataStream *stream, const Module::Info &node) :
	lexer(stream), data(node),
	m_module_context(new Context),
	m_branch(new MainBranch(this)) {
	stream->set_new_line_callback([this] (size_t line_number) {
		m_branch->set_pending_new_line(line_number);
	});
}

BuildContext::~BuildContext() {
	assert(m_operators.empty());
	assert(m_modifiers.empty());
	assert(m_branches.empty());
	m_branch->build();
	delete m_branch;
}

void BuildContext::commit_line() {
	m_branch->commit_line();
}

void BuildContext::commit_expr_result() {
	Context *context = current_context();
	if (context->result_targets.empty()) {
		push_node(Node::unload_reference);
	}
	else {
		switch (context->result_targets.top()) {
		case Context::send_to_printer:
			push_node(Node::print);
			break;
		case Context::send_to_generator_expression:
			push_node(Node::yield_expression);
			break;
		}
	}
}

int BuildContext::create_fast_scoped_symbol_index(const std::string &symbol) {

	Symbol *s = nullptr;

	if (Context *context = current_context()) {
		if (context->condition_scoped_symbols) {
			s = data.module->make_symbol(symbol.c_str());
			context->condition_scoped_symbols->emplace_back(s);
		}
		else if (!context->blocks.empty()) {
			Block *block = context->blocks.back();
			s = data.module->make_symbol(symbol.c_str());
			block->block_scoped_symbols.push_back(s);
		}
	}

	if (Definition *def = current_definition()) {
		if (def->with_fast) {
			if (s == nullptr) {
				s = data.module->make_symbol(symbol.c_str());
			}
			return mint::create_fast_symbol_index(def, s);
		}
	}

	return -1;
}

int BuildContext::fast_scoped_symbol_index(const std::string &symbol) {

	Symbol *s = nullptr;

	if (Context *context = current_context()) {
		if (context->condition_scoped_symbols) {
			s = data.module->make_symbol(symbol.c_str());
			context->condition_scoped_symbols->push_back(s);
		}
		else if (!context->blocks.empty()) {
			Block *block = context->blocks.back();
			s = data.module->make_symbol(symbol.c_str());
			block->block_scoped_symbols.push_back(s);
		}
	}

	if (Definition *def = current_definition()) {
		if (def->with_fast) {
			if (s == nullptr) {
				s = data.module->make_symbol(symbol.c_str());
			}
			return mint::fast_symbol_index(def, s);
		}
	}

	return -1;
}

int BuildContext::create_fast_symbol_index(const std::string &symbol) {

	if (Definition *def = current_definition()) {
		if (def->with_fast) {
			return mint::create_fast_symbol_index(def, data.module->make_symbol(symbol.c_str()));
		}
	}

	return -1;
}

int BuildContext::fast_symbol_index(const std::string &symbol) {

	if (Definition *def = current_definition()) {
		if (def->with_fast) {
			return mint::fast_symbol_index(def, data.module->make_symbol(symbol.c_str()));
		}
	}

	return -1;
}

bool BuildContext::has_returned() const {
	if (const Definition *def = current_definition()) {
		return def->returned;
	}
	return false;
}

void BuildContext::open_block(BlockType type) {

	Context *context = current_context();
	Block *block = new Block(type);

	switch (type) {
	case conditional_loop_type:
	case custom_range_loop_type:
	case range_loop_type:
		block->backward = m_branch->next_jump_backward();
		block->forward = m_branch->next_jump_forward();
		break;

	case switch_type:
		block->case_table = new CaseTable;
		push_node(Node::jump);
		block->case_table->origin = m_branch->next_node_offset();
		push_node(0);
		block->forward = m_branch->start_empty_jump_forward();
		break;

	case catch_type:
		block->catch_context = new CatchContext;
		break;

	default:
		break;
	}

	if (context->condition_scoped_symbols) {
		move(context->condition_scoped_symbols->begin(), context->condition_scoped_symbols->end(), back_inserter(block->block_scoped_symbols));
		block->condition_scoped_symbols = context->condition_scoped_symbols.release();
	}

	context->blocks.emplace_back(block);
}

void BuildContext::reset_scoped_symbols() {
	Context *context = current_context();
	reset_scoped_symbols(&context->blocks.back()->block_scoped_symbols);
}

void BuildContext::reset_scoped_symbols_until(BlockType type) {
	Context *context = current_context();
	for (auto it = context->blocks.rbegin(); it != context->blocks.rend(); ++it) {
		reset_scoped_symbols(&(*it)->block_scoped_symbols);
		if ((*it)->type == type) {
			break;
		}
	}
}

void BuildContext::close_block() {

	Context *context = current_context();
	Block *block = context->blocks.back();

	switch (block->type) {
	case switch_type:
		delete block->case_table;
		resolve_jump_forward();
		break;

	case catch_type:
		delete block->catch_context;
		break;

	default:
		break;
	}

	if (block->condition_scoped_symbols) {
		reset_scoped_symbols(block->condition_scoped_symbols);
		delete block->condition_scoped_symbols;
	}

	context->blocks.pop_back();
	delete block;
}

bool BuildContext::is_in_loop() const {
	if (const Block *block = current_continuable_block()) {
		switch (block->type) {
		case conditional_loop_type:
		case custom_range_loop_type:
		case range_loop_type:
			return true;
		default:
			break;
		}
	}
	return false;
}

bool BuildContext::is_in_switch() const {
	if (const Block *block = current_breakable_block()) {
		return block->type == switch_type;
	}
	return false;
}

bool BuildContext::is_in_range_loop() const {
	if (const Block *block = current_continuable_block()) {
		return block->type == range_loop_type;
	}
	return false;
}

bool BuildContext::is_in_function() const {
	return !m_definitions.empty();
}

bool BuildContext::is_in_generator() const {
	if (const Definition *def = current_definition()) {
		return def->generator;
	}
	return false;
}

void BuildContext::prepare_continue() {

	if (Block *block = current_breakable_block()) {
		
		for (size_t i = 0; i < block->retrieve_point_count; ++i) {
			push_node(Node::unset_retrieve_point);
		}

		Context *context = current_context();
		auto &children = context->blocks;

		for (auto child = children.rbegin(); child != children.rend() && *child != block; ++child) {
			reset_scoped_symbols(&(*child)->block_scoped_symbols);
		}

		reset_scoped_symbols(&block->block_scoped_symbols);
	}
}

void BuildContext::prepare_break() {

	if (Block *block = current_breakable_block()) {

		switch (block->type) {
		case range_loop_type:
			// unload range
			push_node(Node::unload_reference);
			// unload target
			push_node(Node::unload_reference);
			break;

		default:
			break;
		}
		
		for (size_t i = 0; i < block->retrieve_point_count; ++i) {
			push_node(Node::unset_retrieve_point);
		}

		Context *context = current_context();
		auto &children = context->blocks;

		for (auto child = children.rbegin(); child != children.rend() && *child != block; ++child) {
			reset_scoped_symbols(&(*child)->block_scoped_symbols);
		}

		reset_scoped_symbols(&block->block_scoped_symbols);
	}
}

void BuildContext::prepare_return() {

	if (Definition *def = current_definition()) {

		for (const Block *block : def->blocks) {
			switch (block->type) {
			case range_loop_type:
				// unload range
				push_node(Node::unload_reference);
				// unload target
				push_node(Node::unload_reference);
				break;

			default:
				break;
			}
		}

		for (size_t i = 0; i < def->retrieve_point_count; ++i) {
			push_node(Node::unset_retrieve_point);
		}

		if (def->blocks.empty()) {
			def->returned = true;
		}
	}
}

void BuildContext::register_retrieve_point() {

	if (Definition *definition = current_definition()) {
		definition->retrieve_point_count++;
	}
	if (Block *block = current_breakable_block()) {
		block->retrieve_point_count++;
	}
}

void BuildContext::unregister_retrieve_point() {

	if (Definition *definition = current_definition()) {
		definition->retrieve_point_count--;
	}
	if (Block *block = current_breakable_block()) {
		block->retrieve_point_count--;
	}
}

void BuildContext::set_exception_symbol(const string &symbol) {

	Context *context = current_context();
	Block *block = context->blocks.back();

	if (CatchContext *catch_context = block->catch_context) {
		catch_context->symbol = data.module->make_symbol(symbol.c_str());
	}
}

void BuildContext::reset_exception() {

	Context *context = current_context();
	Block *block = context->blocks.back();

	if (CatchContext *catch_context = block->catch_context) {
		push_node(Node::reset_exception);
		push_node(catch_context->symbol);
	}
}

void BuildContext::start_case_label() {
	if (CaseTable *case_table = current_breakable_block()->case_table) {
		case_table->current_label = new CaseTable::Label(m_branch);
		case_table->current_label->offset = m_branch->next_node_offset();
		push_branch(case_table->current_label->condition.get());
	}
}

void BuildContext::resolve_case_label(const string &label) {
	if (CaseTable *case_table = current_breakable_block()->case_table) {
		if (!case_table->labels.emplace(label, case_table->current_label).second) {
			parse_error("duplicate case value");
		}
		pop_branch();
	}
}

void BuildContext::set_default_label() {
	if (CaseTable *case_table = current_breakable_block()->case_table) {
		if (case_table->default_label) {
			parse_error("multiple default labels in one switch");
		}
		case_table->default_label = new size_t(m_branch->next_node_offset());
	}
}

void BuildContext::build_case_table() {

	if (CaseTable *case_table = current_breakable_block()->case_table) {
		
		m_branch->replace_node(case_table->origin, static_cast<int>(m_branch->next_node_offset()));

		for (auto &label : case_table->labels) {
			push_node(Node::reload_reference);
			label.second->condition->build();
			push_node(Node::case_jump);
			push_node(static_cast<int>(label.second->offset));
			delete label.second;
		}

		if (case_table->default_label) {
			push_node(Node::load_constant);
			push_node(Compiler::make_data("true", Compiler::data_true_hint));
			push_node(Node::case_jump);
			push_node(static_cast<int>(*case_table->default_label));
			delete case_table->default_label;
		}
		else {
			push_node(Node::unload_reference);
		}
	}
}

void BuildContext::start_jump_forward() {
	m_branch->start_jump_forward();
}

void BuildContext::bloc_jump_forward() {
	Block *block = current_breakable_block();
	assert(block && block->forward);
	block->forward->push_back(m_branch->next_node_offset());
	push_node(0);
}

void BuildContext::shift_jump_forward() {
	m_branch->shift_jump_forward();
}

void BuildContext::resolve_jump_forward() {
	m_branch->resolve_jump_forward();
}

void BuildContext::start_jump_backward() {
	m_branch->start_jump_backward();
}

void BuildContext::bloc_jump_backward() {
	Block *block = current_continuable_block();
	assert(block && block->backward);
	push_node(static_cast<int>(*block->backward));
}

void BuildContext::shift_jump_backward() {
	m_branch->shift_jump_backward();
}

void BuildContext::resolve_jump_backward() {
	m_branch->resolve_jump_backward();
}

void BuildContext::start_definition() {
	Definition *def = new Definition;
	def->function = data.module->make_constant(GarbageCollector::instance().alloc<Function>());
	def->begin_offset = m_branch->next_node_offset();
	m_definitions.push(def);
}

bool BuildContext::add_parameter(const string &symbol, Reference::Flags flags) {

	Definition *def = current_definition();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}
	
	Symbol *s = data.module->make_symbol(symbol.c_str());
	const int index = static_cast<int>(def->fast_symbol_count++);
	def->fast_symbol_indexes.emplace(*s, index);
	def->parameters.push({flags, s});
	return true;
}

bool BuildContext::set_variadic() {

	Definition *def = current_definition();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}
	
	Symbol *s = data.module->make_symbol("va_args");
	const int index = static_cast<int>(def->fast_symbol_count++);
	def->fast_symbol_indexes.emplace(*s, index);
	def->parameters.push({Reference::standard, s});
	def->variadic = true;

	if (!def->function->data<Function>()->mapping.empty()) {
		push_node(Node::create_iterator);
		push_node(0);
	}

	return true;
}

void BuildContext::set_generator() {

	Definition *def = current_definition();

	for (auto exit_point : def->exit_points) {
		m_branch->replace_node(exit_point, Node::yield_exit_generator);
	}

	def->generator = true;
}

void BuildContext::set_exit_point() {
	current_definition()->exit_points.emplace_back(m_branch->next_node_offset());
}

bool BuildContext::save_parameters() {

	Definition *def = current_definition();
	if (def->variadic && def->parameters.empty()) {
		parse_error("expected parameter before '...' token");
		return false;
	}

	int count = static_cast<int>(def->parameters.size());
	int signature = def->variadic ? ~(count - 1) : count;
	Module::Handle *handle = data.module->make_handle(current_package(), data.id, def->begin_offset);
	def->function->data<Function>()->mapping.emplace(signature, Function::Signature(handle, def->capture != nullptr));

	while (!def->parameters.empty()) {
		Parameter &param = def->parameters.top();
		push_node(Node::init_param);
		push_node(param.symbol);
		push_node(param.flags);
		push_node(mint::fast_symbol_index(def, param.symbol));
		def->parameters.pop();
	}

	return true;
}

bool BuildContext::add_definition_signature() {

	Definition *def = current_definition();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	int signature = static_cast<int>(def->parameters.size());
	Module::Handle *handle = data.module->make_handle(current_package(), data.id, def->begin_offset);
	def->function->data<Function>()->mapping.emplace(signature, Function::Signature(handle, def->capture != nullptr));
	def->begin_offset = m_branch->next_node_offset();
	return true;
}

void BuildContext::save_definition() {

	Definition *def = current_definition();

	for (auto &signature : def->function->data<Function>()->mapping) {
		signature.second.handle->fast_count = def->fast_symbol_count;
		signature.second.handle->generator = def->generator;
	}

	push_node(Node::load_constant);
	push_node(def->function);

	if (def->capture) {
		def->capture->build();
		delete def->capture;
	}

	assert(def->blocks.empty());
	m_definitions.pop();
	delete def;
}

Data *BuildContext::retrieve_definition() {

	Definition *def = current_definition();
	Data *data = def->function->data();

	for (auto &signature : def->function->data<Function>()->mapping) {
		signature.second.handle->fast_count = def->fast_symbol_count;
		signature.second.handle->generator = def->generator;
	}

	assert(def->blocks.empty());
	m_definitions.pop();
	delete def;

	return data;
}

PackageData *BuildContext::current_package() const {
	if (m_packages.empty()) {
		return GlobalData::instance();
	}
	return m_packages.top();
}

void BuildContext::open_package(const string &name) {
	PackageData *package = current_package()->get_package(Symbol(name));
	push_node(Node::open_package);
	push_node(GarbageCollector::instance().alloc<Package>(package));
	m_packages.push(package);
}

void BuildContext::close_package() {
	assert(!m_packages.empty());
	push_node(Node::close_package);
	m_packages.pop();
}

void BuildContext::start_class_description(const string &name, Reference::Flags flags) {
	m_class_base.clear();
	current_context()->classes.push(new ClassDescription(current_package(), flags, name));
}

void BuildContext::append_symbol_to_base_class_path(const string &symbol) {
	m_class_base.append_symbol(Symbol(symbol));
}

void BuildContext::save_base_class_path() {
	current_context()->classes.top()->add_base(m_class_base);
	m_class_base.clear();
}

bool BuildContext::create_member(Reference::Flags flags, Class::Operator op, Data *value) {

	if (value == nullptr) {
		string error_message = get_operator_symbol(op).str() + ": member value is not a valid constant";
		parse_error(error_message.c_str());
		return false;
	}
	
	if (!current_context()->classes.top()->create_member(op, WeakReference(flags, value))) {
		string error_message = get_operator_symbol(op).str() + ": member was already defined";
		parse_error(error_message.c_str());
		return false;
	}

	return true;
}

bool BuildContext::create_member(Reference::Flags flags, const Symbol &symbol, Data *value) {

	auto i = Operators.find(symbol);

	if (i == Operators.end()) {
		if (value == nullptr) {
			string error_message = symbol.str() + ": member value is not a valid constant";
			parse_error(error_message.c_str());
			return false;
		}
		
		if (!current_context()->classes.top()->create_member(symbol, WeakReference(flags, value))) {
			string error_message = symbol.str() + ": member was already defined";
			parse_error(error_message.c_str());
			return false;
		}

		return true;
	}

	return create_member(flags, i->second, value);
}

bool BuildContext::update_member(Reference::Flags flags, Class::Operator op, Data *value) {
	
	if (!current_context()->classes.top()->update_member(op, WeakReference(flags, value))) {
		string error_message = get_operator_symbol(op).str() + ": member was already defined";
		parse_error(error_message.c_str());
		return false;
	}

	return true;
}

bool BuildContext::update_member(Reference::Flags flags, const Symbol &symbol, Data *value) {

	auto i = Operators.find(symbol);

	if (i == Operators.end()) {
		if (!current_context()->classes.top()->update_member(symbol, WeakReference(flags, value))) {
			string error_message = symbol.str() + ": member was already defined";
			parse_error(error_message.c_str());
			return false;
		}

		return true;
	}

	return update_member(flags, i->second, value);
}

void BuildContext::resolve_class_description() {

	Context *context = current_context();

	ClassDescription *desc = context->classes.top();
	context->classes.pop();

	if (context->classes.empty()) {
		push_node(Node::register_class);
		push_node(static_cast<int>(current_package()->create_class(desc)));
	}
	else {
		context->classes.top()->create_class(desc);
	}
}

void BuildContext::start_enum_description(const string &name, Reference::Flags flags) {
	start_class_description(name, flags);
	m_next_enum_value = 0;
}

void BuildContext::set_current_enum_value(int value) {
	m_next_enum_value = value + 1;
}

int BuildContext::next_enum_value() {
	return m_next_enum_value++;
}

void BuildContext::resolve_enum_description() {
	resolve_class_description();
}

void BuildContext::start_call() {
	m_calls.push(new Call);
}

void BuildContext::add_to_call() {
	m_calls.top()->argc++;
}

void BuildContext::resolve_call() {
	push_node(m_calls.top()->argc);
	delete m_calls.top();
	m_calls.pop();
}


void BuildContext::start_capture() {
	Definition *def = current_definition();
	def->capture = new SubBranch(m_branch);
	def->with_fast = false;
	push_branch(def->capture);
}

void BuildContext::resolve_capture() {
	Definition *def = current_definition();
	def->with_fast = true;
	pop_branch();
}

bool BuildContext::capture_as(const string &symbol) {

	Definition *def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		delete def->capture;
		return false;
	}

	push_node(Node::capture_as);
	push_node(symbol.c_str());
	return true;
}

bool BuildContext::capture(const string &symbol) {

	Definition *def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		delete def->capture;
		return false;
	}

	push_node(Node::capture_symbol);
	push_node(symbol.c_str());
	return true;
}

bool BuildContext::capture_all() {

	Definition *def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		delete def->capture;
		return false;
	}

	push_node(Node::capture_all);
	def->capture_all = true;
	return true;
}

void BuildContext::open_generator_expression() {
	push_node(Node::begin_generator_expression);
	current_context()->result_targets.push(Context::send_to_generator_expression);
}

void BuildContext::close_generator_expression() {
	push_node(Node::end_generator_expression);
	current_context()->result_targets.pop();
}

void BuildContext::open_printer() {
	push_node(Node::open_printer);
	current_context()->result_targets.push(Context::send_to_printer);
}

void BuildContext::close_printer() {
	push_node(Node::close_printer);
	current_context()->result_targets.pop();
}

void BuildContext::force_printer() {
	current_context()->result_targets.push(Context::send_to_printer);
}

void BuildContext::start_condition() {
	Context *context = current_context();
	context->condition_scoped_symbols.reset(new vector<Symbol *>);
}

void BuildContext::resolve_condition() {

}

void BuildContext::push_node(Node::Command command) {
	m_branch->push_node(command);
}

void BuildContext::push_node(int parameter) {
	m_branch->push_node(parameter);
}

void BuildContext::push_node(const char *symbol) {
	m_branch->push_node(data.module->make_symbol(symbol));
}

void BuildContext::push_node(Symbol *symbol) {
	m_branch->push_node(symbol);
}

void BuildContext::push_node(Data *constant) {
	m_branch->push_node(data.module->make_constant(constant));
}

void BuildContext::push_node(Reference *constant) {
	m_branch->push_node(constant);
}

void BuildContext::push_branch(Branch *branch) {
	m_branches.push(m_branch);
	m_branch = branch;
}

void BuildContext::pop_branch() {
	m_branch = m_branches.top();
	m_branches.pop();
}

void BuildContext::start_operator(Class::Operator op) {
	m_operators.push(op);
}

Class::Operator BuildContext::retrieve_operator() {
	assert(!m_operators.empty());
	Class::Operator op = m_operators.top();
	m_operators.pop();
	return op;
}

void BuildContext::start_modifiers(Reference::Flags flags) {
	m_modifiers.push(flags);
}

void BuildContext::add_modifiers(Reference::Flags flags) {
	assert(!m_modifiers.empty());
	m_modifiers.top() |= flags;
}

Reference::Flags BuildContext::retrieve_modifiers() {
	assert(!m_modifiers.empty());
	Reference::Flags flags = m_modifiers.top();
	m_modifiers.pop();
	return flags;
}

void BuildContext::parse_error(const char *error_msg) {
	fflush(stdout);
	error("%s", lexer.format_error(error_msg).c_str());
}

Block *BuildContext::current_breakable_block() {
	auto &current_stack = current_context()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_breakable()) {
			return *block;
		}
	}
	return nullptr;
}

const Block *BuildContext::current_breakable_block() const {
	auto &current_stack = current_context()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_breakable()) {
			return *block;
		}
	}
	return nullptr;
}

Block *BuildContext::current_continuable_block() {
	auto &current_stack = current_context()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_continuable()) {
			return *block;
		}
	}
	return nullptr;
}

const Block *BuildContext::current_continuable_block() const {
	auto &current_stack = current_context()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_continuable()) {
			return *block;
		}
	}
	return nullptr;
}

Context *BuildContext::current_context() {
	if (m_definitions.empty()) {
		return m_module_context.get();
	}
	return m_definitions.top();
}

const Context *BuildContext::current_context() const {
	if (m_definitions.empty()) {
		return m_module_context.get();
	}
	return m_definitions.top();
}

Definition *BuildContext::current_definition() {
	if (m_definitions.empty()) {
		return nullptr;
	}
	return m_definitions.top();
}

const Definition *BuildContext::current_definition() const {
	if (m_definitions.empty()) {
		return nullptr;
	}
	return m_definitions.top();
}

int BuildContext::find_fast_symbol_index(Symbol *symbol) const {
	if (const Definition *def = current_definition()) {
		if (def->with_fast) {
			return mint::find_fast_symbol_index(def, symbol);
		}
	}
	return -1;
}

void BuildContext::reset_scoped_symbols(const vector<Symbol *> *symbols) {
	for (auto symbol = symbols->rbegin(); symbol != symbols->rend(); ++symbol) {
		int index = find_fast_symbol_index(*symbol);
		if (index != -1) {
			push_node(Node::reset_fast);
			push_node(*symbol);
			push_node(index);
		}
		else {
			push_node(Node::reset_symbol);
			push_node(*symbol);
		}
	}
}
