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

#include "mint/compiler/buildtool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/classregister.h"
#include "mint/ast/node.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/compiler.h"
#include "mint/ast/module.h"
#include "mint/memory/data.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/object.h"
#include "mint/memory/class.h"
#include "mint/memory/reference.h"
#include "mint/system/datastream.h"
#include "mint/system/error.h"
#include "catchcontext.h"
#include "casetable.h"
#include "context.h"
#include "branch.h"
#include "block.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace mint;

BuildContext::BuildContext(DataStream& stream, Compiler& compiler, const Module::Info& data) :
    _compiler(compiler),
    _data(data),
    _lexer(stream),
    _module_context(std::make_unique<Context>()),
    _main_branch(std::make_unique<MainBranch>(compiler.ast(), data)),
    _branch(*_main_branch) {
	stream.set_new_line_callback([this](std::size_t line_number) {
		_branch.get().set_pending_new_line(line_number);
	});
}

BuildContext::~BuildContext() {
	assert(_operators.empty());
	assert(_modifiers.empty());
	assert(_branches.empty());
	_branch.get().build();
}

void BuildContext::commit_line() {
	_branch.get().commit_line();
}

void BuildContext::commit_expr_result() {
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

std::size_t BuildContext::create_fast_scoped_symbol_index(const std::string& symbol) {

	const Symbol* module_symbol = nullptr;
	Context& context = current_context();

	if (context.condition_scoped_symbols) {
		module_symbol = _data.bytecode->make_symbol(symbol);
		context.condition_scoped_symbols->emplace_back(module_symbol);
	}
	else if (context.range_loop_scoped_symbols) {
		module_symbol = _data.bytecode->make_symbol(symbol);
		context.range_loop_scoped_symbols->emplace_back(module_symbol);
	}
	else if (!context.blocks.empty()) {
		auto& block = context.blocks.back();
		module_symbol = _data.bytecode->make_symbol(symbol);
		block->block_scoped_symbols.push_back(module_symbol);
	}

	if (Definition* def = current_definition()) {
		if (def->with_fast) {
			if (module_symbol == nullptr) {
				module_symbol = _data.bytecode->make_symbol(symbol);
			}
			return mint::create_fast_symbol_index(*def, *module_symbol);
		}
	}

	return invalid_index;
}

std::size_t BuildContext::create_fast_symbol_index(const std::string& symbol) {

	const Symbol* module_symbol = nullptr;
	if (Definition* def = current_definition()) {
		if (def->with_fast) {
			module_symbol = _data.bytecode->make_symbol(symbol);
			return mint::create_fast_symbol_index(*def, *module_symbol);
		}
	}

	return invalid_index;
}

std::size_t BuildContext::fast_symbol_index(const std::string& symbol) {

	if (Definition* def = current_definition()) {
		if (def->with_fast) {
			const Symbol* module_symbol = _data.bytecode->make_symbol(symbol);
			return mint::fast_symbol_index(*def, *module_symbol);
		}
	}

	return invalid_index;
}

bool BuildContext::has_returned() const {
	if (const Definition* def = current_definition()) {
		return def->returned;
	}
	return false;
}

void BuildContext::open_block(BlockType type) {

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

void BuildContext::reset_scoped_symbols() {
	Context& context = current_context();
	reset_scoped_symbols(context.blocks.back()->block_scoped_symbols);
}

void BuildContext::reset_scoped_symbols_until(BlockType type) {
	Context& context = current_context();
	for (auto& block : std::views::reverse(context.blocks)) {
		reset_scoped_symbols(block->block_scoped_symbols);
		if (block->range_loop_scoped_symbols) {
			reset_scoped_symbols(*block->range_loop_scoped_symbols);
		}
		if (block->type == type) {
			break;
		}
	}
}

void BuildContext::close_block() {

	Context& context = current_context();
	auto& block = context.blocks.back();

	if (block->condition_scoped_symbols) {
		reset_scoped_symbols(*block->condition_scoped_symbols);
	}

	if (block->range_loop_scoped_symbols) {
		reset_scoped_symbols(*block->range_loop_scoped_symbols);
	}

	context.blocks.pop_back();
}

bool BuildContext::is_in_loop() const {
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

bool BuildContext::is_in_switch() const {
	if (const Block* block = current_breakable_block()) {
		return block->type == BlockType::switch_type;
	}
	return false;
}

bool BuildContext::is_in_range_loop() const {
	if (const Block* block = current_continuable_block()) {
		return block->type == BlockType::range_loop_type;
	}
	return false;
}

bool BuildContext::is_in_function() const {
	return !_definitions.empty();
}

bool BuildContext::is_in_nested_function() const {
	return _definitions.size() >= 2;
}

bool BuildContext::is_in_async_function() const {
	return !_definitions.empty() && _definitions.top()->async;
}

bool BuildContext::is_in_generator() const {
	if (const Definition* def = current_definition()) {
		return def->generator;
	}
	return false;
}

bool BuildContext::is_in_generator_expression() const {
	const auto& context = current_context();
	if (context.meta_blocks.empty()) {
		return false;
	}
	return context.meta_blocks.top() == Context::MetaBlock::generator_expression;
}

void BuildContext::prepare_continue() {

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

void BuildContext::prepare_break() {

	if (const auto* block = current_breakable_block()) {

		switch (block->type) {
		case BlockType::range_loop_type:
			// unload range
			push_node(Node::Command::unload_reference);
			// unload target
			push_node(Node::Command::unload_reference);
			break;

		default:
			break;
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

void BuildContext::prepare_return() {

	if (Definition* def = current_definition()) {

		for (const auto& block : def->blocks) {
			switch (block->type) {
			case BlockType::range_loop_type:
				// unload range
				push_node(Node::Command::unload_reference);
				// unload target
				push_node(Node::Command::unload_reference);
				break;

			default:
				break;
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

void BuildContext::register_retrieve_point() {

	if (Definition* definition = current_definition()) {
		definition->retrieve_point_count++;
	}
	if (Block* block = current_breakable_block()) {
		block->retrieve_point_count++;
	}
}

void BuildContext::unregister_retrieve_point() {

	if (Definition* definition = current_definition()) {
		definition->retrieve_point_count--;
	}
	if (Block* block = current_breakable_block()) {
		block->retrieve_point_count--;
	}
}

void BuildContext::set_exception_symbol(const std::string& symbol) {

	Context& context = current_context();
	auto& block = context.blocks.back();

	if (CatchContext* catch_context = block->catch_context.get()) {
		catch_context->symbol = _data.bytecode->make_symbol(symbol);
	}
}

void BuildContext::reset_exception() {

	Context& context = current_context();
	auto& block = context.blocks.back();

	if (const auto* catch_context = block->catch_context.get()) {
		push_node(Node::Command::reset_exception);
		push_node(catch_context->symbol);
	}
}

void BuildContext::start_case_label() {
	if (CaseTable* case_table = current_breakable_block()->case_table.get()) {
		case_table->current_label = CaseTable::Label(_branch.get());
		push_branch(*case_table->current_label->condition);
	}
}

void BuildContext::resolve_case_label(const std::string& label) {
	if (CaseTable* case_table = current_breakable_block()->case_table.get()) {
		if (!case_table->labels.emplace(label, std::move(*case_table->current_label)).second) {
			parse_error("duplicate case value");
		}
		case_table->current_label = std::nullopt;
		pop_branch();
	}
}

void BuildContext::set_default_label() {
	if (CaseTable* case_table = current_breakable_block()->case_table.get()) {
		if (case_table->default_label) {
			parse_error("multiple default labels in one switch");
		}
		case_table->default_label = _branch.get().next_node_offset();
	}
}

void BuildContext::build_case_table() {

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

void BuildContext::start_jump_forward() {
	_branch.get().start_jump_forward();
}

void BuildContext::bloc_jump_forward() {
	Block* block = current_breakable_block();
	assert(block && block->forward);
	block->forward->push_back(_branch.get().next_node_offset());
	push_node(0);
}

void BuildContext::shift_jump_forward() {
	_branch.get().shift_jump_forward();
}

void BuildContext::resolve_jump_forward() {
	_branch.get().resolve_jump_forward();
}

void BuildContext::start_jump_backward() {
	_branch.get().start_jump_backward();
}

void BuildContext::bloc_jump_backward() {
	const auto* block = current_continuable_block();
	assert(block && block->backward);
	push_node(static_cast<int>(*block->backward));
}

void BuildContext::shift_jump_backward() {
	_branch.get().shift_jump_backward();
}

void BuildContext::resolve_jump_backward() {
	_branch.get().resolve_jump_backward();
}

void BuildContext::start_definition() {
	_definitions.push(std::make_unique<Definition>(Definition {
	    .begin_offset = _branch.get().next_node_offset(),
	    .function = _data.bytecode->make_constant<Function>(),
	}));
}

void BuildContext::start_async_definition() {
	_definitions.push(std::make_unique<Definition>(Definition {
	    .begin_offset = _branch.get().next_node_offset(),
	    .function = _data.bytecode->make_constant<Function>(),
	    .async = true,
	}));
}

bool BuildContext::add_parameter(const std::string& symbol, Reference::Flags flags) {

	Definition* def = current_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	const auto* s = _data.bytecode->make_symbol(symbol);
	const auto index = static_cast<int>(def->fast_symbol_count++);
	def->fast_symbol_indexes.emplace(*s, index);
	def->parameters.push({
	    .flags = flags,
	    .symbol = s,
	});
	return true;
}

bool BuildContext::set_variadic() {

	Definition* def = current_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	const auto* s = _data.bytecode->make_symbol("va_args");
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

	return true;
}

void BuildContext::set_generator() {

	Definition* def = current_definition();
	assert(def);

	for (auto exit_point : def->exit_points) {
		_branch.get().replace_node(exit_point, Node::Command::yield_exit_generator);
	}

	def->generator = true;
}

void BuildContext::set_exit_point() {
	current_definition()->exit_points.emplace_back(_branch.get().next_node_offset());
}

bool BuildContext::save_parameters() {

	auto* def = current_definition();
	assert(def);

	if (def->variadic && def->parameters.empty()) {
		parse_error("expected parameter before '...' token");
		return false;
	}

	const auto count = static_cast<int>(def->parameters.size());
	const int signature = def->variadic ? ~(count - 1) : count;
	Module::Handle& handle = _data.bytecode->make_handle(current_package(), def->begin_offset);

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

	return true;
}

bool BuildContext::add_definition_signature() {

	auto* def = current_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	const auto signature = static_cast<int>(def->parameters.size());
	Module::Handle& handle = _data.bytecode->make_handle(current_package(), def->begin_offset);

	if (def->capture) {
		def->function->data<Function>().mapping.emplace(signature, std::make_unique<Function::Stateful>(handle));
	}
	else {
		def->function->data<Function>().mapping.emplace(signature, std::make_unique<Function::Stateless>(handle));
	}

	def->begin_offset = _branch.get().next_node_offset();
	return true;
}

void BuildContext::save_definition() {

	auto* def = current_definition();
	assert(def);

	for (auto& signature : def->function->data<Function>().mapping) {
		signature.second.handle().fast_count = def->fast_symbol_count;
		signature.second.handle().generator = def->generator;
		signature.second.handle().async = def->async;
	}

	if (def->global_data) {
		_data.bytecode->add_internal_register(std::move(def->global_data));
	}

	push_node(Node::Command::load_constant);
	push_node(def->function);

	if (def->capture) {
		def->capture->build();
	}

	assert(def->blocks.empty());
	_definitions.pop();
}

Function& BuildContext::retrieve_definition() {

	assert(!_definitions.empty());

	auto def = std::move(_definitions.top());
	_definitions.pop();

	auto& data = def->function->data<Function>();
	for (auto& signature : data.mapping) {
		signature.second.handle().fast_count = def->fast_symbol_count;
		signature.second.handle().generator = def->generator;
		signature.second.handle().async = def->async;
	}

	if (def->global_data) {
		_data.bytecode->add_internal_register(std::move(def->global_data));
	}

	assert(def->blocks.empty());
	return data;
}

PackageData& BuildContext::current_package() const {
	if (_packages.empty()) {
		return _compiler.get().ast().global_data();
	}
	return _packages.top().get();
}

void BuildContext::open_package(const std::string& name) {
	PackageData& package = current_package().get_package(Symbol(name));
	push_node(Node::Command::open_package);
	push_node(Compiler::make_package(package));
	_packages.emplace(package);
}

void BuildContext::close_package() {
	assert(!_packages.empty());
	push_node(Node::Command::close_package);
	_packages.pop();
}

void BuildContext::start_class_description(const std::string& name, Reference::Flags flags) {
	_class_base.clear();
	current_context().classes.emplace(_data.bytecode->make_class(_compiler.get().ast(), name), flags);
}

void BuildContext::append_symbol_to_base_class_path(const std::string& symbol) {
	_class_base.append_symbol(Symbol(symbol));
}

void BuildContext::save_base_class_path() {
	current_context().classes.top().first->add_base(_class_base);
	_class_base.clear();
}

bool BuildContext::create_member(Reference::Flags flags, const Symbol& symbol, Data* value) {
	if (value == nullptr) {
		parse_error(symbol.str() + ": member value is not a valid constant");
		return false;
	}
	return create_member(flags, symbol, *value);
}

bool BuildContext::create_member(Reference::Flags flags, const Symbol& symbol, Data& value) {
	if (!current_context().classes.top().first->create_member(symbol, Reference(flags, value))) {
		parse_error(symbol.str() + ": member was already defined");
		return false;
	}
	return true;
}

bool BuildContext::update_member(Reference::Flags flags, const Symbol& symbol, Data& value) {
	if (!current_context().classes.top().first->update_member(symbol, Reference(flags, value))) {
		parse_error(symbol.str() + ": member was already defined");
		return false;
	}
	return true;
}

void BuildContext::resolve_class_description() {

	auto& context = current_context();
	auto [desc, flags] = context.classes.top();
	context.classes.pop();

	if (context.classes.empty()) {
		if (flags & Reference::global) {
			current_package().register_class_description(*desc, flags);
		}
		else if (auto* def = current_definition()) {
			if (!def->global_data) {
				def->global_data = std::make_unique<FunctionData>(_compiler.get().ast());
			}
			def->global_data->register_class_description(*desc, flags);
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

void BuildContext::start_enum_description(const std::string& name, Reference::Flags flags) {
	start_class_description(name, flags);
	_next_enum_value = 0;
}

void BuildContext::set_current_enum_value(int value) {
	_next_enum_value = value + 1;
}

int BuildContext::next_enum_value() {
	return _next_enum_value++;
}

void BuildContext::resolve_enum_description() {
	resolve_class_description();
}

void BuildContext::start_call() {
	_calls.push(std::make_unique<Call>());
}

void BuildContext::add_to_call() {
	_calls.top()->argc++;
}

void BuildContext::resolve_call() {
	push_node(_calls.top()->argc);
	_calls.pop();
}

void BuildContext::start_capture() {
	Definition* def = current_definition();
	def->capture = std::make_unique<SubBranch>(_branch);
	def->with_fast = false;
	push_branch(*def->capture);
	push_node(Node::Command::init_capture);
}

void BuildContext::resolve_capture() {
	Definition* def = current_definition();
	def->with_fast = true;
	pop_branch();
}

bool BuildContext::capture_as(const std::string& symbol) {

	const auto* def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	push_node(Node::Command::capture_as);
	push_node(symbol.c_str());
	return true;
}

bool BuildContext::capture(const std::string& symbol) {

	const auto* def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	push_node(Node::Command::capture_symbol);
	push_node(symbol.c_str());
	return true;
}

bool BuildContext::capture_all() {

	Definition* def = current_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	push_node(Node::Command::capture_all);
	def->capture_all = true;
	return true;
}

void BuildContext::open_generator_expression() {
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

void BuildContext::close_generator_expression() {
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

void BuildContext::open_printer() {
	push_node(Node::Command::open_printer);
	current_context().meta_blocks.push(Context::MetaBlock::printer);
}

void BuildContext::close_printer() {
	assert(current_context().meta_blocks.top() == Context::MetaBlock::printer);
	current_context().meta_blocks.pop();
	push_node(Node::Command::close_printer);
}

void BuildContext::force_printer() {
	current_context().meta_blocks.push(Context::MetaBlock::printer);
}

void BuildContext::start_range_loop() {
	Context& context = current_context();
	context.range_loop_scoped_symbols = std::make_unique<std::vector<const Symbol*>>();
}

void BuildContext::resolve_range_loop() {}

void BuildContext::start_condition() {
	Context& context = current_context();
	context.condition_scoped_symbols = std::make_unique<std::vector<const Symbol*>>();
}

void BuildContext::resolve_condition() {}

void BuildContext::open_sub_branch() {
	Context& context = current_context();
	context.branches.emplace(_branch);
	push_branch(context.branches.top());
}

void BuildContext::close_sub_branch() {
	pop_branch();
}

void BuildContext::build_sub_branch() {
	Context& context = current_context();
	SubBranch branch = std::move(context.branches.top());
	context.branches.pop();
	branch.build();
}

void BuildContext::push_node(Node::Command command) {
	_branch.get().push_node(command);
}

void BuildContext::push_node(int parameter) {
	_branch.get().push_node(parameter);
}

void BuildContext::push_node(std::size_t parameter) {
	_branch.get().push_node(static_cast<int>(parameter));
}

void BuildContext::push_node(const char* symbol) {
	_branch.get().push_node(_data.bytecode->make_symbol(symbol));
}

void BuildContext::push_node(const Symbol* symbol) {
	_branch.get().push_node(symbol);
}

void BuildContext::push_node(Data& constant) {
	_branch.get().push_node(_data.bytecode->make_constant(constant));
}

void BuildContext::push_node(ClassDescription* desc) {
	_branch.get().push_node(desc);
}

std::size_t BuildContext::next_offset() const {
	return _branch.get().next_node_offset();
}

void BuildContext::push_node(const Reference* constant) {
	_branch.get().push_node(constant);
}

void BuildContext::push_branch(Branch& branch) {
	_branches.push(_branch);
	_branch = std::ref(branch);
}

void BuildContext::pop_branch() {
	_branch = _branches.top();
	_branches.pop();
}

void BuildContext::start_operator(Class::Operator op) {
	_operators.push(op);
}

Class::Operator BuildContext::retrieve_operator() {
	assert(!_operators.empty());
	const auto op = _operators.top();
	_operators.pop();
	return op;
}

Symbol BuildContext::retrieve_operator_symbol() {
	assert(!_operators.empty());
	const auto op = _operators.top();
	_operators.pop();
	return get_operator_symbol(op);
}

void BuildContext::start_modifiers(Reference::Flags flags) {
	_modifiers.push(flags);
}

void BuildContext::add_modifiers(Reference::Flags flags) {
	assert(!_modifiers.empty());
	_modifiers.top() |= flags;
}

Reference::Flags BuildContext::get_modifiers() const {
	assert(!_modifiers.empty());
	return _modifiers.top();
}

Reference::Flags BuildContext::retrieve_modifiers() {
	assert(!_modifiers.empty());
	const auto flags = _modifiers.top();
	_modifiers.pop();
	return flags;
}

Compiler& mint::BuildContext::compiler() {
	return _compiler;
}

std::string BuildContext::read_regex() {
	return _lexer.read_regex();
}

void BuildContext::parse_error(const std::string& error_msg) const {
	fflush(stdout);
	error("{}", _lexer.format_error(error_msg));
}

Block* BuildContext::current_breakable_block() {
	const auto& current_stack = current_context().blocks;
	auto it = std::ranges::find_if(std::views::reverse(current_stack), [](const auto& block) {
		return block->is_breakable();
	});
	if (it != current_stack.rend()) {
		return it->get();
	}
	return nullptr;
}

const Block* BuildContext::current_breakable_block() const {
	const auto& current_stack = current_context().blocks;
	for (const auto& block : std::views::reverse(current_stack)) {
		if (block->is_breakable()) {
			return block.get();
		}
	}
	return nullptr;
}

Block* BuildContext::current_continuable_block() {
	const auto& current_stack = current_context().blocks;
	auto it = std::ranges::find_if(std::views::reverse(current_stack), [](const auto& block) {
		return block->is_continuable();
	});
	if (it != current_stack.rend()) {
		return it->get();
	}
	return nullptr;
}

const Block* BuildContext::current_continuable_block() const {
	const auto& current_stack = current_context().blocks;
	for (const auto& block : std::views::reverse(current_stack)) {
		if (block->is_continuable()) {
			return block.get();
		}
	}
	return nullptr;
}

Context& BuildContext::current_context() {
	if (_definitions.empty()) {
		return *_module_context;
	}
	return *_definitions.top();
}

const Context& BuildContext::current_context() const {
	if (_definitions.empty()) {
		return *_module_context;
	}
	return *_definitions.top();
}

Definition* BuildContext::current_definition() {
	if (_definitions.empty()) {
		return nullptr;
	}
	return _definitions.top().get();
}

const Definition* BuildContext::current_definition() const {
	if (_definitions.empty()) {
		return nullptr;
	}
	return _definitions.top().get();
}

std::size_t BuildContext::find_fast_symbol_index(const Symbol& symbol) const {
	if (const Definition* def = current_definition()) {
		if (def->with_fast) {
			return mint::find_fast_symbol_index(*def, symbol);
		}
	}
	return invalid_index;
}

void BuildContext::reset_scoped_symbols(const std::vector<const Symbol*>& symbols) {
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
