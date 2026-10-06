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

#include "mint/compiler/build_context.h"
#include "mint/compiler/descriptions.h"
#include "mint/compiler/parser_data.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/memory/garbage_collector.h"
#include "mint/program/program.h"
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
#include "mint/system/error.h"
#include "catch_context.h"
#include "case_table.h"
#include "context.h"
#include "branch.h"
#include "block.h"
#include "parser.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace mint;

BuildContext::BuildContext(DataStream& stream, Compiler& compiler) :
    _compiler(compiler),
    _lexer(stream),
    _module(std::make_unique<Context>()),
    _main_branch(std::make_unique<MainBranch>(compiler)),
    _branch(*_main_branch) {
	stream.set_new_line_callback([this](std::size_t line_number) {
		_branch.get().set_pending_new_line(line_number);
	});
}

BuildContext::~BuildContext() {
	assert(_operators.empty() || std::uncaught_exceptions() > 0);
	assert(_modifiers.empty() || std::uncaught_exceptions() > 0);
	assert(_branches.empty() || std::uncaught_exceptions() > 0);
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

bool BuildContext::has_returned() const {
	if (const auto* def = current_function_definition()) {
		return def->returned;
	}
	return false;
}

std::size_t BuildContext::fast_symbol_index(const std::string& name) {
	if (auto* def = current_function_definition(); def && def->with_fast) {
		const Symbol* module_symbol = _compiler.get().data().bytecode.make_symbol(Symbol(name));
		return mint::fast_symbol_index(*def, *module_symbol);
	}
	return invalid_index;
}

ParserData mint::BuildContext::create_fast_scoped_symbol(const std::string& name, TypeAnnotation type,
    Reference::Flags flags) {

	const auto [symbol, index] = create_fast_scoped_symbol_index(name);
	const VariableDescription* description = nullptr;

	if (flags & Reference::global) {
		description = &current_package().create_variable(*symbol, std::move(type));
	}
	else if (const auto* def = current_function_definition()) {
		description = &def->description->create_variable(*symbol, std::move(type));
	}
	else {
		description = &_compiler.get().data().description.create_variable(*symbol, std::move(type));
	}

	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(symbol);
		push_node(index);
		push_node(flags);
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(symbol);
		push_node(flags);
	}

	return *description;
}

ParserData mint::BuildContext::create_fast_symbol(const std::string& name, TypeAnnotation type, Reference::Flags flags) {

	const auto [symbol, index] = create_fast_symbol_index(name);
	const VariableDescription* description = nullptr;

	if (flags & Reference::global) {
		description = &current_package().create_variable(*symbol, std::move(type));
	}
	else if (const auto* def = current_function_definition()) {
		description = &def->description->create_variable(*symbol, std::move(type));
	}
	else {
		description = &_compiler.get().data().description.create_variable(*symbol, std::move(type));
	}

	if (index != invalid_index) {
		push_node(Node::Command::declare_fast);
		push_node(symbol);
		push_node(index);
		push_node(flags);
	}
	else {
		push_node(Node::Command::declare_symbol);
		push_node(symbol);
		push_node(flags);
	}

	return *description;
}

ParserData mint::BuildContext::load_fast_symbol(const std::string& name) {
	const auto index = fast_symbol_index(name);
	if (index != invalid_index) {
		push_node(Node::Command::load_fast);
		push_node(name.c_str());
		push_node(index);
	}
	else {
		push_node(Node::Command::load_symbol);
		push_node(name.c_str());
	}
	return {}; // TODO: find variable description from index or name
}

void BuildContext::open_block(BlockType type) {

	Context& context = current_context();
	auto block = std::unique_ptr<Block>();

	switch (type) {
	case BlockType::conditional_loop_type:
		block = std::make_unique<ConditionalLoopBlock>(context);
		block->backward = _branch.get().next_jump_backward();
		block->forward = _branch.get().next_jump_forward();
		break;
	case BlockType::custom_range_loop_type:
		block = std::make_unique<CustomRangeLoopBlock>(context);
		block->backward = _branch.get().next_jump_backward();
		block->forward = _branch.get().next_jump_forward();
		break;
	case BlockType::range_loop_type:
		block = std::make_unique<RangeLoopBlock>(context);
		block->backward = _branch.get().next_jump_backward();
		block->forward = _branch.get().next_jump_forward();
		break;
	case BlockType::switch_type:
		{
			auto switch_block = std::make_unique<SwitchBlock>(context);
			push_node(Node::Command::jump);
			switch_block->case_table.origin = _branch.get().next_node_offset();
			push_node(0);
			switch_block->forward = _branch.get().start_empty_jump_forward();
			block = std::move(switch_block);
		}
		break;
	case BlockType::if_type:
		block = std::make_unique<IfBlock>(context);
		break;
	case BlockType::elif_type:
		block = std::make_unique<ElifBlock>(context);
		break;
	case BlockType::else_type:
		block = std::make_unique<ElseBlock>();
		break;
	case BlockType::try_type:
		block = std::make_unique<TryBlock>();
		break;
	case BlockType::catch_type:
		block = std::make_unique<CatchBlock>();
		break;
	case BlockType::print_type:
		block = std::make_unique<PrintBlock>();
		break;
	}

	context.blocks.emplace_back(std::move(block));
}

void BuildContext::reset_scoped_symbols() {
	Context& context = current_context();
	reset_scoped_symbols(context.blocks.back()->block_scoped_symbols);
}

void BuildContext::reset_scoped_symbols_until(BlockType type) {
	Context& context = current_context();
	for (const auto& block : std::views::reverse(context.blocks)) {
		reset_scoped_symbols(block->block_scoped_symbols);
		if (const auto* symbols = block->get_range_loop_scoped_symbols()) {
			reset_scoped_symbols(*symbols);
		}
		if (block->type() == type) {
			break;
		}
	}
}

void BuildContext::close_block() {

	Context& context = current_context();
	const auto& block = context.blocks.back();

	if (const auto* symbols = block->get_condition_scoped_symbols()) {
		reset_scoped_symbols(*symbols);
	}

	if (const auto* symbols = block->get_range_loop_scoped_symbols()) {
		reset_scoped_symbols(*symbols);
	}

	context.blocks.pop_back();
}

bool BuildContext::is_in_catch() const {
	return std::ranges::any_of(current_context().blocks, [](const auto& block) {
		return block->type() == BlockType::catch_type;
	});
}

bool BuildContext::is_in_loop() const {
	if (const Block* block = current_continuable_block()) {
		switch (block->type()) {
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
		return block->type() == BlockType::switch_type;
	}
	return false;
}

bool BuildContext::is_in_range_loop() const {
	if (const Block* block = current_continuable_block()) {
		return block->type() == BlockType::range_loop_type;
	}
	return false;
}

bool BuildContext::is_in_function() const {
	return !_functions.empty();
}

bool BuildContext::is_in_nested_function() const {
	return _functions.size() >= 2;
}

bool BuildContext::is_in_async_function() const {
	return !_functions.empty() && _functions.top()->async;
}

bool BuildContext::is_in_generator() const {
	if (const auto* def = current_function_definition()) {
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

		if (block->type() == BlockType::range_loop_type) {
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

void BuildContext::prepare_return() {

	if (auto* def = current_function_definition()) {

		for (const auto& block : def->blocks) {
			if (block->type() == BlockType::range_loop_type) {
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

void BuildContext::register_retrieve_point() {

	if (auto* definition = current_function_definition()) {
		definition->retrieve_point_count++;
	}
	if (Block* block = current_breakable_block()) {
		block->retrieve_point_count++;
	}
}

void BuildContext::unregister_retrieve_point() {

	if (auto* definition = current_function_definition()) {
		definition->retrieve_point_count--;
	}
	if (Block* block = current_breakable_block()) {
		block->retrieve_point_count--;
	}
}

void BuildContext::set_exception_symbol(const std::string& symbol) {

	Context& context = current_context();
	const auto& block = context.blocks.back();

	if (auto* catch_block = dynamic_cast<CatchBlock*>(block.get())) {
		catch_block->catch_context.symbol = _compiler.get().data().bytecode.make_symbol(Symbol(symbol));
	}
}

void BuildContext::reset_exception() {

	Context& context = current_context();
	const auto& block = context.blocks.back();

	if (const auto* catch_block = dynamic_cast<CatchBlock*>(block.get())) {
		push_node(Node::Command::reset_exception);
		push_node(catch_block->catch_context.symbol);
	}
}

void BuildContext::start_case_label() {
	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {
		switch_block->case_table.current_case = std::make_pair(CaseTable::Label(), CaseTable::Case(_branch));
		push_branch(switch_block->case_table.current_case->second.condition);
	}
}

void mint::BuildContext::add_to_case_label(const std::string& token) {
	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {
		switch_block->case_table.current_case->first.tokens.push_back(token);
	}
}

void BuildContext::resolve_case_label() {
	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {
		auto case_label = std::exchange(switch_block->case_table.current_case, std::nullopt);
		if (const auto labels = std::views::keys(switch_block->case_table.cases);
		    std::ranges::find(labels, case_label->first) != labels.end()) {
			parse_error("duplicate case value");
		}
		switch_block->case_table.cases.emplace_back(*case_label);
		pop_branch();
	}
}

void BuildContext::set_default_label() {
	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {
		if (switch_block->case_table.default_case) {
			parse_error("multiple default labels in one switch");
		}
		switch_block->case_table.default_case = _branch.get().next_node_offset();
	}
}

void BuildContext::build_case_table() {

	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {

		_branch.get().replace_node(switch_block->case_table.origin, static_cast<int>(_branch.get().next_node_offset()));

		for (auto& case_label : std::views::values(switch_block->case_table.cases)) {
			push_node(Node::Command::reload_reference);
			case_label.condition.build();
			push_node(Node::Command::case_jump);
			push_node(static_cast<int>(case_label.offset));
		}

		if (const auto default_case = switch_block->case_table.default_case) {
			push_node(Node::Command::load_constant);
			push_node(_compiler.get().make_constant<Boolean>(true));
			push_node(Node::Command::case_jump);
			push_node(static_cast<int>(*default_case));
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
	const auto* block = current_breakable_block();
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

PackageDescription& BuildContext::current_package() const {
	if (_packages.empty()) {
		return _compiler.get().program().global_data_description();
	}
	return _packages.top().get();
}

void BuildContext::open_package(const std::string& name) {
	PackageDescription& package = current_package().get_or_create_package(_compiler.get().program(), Symbol(name));
	push_node(Node::Command::open_package);
	push_node(_compiler.get().make_constant<Package>(package.data()));
	_packages.emplace(package);
}

void BuildContext::close_package() {
	assert(!_packages.empty());
	push_node(Node::Command::close_package);
	_packages.pop();
}

void BuildContext::start_class_description(const std::string& name, Reference::Flags flags) {

	auto& context = current_context();

	const auto symbol = Symbol(name);
	_class_base.clear();

	if (!context.classes.empty()) {
		assert(flags & Reference::global);
		current_context().classes.emplace(context.classes.top().description.get().create_class(symbol, flags), flags);
	}
	else if (flags & Reference::global) {
		current_context().classes.emplace(current_package().create_class(symbol, flags), flags);
	}
	else if (const auto* def = current_function_definition()) {
		current_context().classes.emplace(def->description->create_class(symbol), flags);
	}
	else {
		current_context().classes.emplace(_compiler.get().data().description.create_class(symbol), flags);
	}
}

void BuildContext::append_symbol_to_base_class_path(const std::string& symbol) {
	_class_base.emplace_back(symbol);
}

void BuildContext::save_base_class_path() {
	current_context().classes.top().description.get().add_base(std::exchange(_class_base, {}));
}

void BuildContext::resolve_class_description() {

	auto& context = current_context();
	auto [desc, flags] = context.classes.top();
	context.classes.pop();

	if (context.classes.empty()) {
		push_node(Node::Command::declare_class);
		push_node(&desc.get());
		push_node(flags);
	}
}

void BuildContext::create_attribute(const std::string& name, TypeAnnotation type, Reference::Flags flags, Data* value) {
	ClassDescription& class_description = current_context().classes.top().description;
	class_description.create_attribute(Symbol(name), std::move(type), _compiler.get().make_constant(flags, value));
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

void BuildContext::start_function_description() {
	_functions.emplace(std::make_unique<FunctionDefinition>(FunctionDefinition {
	    .enclosing = current_function_definition(),
	    .begin_offset = _branch.get().next_node_offset(),
	}));
}

void BuildContext::start_async_function_description() {
	_functions.emplace(std::make_unique<FunctionDefinition>(FunctionDefinition {
	    .enclosing = current_function_definition(),
	    .begin_offset = _branch.get().next_node_offset(),
	    .async = true,
	}));
}

void BuildContext::add_parameter(const std::string& name, TypeAnnotation type, Reference::Flags flags) {

	auto* def = current_function_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	const auto* symbol = _compiler.get().data().bytecode.make_symbol(Symbol(name));
	const auto index = static_cast<int>(def->fast_symbol_count++);
	def->fast_symbol_indexes.emplace(*symbol, index);
	def->parameters.push({
	    .symbol = symbol,
	    .type = std::move(type),
	    .flags = flags,
	});
}

void BuildContext::set_variadic() {

	auto* def = current_function_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	const auto* symbol = _compiler.get().data().bytecode.make_symbol("va_args");
	const auto index = static_cast<int>(def->fast_symbol_count++);
	def->fast_symbol_indexes.emplace(*symbol, index);
	def->parameters.push({
	    .symbol = symbol,
	    .type = BuiltinTypeKind::iterator,
	    .flags = Reference::default_flags,
	});
	def->variadic = true;

	if (!def->signatures.empty()) {
		push_node(Node::Command::init_iterator);
		push_node(0);
	}
}

void BuildContext::set_generator() {

	auto* def = current_function_definition();
	assert(def);

	for (const auto exit_point : def->exit_points) {
		_branch.get().replace_node(exit_point, Node::Command::yield_exit_generator);
	}

	def->generator = true;
}

void BuildContext::set_exit_point() {
	current_function_definition()->exit_points.emplace_back(_branch.get().next_node_offset());
}

ParserData BuildContext::save_method_signature(const std::string& name, TypeAnnotation return_type,
    Reference::Flags flags) {

	auto* def = current_function_definition();
	assert(def != nullptr);

	save_function_signatures(def);
	def->flags = flags;

	if (auto* enclosing = def->enclosing) {
		def->description = &enclosing->classes.top().description.get().create_method(Symbol(name),
		    std::move(return_type), make_function(def));
	}
	else {
		def->description = &_module->classes.top().description.get().create_method(Symbol(name), std::move(return_type),
		    make_function(def));
	}

	return *def->description;
}

ParserData mint::BuildContext::save_method_signature(Class::Operator op, TypeAnnotation return_type,
    Reference::Flags flags) {

	auto* def = current_function_definition();
	assert(def != nullptr);

	save_function_signatures(def);
	def->flags = flags;

	if (auto* enclosing = def->enclosing) {
		def->description = &enclosing->classes.top().description.get().create_method(get_operator_symbol(op),
		    std::move(return_type), make_function(def));
	}
	else {
		def->description = &_module->classes.top().description.get().create_method(get_operator_symbol(op),
		    std::move(return_type), make_function(def));
	}

	return *def->description;
}

ParserData BuildContext::save_function_signatures(const std::string& name, TypeAnnotation return_type,
    Reference::Flags flags) {

	auto* def = current_function_definition();
	assert(def != nullptr);

	save_function_signatures(def);
	def->flags = flags;

	if (def->flags & Reference::global) {
		def->description = &current_package().create_function(Symbol(name), std::move(return_type));
	}
	else if (const auto* parent = def->enclosing) {
		def->description = &parent->description->create_function(Symbol(name), std::move(return_type));
	}
	else {
		def->description = &compiler().data().description.create_function(Symbol(name), std::move(return_type));
	}

	return *def->description;
}

ParserData mint::BuildContext::save_function_signatures(TypeAnnotation return_type, Reference::Flags flags) {

	auto* def = current_function_definition();
	assert(def != nullptr);

	save_function_signatures(def);
	def->flags = flags;

	if (def->flags & Reference::global) {
		def->description = &current_package().create_function(std::move(return_type));
	}
	else if (const auto* parent = def->enclosing; parent && parent->description) {
		def->description = &parent->description->create_function(std::move(return_type));
	}
	else {
		def->description = &compiler().data().description.create_function(std::move(return_type));
	}

	return *def->description;
}

void BuildContext::add_function_signature() {

	auto* def = current_function_definition();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	def->signatures.push_back({
	    .parameter_count = static_cast<int>(def->parameters.size()),
	    .handle = _compiler.get().data().bytecode.make_handle(current_package().data(), def->begin_offset),
	});

	def->begin_offset = _branch.get().next_node_offset();
}

void mint::BuildContext::resolve_method_description() {

	const auto* def = current_function_definition();
	assert(def != nullptr);

	resolve_function_description(def);

	assert(def->blocks.empty());
	_functions.pop();
}

void BuildContext::resolve_function_description() {

	auto* def = current_function_definition();
	assert(def != nullptr);

	resolve_function_description(def);

	push_node(Node::Command::load_constant);
	push_node(make_function(def));

	if (def->capture) {
		def->capture->build();
	}

	assert(def->blocks.empty());
	_functions.pop();
}

void mint::BuildContext::register_function_description() {

	const auto* def = current_function_definition();
	assert(def);

	push_node(Node::Command::declare_function);
	push_node(def->description->get_name());
	push_node(def->flags);
}

void BuildContext::start_call() {
	_calls.emplace();
}

void BuildContext::add_to_call(ParserData parameter) {
	_calls.top().parameters.emplace_back(std::move(parameter));
}

BuildContext::Call BuildContext::resolve_call() {
	auto call = std::move(_calls.top());
	push_node(static_cast<int>(call.parameters.size()));
	_calls.pop();
	return call;
}

void BuildContext::start_capture() {
	auto* def = current_function_definition();
	def->capture.emplace(_branch);
	def->with_fast = false;
	push_branch(*def->capture);
	push_node(Node::Command::init_capture);
}

void BuildContext::resolve_capture() {
	auto* def = current_function_definition();
	def->with_fast = true;
	pop_branch();
}

void BuildContext::capture_as(const std::string& symbol) {

	const auto* def = current_function_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	push_node(Node::Command::capture_as);
	push_node(Symbol(symbol));
}

void BuildContext::capture(const std::string& symbol) {

	const auto* def = current_function_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	push_node(Node::Command::capture_symbol);
	push_node(Symbol(symbol));
}

void BuildContext::capture_all() {

	auto* def = current_function_definition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	push_node(Node::Command::capture_all);
	def->capture_all = true;
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

void BuildContext::build_sub_branch() {
	Context& context = current_context();
	SubBranch branch = std::move(context.branches.top());
	context.branches.pop();
	branch.build();
}

void BuildContext::close_sub_branch() {
	pop_branch();
}

void BuildContext::push_node(Node::Command command) {
	_branch.get().push_node(command);
}

void BuildContext::push_node(int parameter) {
	_branch.get().push_node(parameter);
}

void BuildContext::push_node(Reference::Flags parameter) {
	_branch.get().push_node(static_cast<int>(parameter));
}

void BuildContext::push_node(std::size_t parameter) {
	_branch.get().push_node(static_cast<int>(parameter));
}

void BuildContext::push_node(const Symbol* symbol) {
	_branch.get().push_node(symbol);
}

void BuildContext::push_node(const Symbol& symbol) {
	_branch.get().push_node(_compiler.get().data().bytecode.make_symbol(symbol));
}

void BuildContext::push_node(const Reference* constant) {
	_branch.get().push_node(constant);
}

void BuildContext::push_node(Data* constant) {
	_branch.get().push_node(_compiler.get().data().bytecode.make_constant(constant));
}

void BuildContext::push_node(const ClassDescription* desc) {
	_branch.get().push_node(desc);
}

std::size_t BuildContext::next_offset() const {
	return _branch.get().next_node_offset();
}

void BuildContext::start_operator(Class::Operator op) {
	_operators.push(op);
}

Class::Operator mint::BuildContext::get_operator() const {
	assert(!_operators.empty());
	return _operators.top();
}

Class::Operator BuildContext::retrieve_operator() {
	assert(!_operators.empty());
	const auto op = _operators.top();
	_operators.pop();
	return op;
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

void mint::BuildContext::start_type_annotation(TypeAnnotation annotation) {
	_annotations.push(std::move(annotation));
}

void mint::BuildContext::add_type_annotation(TypeAnnotation annotation) {
	assert(!_annotations.empty());
	_annotations.top().add_type(std::move(annotation));
}

TypeAnnotation mint::BuildContext::get_type_annotation() const {
	assert(!_annotations.empty());
	return _annotations.top();
}

TypeAnnotation BuildContext::retrieve_type_annotation() {
	assert(!_annotations.empty());
	const auto annotation = _annotations.top();
	_annotations.pop();
	return annotation;
}

ParserData mint::BuildContext::get_member(const ParserData& object, const std::string& member) const {
	auto member_data = find_member(object, Symbol(member));
	if (!member_data) [[unlikely]] {
		parse_error(std::format("{} has no member '{}'", object.get_name(), member));
	}
	return *member_data;
}

ParserData mint::BuildContext::get_operator(const ParserData& object, Class::Operator op) const {
	auto member_data = find_member(object, get_operator_symbol(op));
	if (!member_data) [[unlikely]] {
		parse_error(std::format("{} has no member '{}'", object.get_name(), get_operator_symbol(op).str()));
	}
	return *member_data;
}

ParserData mint::BuildContext::check_function_call(const ParserData& function, Call call) {
	return {};
}

ParserData mint::BuildContext::check_method_call(const ParserData& method, Call call) {
	return {};
}

ParserData mint::BuildContext::check_operation(UnaryOperation op, const ParserData& operand) const {
	switch (op) {
	case UnaryOperation::inc_operator:
	case UnaryOperation::dec_operator:
	case UnaryOperation::not_operator:
	case UnaryOperation::compl_operator:
	case UnaryOperation::pos_operator:
	case UnaryOperation::neg_operator:
	case UnaryOperation::typeof_operator:
	case UnaryOperation::membersof_operator:
	case UnaryOperation::defined_operator:
	case UnaryOperation::await_operator:
		break;
	}
	return {};
}

ParserData mint::BuildContext::check_operation(BinaryOperation op, const ParserData& lhs, const ParserData& rhs) const {
	switch (op) {
	case BinaryOperation::move_operator:
	case BinaryOperation::copy_operator:
	case BinaryOperation::add_operator:
	case BinaryOperation::sub_operator:
	case BinaryOperation::mul_operator:
	case BinaryOperation::div_operator:
	case BinaryOperation::pow_operator:
	case BinaryOperation::mod_operator:
	case BinaryOperation::is_operator:
	case BinaryOperation::in_operator:
	case BinaryOperation::eq_operator:
	case BinaryOperation::ne_operator:
	case BinaryOperation::lt_operator:
	case BinaryOperation::gt_operator:
	case BinaryOperation::le_operator:
	case BinaryOperation::ge_operator:
	case BinaryOperation::and_operator:
	case BinaryOperation::or_operator:
	case BinaryOperation::band_operator:
	case BinaryOperation::bor_operator:
	case BinaryOperation::xor_operator:
	case BinaryOperation::shift_left_operator:
	case BinaryOperation::shift_right_operator:
	case BinaryOperation::inclusive_range_operator:
	case BinaryOperation::exclusive_range_operator:
	case BinaryOperation::subscript_operator:
	case BinaryOperation::regex_match_operator:
	case BinaryOperation::regex_unmatch_operator:
	case BinaryOperation::strict_eq_operator:
	case BinaryOperation::strict_ne_operator:
		break;
	}
	return {};
}

ParserData mint::BuildContext::check_operation(TernaryOperation op, const ParserData& first, const ParserData& second,
    const ParserData& third) const {
	switch (op) {
	case TernaryOperation::conditional_operator:
		if (!match_type(second.get_type(), third.get_type())) {
			// parse_error();
		}
		return first.get_type();
	case TernaryOperation::subscript_move_operator:
		break;
	}
	return {};
}

Compiler& mint::BuildContext::compiler() {
	return _compiler;
}

std::string BuildContext::read_regex() {
	return _lexer.read_regex();
}

std::pair<int, std::string> BuildContext::next_token() {

	if (_lexer.at_end()) {
		return {parser::token::file_end_token, {}};
	}

	auto [token, type] = _lexer.next_token();
	while (type == parser::token::comment_token || type == parser::token::no_line_end_token) {
		std::tie(token, type) = _lexer.next_token();
	}
	return {type, token};
}

void BuildContext::parse_error(const std::string& error_msg) const {
	fflush(stdout);
	error("{}", _lexer.format_error(error_msg));
}

void BuildContext::push_branch(Branch& branch) {
	_branches.push(_branch);
	_branch = std::ref(branch);
}

void BuildContext::pop_branch() {
	_branch = _branches.top();
	_branches.pop();
}

Block* BuildContext::current_breakable_block() {
	const auto& current_stack = current_context().blocks;
	const auto it = std::ranges::find_if(std::views::reverse(current_stack), [](const auto& block) {
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
	const auto it = std::ranges::find_if(std::views::reverse(current_stack), [](const auto& block) {
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
	if (_functions.empty()) {
		return *_module;
	}
	return *_functions.top();
}

const Context& BuildContext::current_context() const {
	if (_functions.empty()) {
		return *_module;
	}
	return *_functions.top();
}

FunctionDefinition* BuildContext::current_function_definition() {
	if (_functions.empty()) {
		return nullptr;
	}
	return _functions.top().get();
}

const FunctionDefinition* BuildContext::current_function_definition() const {
	if (_functions.empty()) {
		return nullptr;
	}
	return _functions.top().get();
}

std::size_t BuildContext::find_fast_symbol_index(const Symbol& symbol) const {
	if (const auto* def = current_function_definition(); def && def->with_fast) {
		return mint::find_fast_symbol_index(*def, symbol);
	}
	return invalid_index;
}

std::pair<const Symbol*, std::size_t> BuildContext::create_fast_scoped_symbol_index(const std::string& name) {

	const Symbol* symbol = _compiler.get().data().bytecode.make_symbol(Symbol(name));
	Context& context = current_context();

	if (context.condition_scoped_symbols) {
		context.condition_scoped_symbols->emplace_back(symbol);
	}
	else if (context.range_loop_scoped_symbols) {
		context.range_loop_scoped_symbols->emplace_back(symbol);
	}
	else if (!context.blocks.empty()) {
		const auto& block = context.blocks.back();
		block->block_scoped_symbols.push_back(symbol);
	}

	if (auto* def = current_function_definition(); def && def->with_fast) {
		return {symbol, mint::create_fast_symbol_index(*def, *symbol)};
	}

	return {symbol, invalid_index};
}

std::pair<const Symbol*, std::size_t> BuildContext::create_fast_symbol_index(const std::string& name) {

	const Symbol* symbol = _compiler.get().data().bytecode.make_symbol(Symbol(name));

	if (auto* def = current_function_definition(); def && def->with_fast) {
		return {symbol, mint::create_fast_symbol_index(*def, *symbol)};
	}

	return {symbol, invalid_index};
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

void mint::BuildContext::save_function_signatures(FunctionDefinition* def) {

	if (def->variadic && def->parameters.empty()) {
		parse_error("expected parameter before '...' token");
	}

	const auto count = static_cast<int>(def->parameters.size());
	def->signatures.push_back({
	    .parameter_count = def->variadic ? ~(count - 1) : count,
	    .handle = _compiler.get().data().bytecode.make_handle(current_package().data(), def->begin_offset),
	});

	while (!def->parameters.empty()) {
		const auto& param = def->parameters.top();
		push_node(Node::Command::init_parameter);
		push_node(param.symbol);
		push_node(param.flags);
		push_node(mint::fast_symbol_index(*def, *param.symbol));
		def->parameters.pop();
	}
}

void mint::BuildContext::resolve_function_description(const FunctionDefinition* def) {

	_compiler.get().data().debug_info.register_function({
	    .description = *def->description,
	    .begin_offset = def->begin_offset,
	    .end_offset = next_offset(),
	});

	for (const auto& signature : def->signatures) {
		signature.handle.get().fast_count = def->fast_symbol_count;
		signature.handle.get().generator = def->generator;
		signature.handle.get().async = def->async;
	}
}

const Reference* mint::BuildContext::make_function(const FunctionDefinition* def) {

	auto* function = _compiler.get().make_data<Function>();
	const auto* constant = _compiler.get().make_constant(def->flags, function);

	for (const auto& signature : def->signatures) {
		if (def->capture) {
			function->mapping.emplace(signature.parameter_count, std::make_unique<Function::Stateful>(signature.handle));
		}
		else {
			function->mapping.emplace(signature.parameter_count,
			    std::make_unique<Function::Stateless>(signature.handle));
		}
	}

	return constant;
}

std::optional<mint::ParserData> mint::BuildContext::find_member(const ParserData& object, const Symbol& symbol) const {
	return std::visit(
	    Overloaded {
	        [&](std::reference_wrapper<const PackageDescription> description) -> std::optional<mint::ParserData> {
		        // TODO: check members
		        // TODO: check global data for UFCS
		        return std::nullopt;
	        },
	        [&](std::reference_wrapper<const ClassDescription> description) -> std::optional<mint::ParserData> {
		        // TODO: check members
		        // TODO: check global data for UFCS
		        return std::nullopt;
	        },
	        [&](std::reference_wrapper<const FunctionDescription> /*description*/) -> std::optional<mint::ParserData> {
		        // TODO: check global data for UFCS
		        return std::nullopt;
	        },
	        [&](std::reference_wrapper<const VariableDescription> description) -> std::optional<mint::ParserData> {
		        if (description.get().type.is_undefined()) {
			        return TypeAnnotation();
		        }
		        // TODO: check variable type members
		        // TODO: check global data for UFCS
		        return std::nullopt;
	        },
	        [](const TypeAnnotation& type) -> std::optional<mint::ParserData> {
		        if (type.is_undefined()) {
			        return TypeAnnotation();
		        }
		        return std::nullopt;
	        },
	    },
	    object);
}

bool mint::BuildContext::match_type(const TypeAnnotation& target, const TypeAnnotation& source) const {
	return std::visit(mint::Overloaded {
	                      [](std::monostate) -> bool {
		                      return true;
	                      },
	                      [&](const PrimitiveType& primitive) -> bool {
		                      if (const auto* source_primitive = std::get_if<PrimitiveType>(&source.definition)) {
			                      return primitive.kind == source_primitive->kind;
		                      }
		                      return false;
	                      },
	                      [&](const BuiltinType& builtin) -> bool {
		                      if (const auto* source_builtin = std::get_if<BuiltinType>(&source.definition)) {
			                      return builtin.kind == source_builtin->kind;
		                      }
		                      return false;
	                      },
	                      [&](const UserType& user_type) -> bool {
		                      if (const auto* source_user_type = std::get_if<UserType>(&source.definition)) {
			                      // return root_scope.locate(user_type.path).is_derived_from(source_user_type->path);
		                      }
		                      return false;
	                      },
	                      [&](const UnionType& union_type) -> bool {
		                      return std::ranges::any_of(union_type.elements, [&](const auto& source) {
			                      return match_type(target, source);
		                      });
	                      },
	                  },
	    target.definition);
}
