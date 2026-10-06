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
#include "mint/debug/debug_tools.h"
#include "mint/memory/builtin/string.h"
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
#include "mint/system/stdio.h"
#include "mint/system/utf8.h"
#include "parser.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace mint;

namespace {

Class::Operator to_operator(UnaryOperation op) {
	switch (op) {
	case UnaryOperation::inc_operator:
		return Class::inc_operator;
	case UnaryOperation::dec_operator:
		return Class::dec_operator;
	case UnaryOperation::not_operator:
		return Class::not_operator;
	case UnaryOperation::compl_operator:
		return Class::compl_operator;
	default:
		std::unreachable();
	}
}

Class::Operator to_operator(BinaryOperation op) {
	switch (op) {
	case BinaryOperation::copy_operator:
		return Class::copy_operator;
	case BinaryOperation::add_operator:
		return Class::add_operator;
	case BinaryOperation::sub_operator:
		return Class::sub_operator;
	case BinaryOperation::mul_operator:
		return Class::mul_operator;
	case BinaryOperation::div_operator:
		return Class::div_operator;
	case BinaryOperation::pow_operator:
		return Class::pow_operator;
	case BinaryOperation::mod_operator:
		return Class::mod_operator;
	case BinaryOperation::in_operator:
		return Class::in_operator;
	case BinaryOperation::eq_operator:
		return Class::eq_operator;
	case BinaryOperation::ne_operator:
		return Class::ne_operator;
	case BinaryOperation::lt_operator:
		return Class::lt_operator;
	case BinaryOperation::gt_operator:
		return Class::gt_operator;
	case BinaryOperation::le_operator:
		return Class::le_operator;
	case BinaryOperation::ge_operator:
		return Class::ge_operator;
	case BinaryOperation::and_operator:
		return Class::and_operator;
	case BinaryOperation::or_operator:
		return Class::or_operator;
	case BinaryOperation::band_operator:
		return Class::band_operator;
	case BinaryOperation::bor_operator:
		return Class::bor_operator;
	case BinaryOperation::xor_operator:
		return Class::xor_operator;
	case BinaryOperation::shift_left_operator:
		return Class::shift_left_operator;
	case BinaryOperation::shift_right_operator:
		return Class::shift_right_operator;
	case BinaryOperation::inclusive_range_operator:
		return Class::inclusive_range_operator;
	case BinaryOperation::exclusive_range_operator:
		return Class::exclusive_range_operator;
	case BinaryOperation::subscript_operator:
		return Class::subscript_operator;
	case BinaryOperation::regex_match_operator:
		return Class::regex_match_operator;
	case BinaryOperation::regex_unmatch_operator:
		return Class::regex_unmatch_operator;
	case BinaryOperation::strict_eq_operator:
		return Class::eq_operator;
	case BinaryOperation::strict_ne_operator:
		return Class::ne_operator;
	default:
		std::unreachable();
	}
}

BuildContext::Call to_call(const ParserData& data) {
	return {
	    .parameters = {data},
	};
}

std::size_t required_parameter_count(int signature) {
	return (signature < 0) ? static_cast<std::size_t>(~signature) : static_cast<std::size_t>(signature);
}

ParserData create_alias(auto& scope, const VariableDescription& target_variable, const ParserData& value) {
	return std::visit(Overloaded {
	                      [&](const PackageDescription& description) -> ParserData {
		                      return scope.create_alias(target_variable, description);
	                      },
	                      [&](const ClassDescription& description) -> ParserData {
		                      return scope.create_alias(target_variable, description);
	                      },
	                      [&](const FunctionDefinition& description) -> ParserData {
		                      return scope.create_alias(target_variable, description);
	                      },
	                      [&](const VariableDescription& description) -> ParserData {
		                      return scope.update_variable(target_variable, description.type);
	                      },
	                      [&](const AliasDescription& description) -> ParserData {
		                      return create_alias(scope, target_variable, unwrap_alias(description));
	                      },
	                      [&](const OverriddenVariable& data) -> ParserData {
		                      return scope.update_variable(target_variable, data.type);
	                      },
	                      [&](const TemporaryData& data) -> ParserData {
		                      return scope.update_variable(target_variable, data.type);
	                      },
	                      [](const DeferredCheck&) -> ParserData {
		                      std::unreachable();
	                      },
	                  },
	    value);
}

}

BuildContext::BuildContext(DataStream& stream, Compiler& compiler) :
    _compiler(compiler),
    _stream(stream),
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

void BuildContext::commit_expr_result(const ParserData& data) {

	if (data.is_deferred()) {
		_deferred_checks.emplace_back(std::get<DeferredCheck>(data));
	}

	Context& context = current_context();
	if (context.meta_blocks.empty()) {
		push_node(Node::Command::unload_reference);
	}
	else {
		switch (context.meta_blocks.back()) {
		case Context::MetaBlock::printer:
			push_node(Node::Command::print);
			break;
		case Context::MetaBlock::generator_expression:
			push_node(Node::Command::unload_reference);
			break;
		}
	}
}

void BuildContext::resolve_deferred_checks() const {
	for (const auto& check : _deferred_checks) {
		(void)resolve_deferred_check(check);
	}
}

std::size_t BuildContext::fast_symbol_index(const std::string& name) {
	const auto symbol = Symbol(name);
	if (auto* def = current_function_context(); def && def->with_fast) {
		if (const auto index = mint::find_fast_symbol_index(*def, symbol); index != invalid_index) {
			return index;
		}
		if (def->description) {
			if (auto definition = def->description->locate(symbol); definition.is_valid()) {
				return mint::create_fast_symbol_index(*def, std::move(definition));
			}
			if (const auto* owner = def->description->get_owner_scope()) {
				if (auto definition = owner->get_root_scope().locate(symbol); definition.is_valid()) {
					return mint::create_fast_symbol_index(*def, std::move(definition));
				}
			}
		}
		if (auto definition = global_data_description().locate(symbol); definition.is_valid()) {
			return mint::create_fast_symbol_index(*def, std::move(definition));
		}
		return mint::create_fast_variable_index(*def, mint::create_variable_definition(*def, {.name = symbol}));
	}
	return invalid_index;
}

void mint::BuildContext::load_module(const std::string& module_path) {

	if (_functions.empty() && _module->blocks.empty()) {
		try {
			_compiler.get().program().load_module(module_path);
		}
		catch (...) {
			const auto line_number = _stream.get().position().line - 1;
			const auto module_file_path = _stream.get().path().generic_string();
			const auto module_name = _compiler.get().data().description.get_path().to_string();
			mint::print(stderr, std::format("  from {} ({}:{})\n", module_name, module_file_path, line_number));
			mint::print(stderr, std::format("    {}\n", _stream.get().get_line(line_number)));
			throw;
		}
	}

	push_node(Node::Command::load_module);
	push_node(Symbol(module_path));
}

ParserData mint::BuildContext::create_fast_scoped_symbol(const std::string& name, TypeAnnotation type,
    Reference::Flags flags) {

	const auto* symbol = _compiler.get().data().bytecode.make_symbol(Symbol(name));
	const auto [index, description] = create_fast_scoped_symbol_index(symbol, std::move(type), flags);

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

	const auto* symbol = _compiler.get().data().bytecode.make_symbol(Symbol(name));
	const auto [index, description] = create_fast_symbol_index(*symbol, std::move(type), flags);

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

	return find_symbol(Symbol(name), index);
}

void BuildContext::open_block(BlockType type) {

	Context& context = current_context();
	auto block = std::unique_ptr<Block>();

	switch (type) {
	case BlockType::conditional_loop_type:
		{
			check_expression(context.current_condition->condition_result);
			auto loop_block = std::make_unique<ConditionalLoopBlock>(context);
			loop_block->backward = _branch.get().next_jump_backward();
			loop_block->forward = _branch.get().next_jump_forward();
			block = std::move(loop_block);
		}
		break;
	case BlockType::custom_range_loop_type:
		{
			check_expression(context.current_condition->condition_result);
			auto loop_block = std::make_unique<CustomRangeLoopBlock>(context);
			loop_block->backward = _branch.get().next_jump_backward();
			loop_block->forward = _branch.get().next_jump_forward();
			block = std::move(loop_block);
		}
		break;
	case BlockType::range_loop_type:
		{
			check_expression(context.current_condition->condition_result);
			auto loop_block = std::make_unique<RangeLoopBlock>(context);
			loop_block->backward = _branch.get().next_jump_backward();
			loop_block->forward = _branch.get().next_jump_forward();
			block = std::move(loop_block);
		}
		break;
	case BlockType::switch_type:
		{
			check_expression(context.current_condition->condition_result);
			auto switch_block = std::make_unique<SwitchBlock>(context);
			push_node(Node::Command::jump);
			switch_block->case_table.origin = _branch.get().next_node_offset();
			push_node(0);
			switch_block->forward = _branch.get().start_empty_jump_forward();
			block = std::move(switch_block);
		}
		break;
	case BlockType::if_type:
		check_expression(context.current_condition->condition_result);
		block = std::make_unique<IfBlock>(context);
		break;
	case BlockType::elif_type:
		check_expression(context.current_condition->condition_result);
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
	reset_scoped_symbols(context.blocks.back()->scoped_symbols);
}

void BuildContext::reset_scoped_symbols_until(BlockType type) {
	Context& context = current_context();
	for (const auto& block : std::views::reverse(context.blocks)) {
		reset_scoped_symbols(block->scoped_symbols);
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
	const auto block = std::move(context.blocks.back());

	if (const auto* symbols = block->get_condition_scoped_symbols()) {
		reset_scoped_symbols(*symbols);
	}

	if (const auto* symbols = block->get_range_loop_scoped_symbols()) {
		reset_scoped_symbols(*symbols);
	}

	context.blocks.pop_back();

	if (context.blocks.empty()) {
		if (auto* def = current_function_context(); def && !def->terminated) {
			def->terminated = block->is_terminated();
		}
	}
	else {
		context.blocks.back()->set_terminated(block->is_terminated());
	}
}

bool BuildContext::has_terminated() const {
	if (const auto* def = current_function_context()) {
		return def->terminated;
	}
	return false;
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
	if (const auto* def = current_function_context()) {
		return def->generator;
	}
	return false;
}

bool BuildContext::is_in_generator_expression() const {
	const auto& context = current_context();
	if (context.meta_blocks.empty()) {
		return false;
	}
	return context.meta_blocks.back() == Context::MetaBlock::generator_expression;
}

void BuildContext::prepare_continue() {

	if (const auto* block = current_breakable_block()) {

		for (std::size_t i = 0; i < block->retrieve_point_count; ++i) {
			push_node(Node::Command::unset_retrieve_point);
		}

		const auto& context = current_context();
		const auto& children = context.blocks;

		for (auto child = children.rbegin(); child != children.rend() && child->get() != block; ++child) {
			reset_scoped_symbols((*child)->scoped_symbols);
		}

		reset_scoped_symbols(block->scoped_symbols);
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
			reset_scoped_symbols((*child)->scoped_symbols);
		}

		reset_scoped_symbols(block->scoped_symbols);
	}
}

void BuildContext::prepare_return() {

	if (auto* def = current_function_context()) {

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
			def->terminated = true;
		}
		else {
			def->blocks.back()->set_terminated(true);
		}
	}
}

void mint::BuildContext::prepare_raise() {
	if (auto* def = current_function_context()) {
		if (def->blocks.empty()) {
			def->terminated = true;
		}
		else {
			def->blocks.back()->set_terminated(true);
		}
	}
}

void BuildContext::register_retrieve_point() {

	if (auto* definition = current_function_context()) {
		definition->retrieve_point_count++;
	}
	if (Block* block = current_breakable_block()) {
		block->retrieve_point_count++;
	}
}

void BuildContext::unregister_retrieve_point() {

	if (auto* definition = current_function_context()) {
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
		switch_block->reset_type_overrides();
	}
}

void mint::BuildContext::add_to_case_label(const std::string& token) {
	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {
		switch_block->case_table.current_case->first.tokens.push_back(token);
	}
}

void BuildContext::resolve_case_label(BinaryOperation op, const ParserData& data) {
	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {
		auto case_label = std::exchange(switch_block->case_table.current_case, std::nullopt);
		if (const auto labels = std::views::keys(switch_block->case_table.cases);
		    std::ranges::find(labels, case_label->first) != labels.end()) {
			parse_error("duplicate case value");
		}
		const auto& condition_data = switch_block->condition_context->condition_result;
		try_extract_type_narrowing(*switch_block->condition_context, condition_data, data);
		(void)check_operation(op, condition_data, data);
		switch_block->case_table.cases.emplace_back(*case_label);
		switch_block->setup_type_overrides();
		pop_branch();
	}
}

void BuildContext::set_default_label() {
	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {
		if (switch_block->case_table.default_case) {
			parse_error("multiple default labels in one switch");
		}
		switch_block->case_table.default_case = CaseTable::DefaultCase {
		    .offset = _branch.get().next_node_offset(),
		};
		switch_block->reset_type_overrides();
	}
}

void BuildContext::build_case_table() {

	if (auto* switch_block = dynamic_cast<SwitchBlock*>(current_breakable_block())) {

		_branch.get().replace_node(switch_block->case_table.origin, static_cast<int>(_branch.get().next_node_offset()));

		for (auto& case_label : std::views::values(switch_block->case_table.cases)) {
			push_node(Node::Command::reload_reference);
			case_label.condition.build();
			push_node(Node::Command::case_jump);
			push_node(case_label.offset);
		}

		if (const auto default_case = switch_block->case_table.default_case) {
			push_node(Node::Command::load_constant);
			push_node(_compiler.get().make_constant<Boolean>(true));
			push_node(Node::Command::case_jump);
			push_node(default_case->offset);
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

GlobalDataDescription& mint::BuildContext::global_data_description() const {
	return _compiler.get().program().global_data_description();
}

ModuleDescription& mint::BuildContext::module_description() const {
	return _compiler.get().data().description;
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
		ClassDescription& owner = context.classes.back().description;
		current_context().classes.emplace_back(owner.create_class(symbol, flags), flags);
	}
	else if (flags & Reference::global) {
		const auto module_id = _compiler.get().data().id;
		current_context().classes.emplace_back(current_package().create_class(module_id, symbol, flags), flags);
	}
	else if (const auto* def = current_function_context()) {
		current_context().classes.emplace_back(def->description->create_class(symbol), flags);
	}
	else {
		current_context().classes.emplace_back(module_description().create_class(symbol), flags);
	}
}

void BuildContext::append_symbol_to_base_class_path(const std::string& symbol) {
	_class_base.emplace_back(symbol);
}

void BuildContext::save_base_class_path() {
	current_context().classes.back().description.get().add_base(std::exchange(_class_base, {}));
}

void BuildContext::resolve_class_description() {

	auto& context = current_context();
	auto [desc, flags] = context.classes.back();
	context.classes.pop_back();

	if (!desc.get().has_any_member(builtin_symbols::new_method)) {
		desc.get()
		    .create_method(builtin_symbols::new_method, desc.get().get_path())
		    .add_parameter({.name = "self", .type = desc.get().get_path()});
	}

	if (context.classes.empty()) {
		push_node(Node::Command::declare_class);
		push_node(&desc.get());
		push_node(flags);
	}
}

void BuildContext::create_attribute(const std::string& name, TypeAnnotation type, Reference::Flags flags, Data* value) {
	ClassDescription& class_description = current_context().classes.back().description;
	class_description.create_attribute(Symbol(name), std::move(type), flags,
	    _compiler.get().make_constant(flags, value));
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
	_functions.emplace(std::make_unique<FunctionContext>(FunctionContext {
	    .enclosing = current_function_context(),
	    .begin_offset = _branch.get().next_node_offset(),
	}));
}

void BuildContext::start_async_function_description() {
	_functions.emplace(std::make_unique<FunctionContext>(FunctionContext {
	    .enclosing = current_function_context(),
	    .begin_offset = _branch.get().next_node_offset(),
	    .async = true,
	}));
}

void BuildContext::add_parameter(const std::string& name, TypeAnnotation type, Reference::Flags flags) {

	auto* def = current_function_context();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	def->parameters.push_back({
	    .symbol = _compiler.get().data().bytecode.make_symbol(Symbol(name)),
	    .type = std::move(type),
	    .flags = flags,
	});
}

void BuildContext::set_exit_point(const ParserData& data) {
	auto* def = current_function_context();
	if (data.is_deferred() || locate_data(data.get_type()).is_deferred()
	    || locate_data(def->description->get_return_type()).is_deferred()) {
		_deferred_checks.emplace_back(defer_check(DeferredReturnCheck {
		    .expected = def->description->get_return_type(),
		    .actual = std::make_shared<ParserData>(data),
		}));
	}
	else {
		check_return_type(def->description->get_return_type(), data, _lexer.position());
	}
	def->exit_points.emplace_back(_branch.get().next_node_offset());
}

void BuildContext::set_generator() {

	auto* def = current_function_context();
	assert(def);

	for (const auto exit_point : def->exit_points) {
		_branch.get().replace_node(exit_point, Node::Command::yield_exit_generator);
	}

	def->generator = true;
}

void BuildContext::set_variadic() {

	auto* def = current_function_context();
	assert(def);

	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	def->parameters.push_back({
	    .symbol = _compiler.get().data().bytecode.make_symbol("va_args"),
	    .type = BuiltinTypeKind::iterator,
	    .flags = Reference::default_flags,
	});
	def->variadic = true;

	if (!def->signatures.empty()) {
		push_node(Node::Command::init_iterator);
		push_node(0);
	}
}

ParserData BuildContext::save_method_signatures(const std::string& name, TypeAnnotation return_type,
    Reference::Flags flags) {

	auto* def = current_function_context();
	assert(def != nullptr);

	save_function_signatures(def);

	const auto symbol = Symbol(name);
	if (auto* enclosing = def->enclosing) {
		ClassDescription& owner = enclosing->classes.back().description;
		if (return_type.is_undefined() && symbol == builtin_symbols::new_method) {
			return_type = owner.get_path();
		}
		def->description = &owner.create_method(symbol, std::move(return_type), flags, make_function(def, flags));
	}
	else {
		ClassDescription& owner = _module->classes.back().description;
		if (return_type.is_undefined() && symbol == builtin_symbols::new_method) {
			return_type = owner.get_path();
		}
		def->description = &owner.create_method(symbol, std::move(return_type), flags, make_function(def, flags));
	}

	if (!(flags & Reference::global)) {
		const auto* self_type = def->description->get_owner_class();
		auto& self_parameter = def->parameters.front();
		if (self_parameter.type.is_undefined()) {
			self_parameter.type = self_type->get_path();
		}
		else if (self_parameter.type != self_type->get_path()) {
			parse_error(std::format("invalid type for parameter '{}': expected '{}', got '{}'",
			    self_parameter.symbol->str(), self_type->get_path().to_string(), self_parameter.type.to_string()));
		}
	}

	save_function_parameters(def);
	return *def->description;
}

ParserData mint::BuildContext::save_method_signatures(Class::Operator op, TypeAnnotation return_type,
    Reference::Flags flags) {

	auto* def = current_function_context();
	assert(def != nullptr);

	save_function_signatures(def);

	if (auto* enclosing = def->enclosing) {
		def->description = &enclosing->classes.back().description.get().create_method(get_operator_symbol(op),
		    std::move(return_type), flags, make_function(def, flags));
	}
	else {
		def->description = &_module->classes.back().description.get().create_method(get_operator_symbol(op),
		    std::move(return_type), flags, make_function(def, flags));
	}

	if (!(flags & Reference::global)) {
		const auto* self_type = def->description->get_owner_class();
		auto& self_parameter = def->parameters.front();
		if (self_parameter.type.is_undefined()) {
			self_parameter.type = self_type->get_path();
		}
		else if (self_parameter.type != self_type->get_path()) {
			parse_error(std::format("invalid type for parameter '{}': expected '{}', got '{}'",
			    self_parameter.symbol->str(), self_type->get_path().to_string(), self_parameter.type.to_string()));
		}
	}

	save_function_parameters(def);
	return *def->description;
}

ParserData BuildContext::save_function_signatures(const std::string& name, TypeAnnotation return_type,
    Reference::Flags flags) {

	auto* def = current_function_context();
	assert(def != nullptr);

	save_function_signatures(def);

	if (flags & Reference::global) {
		const auto module_id = _compiler.get().data().id;
		def->description = &current_package().create_function(module_id, Symbol(name), std::move(return_type), flags);
	}
	else if (const auto* parent = def->enclosing) {
		def->description = &parent->description->create_function(Symbol(name), std::move(return_type), flags);
	}
	else {
		def->description = &compiler().data().description.create_function(Symbol(name), std::move(return_type), flags);
	}

	save_function_parameters(def);
	return *def->description;
}

ParserData mint::BuildContext::save_function_signatures(TypeAnnotation return_type, Reference::Flags flags) {

	auto* def = current_function_context();
	assert(def != nullptr);

	save_function_signatures(def);

	if (flags & Reference::global) {
		const auto module_id = _compiler.get().data().id;
		def->description = &current_package().create_function(module_id, std::move(return_type), flags);
	}
	else if (const auto* parent = def->enclosing; parent && parent->description) {
		def->description = &parent->description->create_function(std::move(return_type), flags);
	}
	else {
		def->description = &compiler().data().description.create_function(std::move(return_type), flags);
	}

	save_function_parameters(def);
	return *def->description;
}

void BuildContext::add_function_signature() {

	auto* def = current_function_context();
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

	const auto* def = current_function_context();
	assert(def != nullptr);

	resolve_function_description(def);

	assert(def->blocks.empty());
	_functions.pop();
}

void BuildContext::resolve_function_description() {

	auto* def = current_function_context();
	assert(def != nullptr);

	resolve_function_description(def);

	push_node(Node::Command::load_constant);
	push_node(make_function(def, def->description->get_flags()));

	if (def->capture) {
		def->capture->build();
	}

	assert(def->blocks.empty());
	_functions.pop();
}

void mint::BuildContext::register_function_description() {

	const auto* def = current_function_context();
	assert(def);

	push_node(Node::Command::declare_function);
	push_node(def->description->get_name());
	push_node(def->description->get_flags());
}

void BuildContext::start_call() {
	_calls.emplace();
}

void BuildContext::add_to_call(ParserData parameter) {
	_calls.top().parameters.emplace_back(std::move(parameter));
}

void mint::BuildContext::add_to_call_unpacked(ParserData parameter) {
	_calls.top().parameters.emplace_back(std::move(parameter));
	_calls.top().with_unpack++;
}

BuildContext::Call BuildContext::resolve_call() {
	auto call = std::move(_calls.top());
	push_node(static_cast<int>(call.parameters.size()));
	_calls.pop();
	return call;
}

void BuildContext::start_capture() {
	auto* def = current_function_context();
	def->capture.emplace(_branch);
	def->with_fast = false;
	push_branch(*def->capture);
	push_node(Node::Command::init_capture);
}

void BuildContext::resolve_capture() {
	auto* def = current_function_context();
	def->with_fast = true;
	pop_branch();
}

void BuildContext::capture_as(const std::string& symbol, TypeAnnotation type) {

	auto* def = current_function_context();
	assert(def != nullptr);

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	const auto& parameter = *def->capture_parameters.emplace_back(
	    std::make_unique<VariableDescription>(VariableDescription {
	        .name = Symbol(symbol),
	        .type = std::move(type),
	    }));

	push_node(Node::Command::capture_as);
	push_node(parameter.name);
}

void BuildContext::capture(const std::string& symbol) {

	const auto* def = current_function_context();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
	}

	push_node(Node::Command::capture_symbol);
	push_node(Symbol(symbol));
}

void BuildContext::capture_all() {

	auto* def = current_function_context();

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
	current_context().meta_blocks.push_back(Context::MetaBlock::generator_expression);
}

void BuildContext::close_generator_expression() {
	assert(current_context().meta_blocks.back() == Context::MetaBlock::generator_expression);
	current_context().meta_blocks.pop_back();
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
	current_context().meta_blocks.push_back(Context::MetaBlock::printer);
}

void BuildContext::close_printer() {
	assert(current_context().meta_blocks.back() == Context::MetaBlock::printer);
	current_context().meta_blocks.pop_back();
	push_node(Node::Command::close_printer);
}

void BuildContext::force_printer() {
	current_context().meta_blocks.push_back(Context::MetaBlock::printer);
}

void BuildContext::start_range_loop() {
	Context& context = current_context();
	context.current_range_loop = std::make_unique<RangeLoopContext>();
}

void BuildContext::resolve_range_loop() {}

void BuildContext::start_condition() {
	Context& context = current_context();
	context.current_condition = std::make_unique<ConditionContext>();
}

void BuildContext::resolve_condition(ParserData data) {
	const Context& context = current_context();
	context.current_condition->condition_result = std::move(data);
}

void BuildContext::open_sub_branch() {
	Context& context = current_context();
	context.branches.emplace_back(_branch);
	push_branch(context.branches.back());
}

void BuildContext::build_sub_branch() {
	Context& context = current_context();
	SubBranch branch = std::move(context.branches.back());
	context.branches.pop_back();
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
	if (object.is_deferred()) {
		return defer_check(DeferredMemberCheck {
		    .object = std::make_shared<ParserData>(object),
		    .member = Symbol(member),
		});
	}
	return find_member(object, Symbol(member));
}

ParserData mint::BuildContext::get_operator(const ParserData& object, Class::Operator op) const {
	return get_member(object, get_operator_symbol(op).str());
}

ParserData mint::BuildContext::check_function_call(const ParserData& function, const Call& call) const {
	if (function.is_deferred() || locate_data(function.get_type()).is_deferred()
	    || std::ranges::any_of(call.parameters, [&](const ParserData& parameter) {
		       return parameter.is_deferred() || locate_data(parameter.get_type()).is_deferred();
	       })) {
		return defer_check(DeferredFunctionCallCheck {
		    .function = std::make_shared<ParserData>(function),
		    .parameters = call.parameters,
		    .with_unpack = call.with_unpack,
		});
	}
	return check_function_call(function, call, _lexer.position());
}

ParserData mint::BuildContext::check_method_call(const ParserData& object, const std::string& member,
    const Call& call) const {
	if (object.is_deferred() || locate_data(object.get_type()).is_deferred()
	    || std::ranges::any_of(call.parameters, [&](const ParserData& parameter) {
		       return parameter.is_deferred() || locate_data(parameter.get_type()).is_deferred();
	       })) {
		return defer_check(DeferredMethodCallCheck {
		    .object = std::make_shared<ParserData>(object),
		    .method = Symbol(member),
		    .parameters = call.parameters,
		    .with_unpack = call.with_unpack,
		});
	}
	return check_method_call(object, Symbol(member), call, _lexer.position());
}

ParserData mint::BuildContext::check_operator_call(const ParserData& object, Class::Operator op,
    const Call& call) const {
	if (object.is_deferred() || locate_data(object.get_type()).is_deferred()
	    || std::ranges::any_of(call.parameters, [&](const ParserData& parameter) {
		       return parameter.is_deferred() || locate_data(parameter.get_type()).is_deferred();
	       })) {
		return defer_check(DeferredMethodCallCheck {
		    .object = std::make_shared<ParserData>(object),
		    .method = get_operator_symbol(op),
		    .parameters = call.parameters,
		    .with_unpack = call.with_unpack,
		});
	}
	return check_method_call(object, get_operator_symbol(op), call, _lexer.position());
}

void mint::BuildContext::check_expression(const ParserData& result) {
	if (result.is_deferred()) {
		_deferred_checks.emplace_back(std::get<DeferredCheck>(result));
	}
}

ParserData mint::BuildContext::check_init_iterator(const Call& call) const {
	if (std::ranges::any_of(call.parameters, [&](const ParserData& parameter) {
		    return parameter.is_deferred() || locate_data(parameter.get_type()).is_deferred();
	    })) {
		// TODO: handle deferred
	}
	return BuiltinTypeKind::iterator;
}

ParserData mint::BuildContext::check_init_symbol(const ParserData& target, const ParserData& value) const {
	const auto lookup_context = LookupContext {
	    .package = current_package(),
	    .function = current_function_description(),
	};
	if (target.is_deferred() || value.is_deferred() || locate_data(target.get_type()).is_deferred()
	    || locate_data(value.get_type()).is_deferred()) {
		return defer_check(DeferredInitSymbolCheck {
		    .target = std::make_shared<ParserData>(target),
		    .value = std::make_shared<ParserData>(value),
		    .lookup_context = lookup_context,
		});
	}
	return check_init_symbol(lookup_context, target, value, _lexer.position());
}

ParserData mint::BuildContext::check_operation(UnaryOperation op, const ParserData& operand) const {
	if (operand.is_deferred() || locate_data(operand.get_type()).is_deferred()) {
		return defer_check(DeferredUnaryOperationCheck {
		    .operation = op,
		    .operand = std::make_shared<ParserData>(operand),
		});
	}
	return check_operation(op, operand, _lexer.position());
}

ParserData mint::BuildContext::check_operation(BinaryOperation op, const ParserData& lhs, const ParserData& rhs) const {
	if (lhs.is_deferred() || rhs.is_deferred() || locate_data(lhs.get_type()).is_deferred()
	    || locate_data(rhs.get_type()).is_deferred()) {
		return defer_check(DeferredBinaryOperationCheck {
		    .operation = op,
		    .lhs = std::make_shared<ParserData>(lhs),
		    .rhs = std::make_shared<ParserData>(rhs),
		});
	}
	return check_operation(op, lhs, rhs, _lexer.position());
}

ParserData mint::BuildContext::check_operation(TernaryOperation op, const ParserData& first, const ParserData& second,
    const ParserData& third) const {
	if (first.is_deferred() || second.is_deferred() || third.is_deferred()
	    || locate_data(first.get_type()).is_deferred() || locate_data(second.get_type()).is_deferred()
	    || locate_data(third.get_type()).is_deferred()) {
		return defer_check(DeferredTernaryOperationCheck {
		    .operation = op,
		    .first = std::make_shared<ParserData>(first),
		    .second = std::make_shared<ParserData>(second),
		    .third = std::make_shared<ParserData>(third),
		});
	}
	return check_operation(op, first, second, third, _lexer.position());
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
	parse_error(error_msg, _lexer.position());
}

void BuildContext::parse_error(const std::string& error_msg, const DataStream::Position& position) const {

	fflush(stdout);

	const auto [line_number, error_location] = line_error(position);
	error("{}:{} {}\n{}", _stream.get().path().generic_string(), line_number, error_msg, error_location);
}

std::pair<std::size_t, std::string> BuildContext::line_error(const DataStream::Position& position) const {

	// TODO: push class/function declaration before the block's bytecode for a better presentation

	const auto line_number = position.offset > 1 ? position.line : std::max(position.line - 1, std::size_t {1});
	auto source_line = _stream.get().get_line(line_number);
	const auto caret_offset = position.offset > 1 ? std::min(position.offset - 2, source_line.size())
	                                              : source_line.size();

	auto line = std::move(source_line);
	if (line.empty() || line.back() != '\n') {
		line += '\n';
	}

	for (std::size_t i = 0; i < caret_offset; ++i) {
		const auto ch = static_cast<char8_t>(line.at(i));
		if (ch == '\t') {
			line += '\t';
		}
		else {
			i += utf8_code_point_length(ch) - 1;
			line += ' ';
		}
	}

	line += '^';
	return {line_number, line};
}

std::string mint::BuildContext::list_candidates(const FunctionDefinition& function) const {
	auto message = std::string();
	const auto module_id = _compiler.get().data().id;
	for (const FunctionDescription& description : function) {
		if (module_id == description.get_module_id()) {
			const auto& debug_info = _compiler.get().data().debug_info;
			if (const auto* info = debug_info.find_function_from_description(description)) {
				const auto line_number = debug_info.line_number(info->begin_offset);
				message += std::format("{}:{} note: candidate declared here:\n", _stream.get().path().generic_string(),
				    line_number);
				message += std::format("{}\n", _stream.get().get_line(line_number));
			}
		}
		else if (const auto* module = _compiler.get().program().find_module(description.get_module_id())) {
			if (const auto* info = module->debug_info.find_function_from_description(description)) {
				const auto line_number = module->debug_info.line_number(info->begin_offset);
				const auto module_path = module->description.get_path().to_string();
				message += std::format("{}:{} note: candidate declared here:\n",
				    to_system_path(module_path).generic_string(), line_number);
				message += std::format("{}\n", get_module_line(module_path, line_number));
			}
		}
	}
	if (!message.empty()) {
		return std::format("\n\n{}", message);
	}
	return {};
}

void BuildContext::push_branch(Branch& branch) {
	_branches.push(_branch);
	_branch = std::ref(branch);
}

void BuildContext::pop_branch() {
	_branch = _branches.top();
	_branches.pop();
}

Context& BuildContext::current_context() const {
	if (_functions.empty()) {
		return *_module;
	}
	return *_functions.top();
}

FunctionContext* BuildContext::current_function_context() const {
	if (_functions.empty()) {
		return nullptr;
	}
	return _functions.top().get();
}

FunctionDescription* mint::BuildContext::current_function_description() const {
	if (const auto* def = current_function_context()) {
		return def->description;
	}
	return nullptr;
}

BreakableBlock* BuildContext::current_breakable_block() const {
	for (const auto& current_stack = current_context().blocks; const auto& block : std::views::reverse(current_stack)) {
		if (auto* breakable_block = dynamic_cast<BreakableBlock*>(block.get())) {
			return breakable_block;
		}
	}
	return nullptr;
}

ContinuableBlock* BuildContext::current_continuable_block() const {
	for (const auto& current_stack = current_context().blocks; const auto& block : std::views::reverse(current_stack)) {
		if (auto* continuable_block = dynamic_cast<ContinuableBlock*>(block.get())) {
			return continuable_block;
		}
	}
	return nullptr;
}

std::size_t BuildContext::find_fast_symbol_index(const Symbol& symbol) const {
	if (const auto* def = current_function_context(); def && def->with_fast) {
		return mint::find_fast_symbol_index(*def, symbol);
	}
	return invalid_index;
}

std::pair<std::size_t, const VariableDescription*> BuildContext::create_fast_scoped_symbol_index(const Symbol* symbol,
    TypeAnnotation type, Reference::Flags flags) {

	Context& context = current_context();

	if (const auto& condition = context.current_condition) {
		condition->scoped_symbols.emplace_back(symbol);
	}
	else if (const auto& range_loop = context.current_range_loop) {
		range_loop->scoped_symbols.emplace_back(symbol);
	}
	else if (!context.blocks.empty()) {
		const auto& block = context.blocks.back();
		block->scoped_symbols.push_back(symbol);
	}

	if (flags & Reference::global) {
		return {invalid_index, &current_package().create_variable(*symbol, std::move(type), flags)};
	}

	if (auto* def = current_function_context()) {
		const auto* description = &def->description->create_variable(*symbol, std::move(type), flags);
		if (def->with_fast) {
			return {mint::create_fast_variable_index(*def, *description), description};
		}
		return {invalid_index, description};
	}

	return {invalid_index, &module_description().create_variable(*symbol, std::move(type), flags)};
}

std::pair<std::size_t, const VariableDescription*> BuildContext::create_fast_symbol_index(const Symbol& symbol,
    TypeAnnotation type, Reference::Flags flags) {

	if (flags & Reference::global) {
		return {invalid_index, &current_package().create_variable(symbol, std::move(type), flags)};
	}

	if (auto* def = current_function_context()) {
		const auto* description = &def->description->create_variable(symbol, std::move(type), flags);
		if (def->with_fast) {
			return {mint::create_fast_variable_index(*def, *description), description};
		}
		return {invalid_index, description};
	}

	return {invalid_index, &module_description().create_variable(symbol, std::move(type), flags)};
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

void mint::BuildContext::save_function_signatures(FunctionContext* def) {

	if (def->variadic && def->parameters.empty()) {
		parse_error("expected parameter before '...' token");
	}

	const auto count = static_cast<int>(def->parameters.size());
	def->signatures.push_back({
	    .parameter_count = def->variadic ? ~(count - 1) : count,
	    .handle = _compiler.get().data().bytecode.make_handle(current_package().data(), def->begin_offset),
	});
}

void mint::BuildContext::save_function_parameters(FunctionContext* def) {

	for (const auto& param : std::views::reverse(def->parameters)) {
		push_node(Node::Command::init_parameter);
		push_node(param.symbol);
		push_node(param.flags);
		push_node(mint::fast_symbol_index(*def, {.name = *param.symbol, .type = param.type}));
	}

	for (const auto& parameter : def->capture_parameters) {
		mint::fast_symbol_index(*def, *parameter);
	}

	for (const auto& parameter : def->parameters) {
		const auto& variable = def->description->create_variable(*parameter.symbol, parameter.type, parameter.flags);
		mint::create_fast_variable_index(*def, variable);
		def->description->add_parameter({
		    .name = variable.name,
		    .type = variable.type,
		    .flags = variable.flags,
		});
	}

	def->description->set_required_parameter_count(required_parameter_count(def->signatures.front().parameter_count));
	def->description->set_variadic(def->variadic);
}

void mint::BuildContext::resolve_function_description(const FunctionContext* def) {

	_compiler.get().data().debug_info.register_function({
	    .description = *def->description,
	    .begin_offset = def->begin_offset,
	    .end_offset = next_offset(),
	});

	for (const auto& signature : def->signatures) {
		signature.handle.get().fast_count = def->local_symbols.size();
		signature.handle.get().generator = def->generator;
		signature.handle.get().async = def->async;
	}

	if (!def->terminated) {
		if (locate_data(def->description->get_return_type()).is_deferred()) {
			_deferred_checks.emplace_back(defer_check(DeferredReturnCheck {
			    .expected = def->description->get_return_type(),
			    .actual = std::make_shared<ParserData>(PrimitiveTypeKind::none),
			}));
		}
		else {
			check_return_type(def->description->get_return_type(), PrimitiveTypeKind::none, _lexer.position());
		}
	}
}

const Reference* mint::BuildContext::make_function(const FunctionContext* def, Reference::Flags flags) {

	auto* function = _compiler.get().make_data<Function>();
	const auto* constant = _compiler.get().make_constant(flags, function);

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

ParserData mint::BuildContext::locate_data(const SymbolPath& path) const {
	auto definition = module_description().locate(path);
	if (!definition.is_valid()) {
		definition = global_data_description().locate(path);
	}
	return std::visit(Overloaded {
	                      [](const PackageDescription& definition) -> ParserData {
		                      return definition;
	                      },
	                      [](const ClassDescription& definition) -> ParserData {
		                      return definition;
	                      },
	                      [](FunctionDefinition definition) -> ParserData {
		                      return definition;
	                      },
	                      [](const VariableDescription& definition) -> ParserData {
		                      return definition;
	                      },
	                      [](const AliasDescription& definition) -> ParserData {
		                      return definition;
	                      },
	                      [&](std::monostate&) -> ParserData {
		                      return defer_check(DeferredSymbolCheck {
		                          .symbol = path,
		                          .scope = &module_description(),
		                      });
	                      },
	                  },
	    definition);
}

ParserData mint::BuildContext::locate_data(BuiltinTypeKind kind) const {
	return global_data_description().builtin(kind);
}

ParserData mint::BuildContext::locate_data(const TypeAnnotation& type) const {
	return std::visit(mint::Overloaded {
	                      [&](std::monostate) -> ParserData {
		                      return type;
	                      },
	                      [&](const PrimitiveType&) -> ParserData {
		                      return type;
	                      },
	                      [&](const BuiltinType& builtin) -> ParserData {
		                      return locate_data(builtin.kind);
	                      },
	                      [&](const UserType& user_type) -> ParserData {
		                      return locate_data(user_type.path);
	                      },
	                      [&](const UnionType& union_type) -> ParserData {
		                      for (const auto& element : union_type.elements) {
			                      auto result = locate_data(element);
			                      if (result.is_deferred()
			                          || (result.is_temporary() && result.get_type().is_undefined()
			                              && (element.is_user() || element.is_union()))) {
				                      return result;
			                      }
		                      }
		                      return type;
	                      },
	                  },
	    type.definition);
}

void mint::BuildContext::check_return_type(const TypeAnnotation& type, const ParserData& data,
    const DataStream::Position& position) const {
	if (!match_type(type, data.get_type())) {
		parse_error(std::format("return type mismatch: expected '{}', got '{}'", type.to_string(),
		                data.get_type().to_string()),
		    position);
	}
}

ParserData mint::BuildContext::check_operator_call(const ParserData& object, Class::Operator op, const Call& call,
    const DataStream::Position& position, DeferPolicy defer_policy) const {
	return check_method_call(object, get_operator_symbol(op), call, position, defer_policy);
}

ParserData mint::BuildContext::check_function_call(const ParserData& function, const Call& call,
    const DataStream::Position& position, DeferPolicy defer_policy) const {
	if (const auto* description = std::get_if<ClassDefinition>(&function)) {
		const auto* owner_package = description->get().get_owner_package();
		const auto object = ParserData(owner_package ? *owner_package : global_data_description());
		return check_method_call(object, description->get().get_name(), call, position);
	}
	if (const auto* description = std::get_if<FunctionDefinition>(&function)) {
		if (call.with_unpack == 0) {
			const auto& overload = find_overload(*description, call.parameters.size(), position);
			if (defer_policy == DeferPolicy::enabled
			    && std::ranges::any_of(overload.parameters(), [&](const ParameterDescription& parameter) {
				       return locate_data(parameter.type).is_deferred();
			       })) {
				return defer_check(DeferredFunctionCallCheck {
				    .function = std::make_shared<ParserData>(function),
				    .parameters = call.parameters,
				    .with_unpack = call.with_unpack,
				});
			}
			return check_call(overload, call.parameters, position);
		}
		auto return_type = TypeAnnotation();
		for (const FunctionDescription& overload :
		    find_overloads(*description, call.parameters.size() - call.with_unpack, position)) {
			if (defer_policy == DeferPolicy::enabled
			    && std::ranges::any_of(overload.parameters(), [&](const ParameterDescription& parameter) {
				       return locate_data(parameter.type).is_deferred();
			       })) {
				return defer_check(DeferredFunctionCallCheck {
				    .function = std::make_shared<ParserData>(function),
				    .parameters = call.parameters,
				    .with_unpack = call.with_unpack,
				});
			}
			if (return_type.is_undefined()) {
				return_type = check_call(overload, call.parameters, position).get_type();
			}
			else {
				return_type.add_type(check_call(overload, call.parameters, position).get_type());
			}
		}
		return return_type;
	}
	return function.get_type();
}

ParserData mint::BuildContext::check_method_call(const ParserData& object, const Symbol& method, const Call& call,
    const DataStream::Position& position, DeferPolicy defer_policy) const {
	if (object.get_type().is_undefined()) {
		return {};
	}
	const auto member = find_member(object, method, defer_policy);
	if (member.is_deferred()) {
		return defer_check(DeferredMethodCallCheck {
		    .object = std::make_shared<ParserData>(object),
		    .method = method,
		    .parameters = call.parameters,
		    .with_unpack = call.with_unpack,
		});
	}
	const auto description = find_method(member);
	if (description.empty()) {
		return defer_check(DeferredMethodCallCheck {
		    .object = std::make_shared<ParserData>(object),
		    .method = method,
		    .parameters = call.parameters,
		    .with_unpack = call.with_unpack,
		});
	}
	auto call_parameters = call.parameters;
	if (const FunctionDescription& candidate = description.front(); candidate.get_flags() & Reference::global) {
		if (const auto* owner = candidate.get_owner_scope(); owner == nullptr || owner->is_root_scope()) {
			call_parameters.insert(call_parameters.begin(), object);
		}
	}
	else if (member.is_class()) {
		call_parameters.insert(call_parameters.begin(), member);
	}
	else if (!object.is_class()) {
		call_parameters.insert(call_parameters.begin(), object);
	}
	if (call.with_unpack == 0) {
		const auto& overload = find_overload(description, call_parameters.size(), position);
		if (defer_policy == DeferPolicy::enabled
		    && std::ranges::any_of(overload.parameters(), [&](const ParameterDescription& parameter) {
			       return locate_data(parameter.type).is_deferred();
		       })) {
			return defer_check(DeferredMethodCallCheck {
			    .object = std::make_shared<ParserData>(object),
			    .method = method,
			    .parameters = call.parameters,
			    .with_unpack = call.with_unpack,
			});
		}
		return check_call(overload, call_parameters, position);
	}
	auto return_type = TypeAnnotation();
	for (const FunctionDescription& overload :
	    find_overloads(description, call_parameters.size() - call.with_unpack, position)) {
		if (defer_policy == DeferPolicy::enabled
		    && std::ranges::any_of(overload.parameters(), [&](const ParameterDescription& parameter) {
			       return locate_data(parameter.type).is_deferred();
		       })) {
			return defer_check(DeferredMethodCallCheck {
			    .object = std::make_shared<ParserData>(object),
			    .method = method,
			    .parameters = call.parameters,
			    .with_unpack = call.with_unpack,
			});
		}
		if (return_type.is_undefined()) {
			return_type = check_call(overload, call_parameters, position).get_type();
		}
		else {
			return_type.add_type(check_call(overload, call_parameters, position).get_type());
		}
	}
	return return_type;
}

ParserData mint::BuildContext::check_init_symbol(const LookupContext& lookup_context, const ParserData& target,
    const ParserData& value, const DataStream::Position& position) const {
	const VariableDescription& target_variable = std::get<VariableDefinition>(target);
	if (!match_type(target.get_type(), value.get_type())) {
		parse_error(std::format("cannot initialize {} to {}", target.get_name(), value.get_name()), position);
	}
	if ((target_variable.flags & (Reference::const_address | Reference::const_value))
	    != (Reference::const_address | Reference::const_value)) {
		return target;
	}
	if (target_variable.flags & Reference::global) {
		return create_alias(lookup_context.package.get(), target_variable, value);
	}
	if (auto* function = lookup_context.function) {
		return create_alias(*function, target_variable, value);
	}
	return create_alias(module_description(), target_variable, value);
}

ParserData mint::BuildContext::check_operation(UnaryOperation op, const ParserData& operand,
    const DataStream::Position& position, DeferPolicy defer_policy) const {
	switch (op) {
	case UnaryOperation::inc_operator:
	case UnaryOperation::dec_operator:
	case UnaryOperation::compl_operator:
		if (const auto type = operand.get_type(); type.is_user() || type.is_built_in()) {
			return check_operator_call(operand, to_operator(op), Call(), position, defer_policy);
		}
		return operand.get_type();
	case UnaryOperation::not_operator:
		if (const auto type = operand.get_type(); type.is_user() || type.is_built_in()) {
			return check_operator_call(operand, to_operator(op), Call(), position, defer_policy);
		}
		return PrimitiveTypeKind::boolean;
	case UnaryOperation::pos_operator:
	case UnaryOperation::neg_operator:
		return operand.get_type();
	case UnaryOperation::typeof_operator:
		if (const auto* variable = std::get_if<VariableDefinition>(&operand)) {
			return TemporaryData {
			    .type = BuiltinTypeKind::string,
			    .typename_for = &variable->get(),
			};
		}
		return PrimitiveTypeKind::boolean;
	case UnaryOperation::membersof_operator:
		return BuiltinTypeKind::array;
	case UnaryOperation::defined_operator:
		return PrimitiveTypeKind::boolean;
	case UnaryOperation::await_operator:
		return {}; // TODO: get return type from call
	}
	return {};
}

ParserData mint::BuildContext::check_operation(BinaryOperation op, const ParserData& lhs, const ParserData& rhs,
    const DataStream::Position& position, DeferPolicy defer_policy) const {
	switch (op) {
	case BinaryOperation::move_operator:
		if (const auto* type_override = std::get_if<OverriddenVariable>(&lhs)) {
			try_reset_type_narrowing(*type_override);
			if (!match_type(type_override->description.type, rhs.get_type())) {
				parse_error(std::format("cannot assign {} to {}", rhs.get_name(), lhs.get_name()), position);
			}
			return type_override->description.type;
		}
		if (!match_type(lhs.get_type(), rhs.get_type())) {
			parse_error(std::format("cannot assign {} to {}", rhs.get_name(), lhs.get_name()), position);
		}
		return lhs.get_type();
	case BinaryOperation::copy_operator:
	case BinaryOperation::add_operator:
	case BinaryOperation::sub_operator:
	case BinaryOperation::mul_operator:
	case BinaryOperation::div_operator:
	case BinaryOperation::pow_operator:
	case BinaryOperation::mod_operator:
	case BinaryOperation::and_operator:
	case BinaryOperation::band_operator:
	case BinaryOperation::bor_operator:
	case BinaryOperation::xor_operator:
	case BinaryOperation::shift_left_operator:
	case BinaryOperation::shift_right_operator:
	case BinaryOperation::inclusive_range_operator:
	case BinaryOperation::exclusive_range_operator:
		if (const auto type = lhs.get_type(); type.is_user() || type.is_built_in()) {
			return check_operator_call(lhs, to_operator(op), to_call(rhs), position, defer_policy);
		}
		return lhs.get_type();
	case BinaryOperation::is_operator:
		return PrimitiveTypeKind::boolean;
	case BinaryOperation::as_operator:
		return rhs.get_type();
	case BinaryOperation::or_operator:
		if (const auto& condition = current_context().current_condition) {
			condition->is_narrowing_valid = false;
		}
		if (const auto type = lhs.get_type(); type.is_user() || type.is_built_in()) {
			return check_operator_call(lhs, to_operator(op), to_call(rhs), position, defer_policy);
		}
		return lhs.get_type();
	case BinaryOperation::eq_operator:
	case BinaryOperation::strict_eq_operator:
		if (const auto& condition = current_context().current_condition) {
			try_extract_type_narrowing(*condition, lhs, rhs);
		}
		if (const auto type = lhs.get_type(); type.is_user() || type.is_built_in()) {
			return check_operator_call(lhs, to_operator(op), to_call(rhs), position, defer_policy);
		}
		return PrimitiveTypeKind::boolean;
	case BinaryOperation::in_operator:
	case BinaryOperation::ne_operator:
	case BinaryOperation::lt_operator:
	case BinaryOperation::gt_operator:
	case BinaryOperation::le_operator:
	case BinaryOperation::ge_operator:
	case BinaryOperation::strict_ne_operator:
		if (const auto type = lhs.get_type(); type.is_user() || type.is_built_in()) {
			return check_operator_call(lhs, to_operator(op), to_call(rhs), position, defer_policy);
		}
		return PrimitiveTypeKind::boolean;
	case BinaryOperation::subscript_operator:
	case BinaryOperation::regex_match_operator:
	case BinaryOperation::regex_unmatch_operator:
		if (const auto type = lhs.get_type(); type.is_user() || type.is_built_in()) {
			return check_operator_call(lhs, to_operator(op), to_call(rhs), position, defer_policy);
		}
		return {};
	}
	return {};
}

ParserData mint::BuildContext::check_operation(TernaryOperation op, const ParserData& first, const ParserData& second,
    const ParserData& third, const DataStream::Position& position, DeferPolicy defer_policy) const {
	switch (op) {
	case TernaryOperation::conditional_operator:
	case TernaryOperation::subscript_move_operator:
		return second.get_type();
	}
	return {};
}

ParserData mint::BuildContext::check_call(const FunctionDescription& function,
    const std::vector<ParserData>& call_parameters, const DataStream::Position& position) const {
	const auto parameters_description = function.parameters();
	const auto parameters = std::views::iota(std::size_t {0}, call_parameters.size())
	                        | std::views::transform([&](std::size_t i) -> ParameterDescription {
		                          if (i < parameters_description.size()) {
			                          return parameters_description[i];
		                          }
		                          return {
		                              .name = "va_args",
		                          };
	                          });
	for (const auto& [parameter, call_parameter] : std::views::zip(parameters, call_parameters)) {
		if (!match_type(parameter.type, call_parameter.get_type())) {
			parse_error(std::format("cannot pass {} to parameter '{}' of type '{}'", call_parameter.get_name(),
			                parameter.name.str(), parameter.type.to_string()),
			    position);
		}
	}
	return function.get_return_type();
}

ParserData mint::BuildContext::find_symbol(const Symbol& symbol, std::size_t index) const {

	if (const auto* def = current_function_context()) {
		if (index != invalid_index) {
			if (index < def->active_type_overrides.size()) {
				if (const auto* type_override = def->active_type_overrides.at(index)) {
					assert(def->local_symbols.at(index).get_name() == symbol);
					return OverriddenVariable {
					    .description = std::get<VariableDefinition>(def->local_symbols.at(index)),
					    .type = *type_override,
					    .index = index,
					};
				}
			}
			assert(def->local_symbols.at(index).get_name() == symbol);
			return def->local_symbols.at(index).to_parser_data();
		}
		if (def->description) {
			if (const auto definition = def->description->locate(symbol); definition.is_valid()) {
				return definition;
			}
			const SymbolScope* fallback_scope = nullptr;
			if (const auto* owner = def->description->get_owner_scope()) {
				fallback_scope = &owner->get_root_scope();
				if (const auto definition = fallback_scope->locate(symbol); definition.is_valid()) {
					return definition;
				}
			}
			if (const auto definition = global_data_description().locate(symbol); definition.is_valid()) {
				return definition;
			}
			return defer_check(DeferredSymbolCheck {
			    .symbol = symbol,
			    .scope = def->description,
			    .fallback_scope = fallback_scope,
			});
		}
	}

	if (index != invalid_index) {
		if (index < _module->active_type_overrides.size()) {
			if (const auto* type_override = _module->active_type_overrides.at(index)) {
				assert(_module->local_symbols.at(index).get_name() == symbol);
				return OverriddenVariable {
				    .description = std::get<VariableDefinition>(_module->local_symbols.at(index)),
				    .type = *type_override,
				    .index = index,
				};
			}
		}
		assert(_module->local_symbols.at(index).get_name() == symbol);
		return _module->local_symbols.at(index).to_parser_data();
	}
	if (const auto definition = module_description().locate(symbol); definition.is_valid()) {
		return definition;
	}
	if (const auto definition = global_data_description().locate(symbol); definition.is_valid()) {
		return definition;
	}
	return defer_check(DeferredSymbolCheck {
	    .symbol = symbol,
	    .scope = &module_description(),
	});
}

FunctionDefinition mint::BuildContext::find_method(const ParserData& member) const {
	return std::visit(Overloaded {
	                      [](const PackageDescription&) -> FunctionDefinition {
		                      return {};
	                      },
	                      [&](const ClassDescription& description) -> FunctionDefinition {
		                      return description.find_method(builtin_symbols::new_method);
	                      },
	                      [](const FunctionDefinition& definition) -> FunctionDefinition {
		                      return definition;
	                      },
	                      [](const VariableDescription&) -> FunctionDefinition {
		                      return {};
	                      },
	                      [&](const AliasDescription& description) -> FunctionDefinition {
		                      return find_method(unwrap_alias(description));
	                      },
	                      [](const OverriddenVariable&) -> FunctionDefinition {
		                      return {};
	                      },
	                      [](const TemporaryData&) -> FunctionDefinition {
		                      return {};
	                      },
	                      [](const DeferredCheck&) -> FunctionDefinition {
		                      return {};
	                      },
	                  },
	    member);
}

ParserData mint::BuildContext::find_member(const TypeAnnotation& type, const Symbol& symbol,
    DeferPolicy defer_policy) const {
	return std::visit(Overloaded {
	                      [&](std::monostate) -> ParserData {
		                      return {};
	                      },
	                      [&](const PrimitiveType&) -> ParserData {
		                      if (auto function = global_data_description().find_any_function(symbol);
		                          !function.empty()) {
			                      return function;
		                      }
		                      return {};
	                      },
	                      [&](const BuiltinType& builtin) -> ParserData {
		                      return find_member(locate_data(builtin.kind), symbol, defer_policy);
	                      },
	                      [&](const UserType& user_type) -> ParserData {
		                      return find_member(locate_data(user_type.path), symbol, defer_policy);
	                      },
	                      [&](const UnionType& union_type) -> ParserData {
		                      // TODO: find member over union
		                      return {};
	                      },
	                  },
	    type.definition);
}

ParserData mint::BuildContext::find_member(const ParserData& object, const Symbol& symbol,
    DeferPolicy defer_policy) const {
	return std::visit(Overloaded {
	                      [&](const PackageDescription& description) -> ParserData {
		                      if (const auto member = description.locate(symbol); member.is_valid()) {
			                      return member;
		                      }
		                      if (defer_policy == DeferPolicy::enabled) {
			                      return defer_check(DeferredMemberCheck {
			                          .object = std::make_shared<ParserData>(object),
			                          .member = symbol,
			                      });
		                      }
		                      if (auto function = global_data_description().find_any_function(symbol);
		                          !function.empty()) {
			                      return function;
		                      }
		                      return {};
	                      },
	                      [&](const ClassDescription& description) -> ParserData {
		                      if (const auto member = description.locate(symbol); member.is_valid()) {
			                      return member;
		                      }
		                      if (defer_policy == DeferPolicy::enabled) {
			                      return defer_check(DeferredMemberCheck {
			                          .object = std::make_shared<ParserData>(object),
			                          .member = symbol,
			                      });
		                      }
		                      if (auto function = global_data_description().find_any_function(symbol);
		                          !function.empty()) {
			                      return function;
		                      }
		                      return {};
	                      },
	                      [&](const FunctionDefinition&) -> ParserData {
		                      if (auto function = global_data_description().find_any_function(symbol);
		                          !function.empty()) {
			                      return function;
		                      }
		                      if (defer_policy == DeferPolicy::enabled) {
			                      return defer_check(DeferredMemberCheck {
			                          .object = std::make_shared<ParserData>(object),
			                          .member = symbol,
			                      });
		                      }
		                      return {};
	                      },
	                      [&](const VariableDescription& description) -> ParserData {
		                      return find_member(description.type, symbol, defer_policy);
	                      },
	                      [&](const AliasDescription& description) -> ParserData {
		                      return find_member(unwrap_alias(description), symbol);
	                      },
	                      [&](const OverriddenVariable& type_override) -> ParserData {
		                      return find_member(type_override.type, symbol, defer_policy);
	                      },
	                      [&](const TemporaryData& temporary) -> ParserData {
		                      return find_member(temporary.type, symbol, defer_policy);
	                      },
	                      [&](const DeferredCheck&) -> ParserData {
		                      return {};
	                      },
	                  },
	    object);
}

const FunctionDescription& mint::BuildContext::find_overload(const FunctionDefinition& functions,
    std::size_t parameter_count, const DataStream::Position& position) const {

	auto min_parameters = std::numeric_limits<std::size_t>::max();
	auto max_parameters = std::size_t {0};
	auto has_variadic = false;

	for (const FunctionDescription& function : functions) {

		min_parameters = std::min(min_parameters, function.get_required_parameter_count());
		max_parameters = std::max(max_parameters, function.parameters().size());
		has_variadic |= function.is_variadic();

		if (parameter_count < function.get_required_parameter_count()) {
			continue;
		}

		if (!function.is_variadic() && parameter_count > function.parameters().size()) {
			continue;
		}

		return function;
	}

	if (parameter_count < min_parameters) {
		parse_error(std::format("too few arguments for {}(): expected at least {}, got {}{}",
		                functions.front().get().get_path().to_string(), min_parameters, parameter_count,
		                list_candidates(functions)),
		    position);
	}

	if (!has_variadic && parameter_count > max_parameters) {
		parse_error(std::format("too many arguments for {}(): expected at most {}, got {}{}",
		                functions.front().get().get_path().to_string(), max_parameters, parameter_count,
		                list_candidates(functions)),
		    position);
	}

	parse_error(std::format("no matching overload for {}() with {} arguments{}",
	                functions.front().get().get_path().to_string(), parameter_count, list_candidates(functions)),
	    position);
}

FunctionDefinition mint::BuildContext::find_overloads(const FunctionDefinition& functions,
    std::size_t min_parameter_count, const DataStream::Position& position) const {

	auto definition = FunctionDefinition();

	for (const FunctionDescription& function : functions) {
		if (!function.is_variadic() && min_parameter_count > function.parameters().size()) {
			continue;
		}
		definition.emplace_back(function);
	}

	if (definition.empty()) {
		parse_error(std::format("no matching overload for {}() with at least {} arguments{}",
		                functions.front().get().get_path().to_string(), min_parameter_count, list_candidates(functions)),
		    position);
	}

	return definition;
}

void mint::BuildContext::try_extract_type_narrowing(ConditionContext& condition, const ParserData& lhs,
    const ParserData& rhs) const {

	const auto* left_tmp = std::get_if<TemporaryData>(&lhs);
	const auto* right_tmp = std::get_if<TemporaryData>(&rhs);
	if (!left_tmp || !right_tmp) {
		return;
	}

	const TemporaryData* typeof_op = nullptr;
	const TemporaryData* constant_op = nullptr;

	if (left_tmp->typename_for) {
		typeof_op = left_tmp;
		constant_op = right_tmp;
	}
	else if (right_tmp->typename_for) {
		typeof_op = right_tmp;
		constant_op = left_tmp;
	}

	if (typeof_op && constant_op && constant_op->data && constant_op->type == BuiltinTypeKind::string) {
		if (const auto index = find_fast_symbol_index(typeof_op->typename_for->name); index != invalid_index) {
			const auto* type_str_data = static_cast<const String*>(constant_op->data);
			condition.pending_type_overrides.push_back({
			    .index = index,
			    .type = make_type_annotation(type_str_data->str),
			});
		}
	}
}

void mint::BuildContext::try_reset_type_narrowing(const OverriddenVariable& type_override) const {
	if (auto& context = current_context(); type_override.index < context.active_type_overrides.size()) {
		context.active_type_overrides.at(type_override.index) = nullptr;
	}
}

DeferredCheck mint::BuildContext::defer_check(DeferredCheck::Definition definition) const {
	return DeferredCheck {
	    .definition = std::make_shared<DeferredCheck::Definition>(std::move(definition)),
	    .position = _lexer.position(),
	};
}

ParserData mint::BuildContext::resolve_deferred_data(const ParserData& data) const {
	if (const auto* check = std::get_if<DeferredCheck>(&data)) {
		return resolve_deferred_check(*check);
	}
	return data;
}

ParserData mint::BuildContext::resolve_deferred_check(const DeferredCheck& check) const {

	if (!check.definition) {
		return {};
	}

	if (check.result) {
		return *check.result;
	}

	const auto result = std::make_shared<ParserData>();
	check.result = result;

	*result = std::visit(Overloaded {
	                         [&](const DeferredInitSymbolCheck& operation) -> ParserData {
		                         return check_init_symbol(operation.lookup_context,
		                             resolve_deferred_data(*operation.target), resolve_deferred_data(*operation.value),
		                             check.position);
	                         },
	                         [&](const DeferredUnaryOperationCheck& operation) -> ParserData {
		                         return check_operation(operation.operation, resolve_deferred_data(*operation.operand),
		                             check.position, DeferPolicy::disabled);
	                         },
	                         [&](const DeferredBinaryOperationCheck& operation) -> ParserData {
		                         return check_operation(operation.operation, resolve_deferred_data(*operation.lhs),
		                             resolve_deferred_data(*operation.rhs), check.position, DeferPolicy::disabled);
	                         },
	                         [&](const DeferredTernaryOperationCheck& operation) -> ParserData {
		                         return check_operation(operation.operation, resolve_deferred_data(*operation.first),
		                             resolve_deferred_data(*operation.second), resolve_deferred_data(*operation.third),
		                             check.position, DeferPolicy::disabled);
	                         },
	                         [&](const DeferredMemberCheck& member) -> ParserData {
		                         const auto object = resolve_deferred_data(*member.object);
		                         return find_member(object, member.member, DeferPolicy::disabled);
	                         },
	                         [&](const DeferredSymbolCheck& symbol) -> ParserData {
		                         if (!symbol.scope) {
			                         return {};
		                         }
		                         if (const auto definition = symbol.scope->locate(symbol.symbol);
		                             definition.is_valid()) {
			                         return definition;
		                         }
		                         if (symbol.fallback_scope) {
			                         if (const auto definition = symbol.fallback_scope->locate(symbol.symbol);
			                             definition.is_valid()) {
				                         return definition;
			                         }
		                         }
		                         return global_data_description().locate(symbol.symbol);
	                         },
	                         [&](const DeferredFunctionCallCheck& function_call) -> ParserData {
		                         const auto call = Call {
		                             .parameters = std::vector(std::from_range,
		                                 std::views::transform(function_call.parameters,
		                                     std::bind_front(&BuildContext::resolve_deferred_data, this))),
		                             .with_unpack = function_call.with_unpack,
		                         };
		                         return check_function_call(resolve_deferred_data(*function_call.function), call,
		                             check.position, DeferPolicy::disabled);
	                         },
	                         [&](const DeferredMethodCallCheck& method_call) -> ParserData {
		                         const auto call = Call {
		                             .parameters = std::vector(std::from_range,
		                                 std::views::transform(method_call.parameters,
		                                     std::bind_front(&BuildContext::resolve_deferred_data, this))),
		                             .with_unpack = method_call.with_unpack,
		                         };
		                         return check_method_call(resolve_deferred_data(*method_call.object),
		                             Symbol(method_call.method), call, check.position, DeferPolicy::disabled);
	                         },
	                         [&](const DeferredReturnCheck& return_check) -> ParserData {
		                         check_return_type(return_check.expected,
		                             resolve_deferred_data(*return_check.actual).get_type(), check.position);
		                         return {};
	                         },
	                     },
	    *check.definition);
	if (result->is_deferred()) {
		*result = {};
	}
	return *result;
}

bool mint::BuildContext::match_type(const TypeAnnotation& target, const TypeAnnotation& source) const {

	assert(!locate_data(target).is_deferred());
	assert(!locate_data(source).is_deferred());

	return std::visit(mint::Overloaded {
	                      [](std::monostate) -> bool {
		                      return true;
	                      },
	                      [&](const PrimitiveType& primitive) -> bool {
		                      return can_cast(primitive.kind, source);
	                      },
	                      [&](const BuiltinType& builtin) -> bool {
		                      return can_cast(builtin.kind, source);
	                      },
	                      [&](const UserType& user_type) -> bool {
		                      if (const auto* source_user_type = std::get_if<UserType>(&source.definition)) {
			                      if (const auto source_definition = locate_data(source_user_type->path);
			                          const auto* source_description = std::get_if<ClassDefinition>(
			                              &source_definition)) {
				                      return source_description->get().is_derived_from(user_type.path);
			                      }
		                      }
		                      return false;
	                      },
	                      [&](const UnionType& union_type) -> bool {
		                      return std::ranges::any_of(union_type.elements, [&](const auto& target) {
			                      return match_type(target, source);
		                      });
	                      },
	                  },
	    target.definition);
}

bool mint::BuildContext::can_cast(PrimitiveTypeKind kind, const TypeAnnotation& type) const {
	return std::visit(mint::Overloaded {
	                      [](std::monostate) -> bool {
		                      return false;
	                      },
	                      [&](const PrimitiveType& primitive) -> bool {
		                      switch (kind) {
		                      case PrimitiveTypeKind::none:
		                      case PrimitiveTypeKind::null:
		                      case PrimitiveTypeKind::function:
		                      case PrimitiveTypeKind::coroutine:
		                      case PrimitiveTypeKind::package:
			                      return primitive.kind == kind;
		                      case PrimitiveTypeKind::boolean:
			                      return true;
		                      case PrimitiveTypeKind::number:
			                      return primitive.kind == PrimitiveTypeKind::boolean
			                             || primitive.kind == PrimitiveTypeKind::number;
		                      }
	                      },
	                      [&](const BuiltinType& builtin) -> bool {
		                      switch (kind) {
		                      case PrimitiveTypeKind::none:
		                      case PrimitiveTypeKind::null:
		                      case PrimitiveTypeKind::function:
		                      case PrimitiveTypeKind::coroutine:
		                      case PrimitiveTypeKind::package:
			                      return false;
		                      case PrimitiveTypeKind::boolean:
			                      return true;
		                      case PrimitiveTypeKind::number:
			                      return builtin.kind == BuiltinTypeKind::string
			                             || builtin.kind == BuiltinTypeKind::iterator;
		                      }
	                      },
	                      [&](const UserType& user_type) -> bool {
		                      switch (kind) {
		                      case PrimitiveTypeKind::none:
		                      case PrimitiveTypeKind::null:
		                      case PrimitiveTypeKind::function:
		                      case PrimitiveTypeKind::coroutine:
		                      case PrimitiveTypeKind::package:
			                      return false;
		                      case PrimitiveTypeKind::boolean:
			                      return true;
		                      case PrimitiveTypeKind::number:
			                      if (const auto definition = locate_data(user_type.path);
			                          const auto* type = std::get_if<ClassDefinition>(&definition)) {
				                      return type->get().has_any_member(builtin_symbols::to_number);
			                      }
			                      return false;
		                      }
	                      },
	                      [&](const UnionType& union_type) -> bool {
		                      return std::ranges::any_of(union_type.elements, [&](const auto& type) {
			                      return can_cast(kind, type);
		                      });
	                      },
	                  },
	    type.definition);
}

bool mint::BuildContext::can_cast(BuiltinTypeKind kind, const TypeAnnotation& type) const {
	return std::visit(mint::Overloaded {
	                      [&](std::monostate) -> bool {
		                      switch (kind) {
		                      case BuiltinTypeKind::string:
		                      case BuiltinTypeKind::regex:
		                      case BuiltinTypeKind::array:
		                      case BuiltinTypeKind::iterator:
		                      case BuiltinTypeKind::async_iterator:
			                      return true;
		                      case BuiltinTypeKind::hash:
		                      case BuiltinTypeKind::libobject:
		                      case BuiltinTypeKind::library:
			                      return false;
		                      }
	                      },
	                      [&](const PrimitiveType&) -> bool {
		                      switch (kind) {
		                      case BuiltinTypeKind::string:
		                      case BuiltinTypeKind::regex:
		                      case BuiltinTypeKind::array:
		                      case BuiltinTypeKind::hash:
		                      case BuiltinTypeKind::iterator:
		                      case BuiltinTypeKind::async_iterator:
			                      return true;
		                      case BuiltinTypeKind::libobject:
		                      case BuiltinTypeKind::library:
			                      return false;
		                      }
	                      },
	                      [&](const BuiltinType& builtin) -> bool {
		                      switch (kind) {
		                      case BuiltinTypeKind::string:
		                      case BuiltinTypeKind::regex:
		                      case BuiltinTypeKind::array:
		                      case BuiltinTypeKind::iterator:
		                      case BuiltinTypeKind::async_iterator:
			                      return true;
		                      case BuiltinTypeKind::hash:
			                      return builtin.kind == BuiltinTypeKind::string
			                             || builtin.kind == BuiltinTypeKind::regex
			                             || builtin.kind == BuiltinTypeKind::array
			                             || builtin.kind == BuiltinTypeKind::hash
			                             || builtin.kind == BuiltinTypeKind::iterator
			                             || builtin.kind == BuiltinTypeKind::async_iterator;
		                      case BuiltinTypeKind::libobject:
		                      case BuiltinTypeKind::library:
			                      return builtin.kind == kind;
		                      }
	                      },
	                      [&](const UserType&) -> bool {
		                      switch (kind) {
		                      case BuiltinTypeKind::string:
		                      case BuiltinTypeKind::regex:
		                      case BuiltinTypeKind::array:
		                      case BuiltinTypeKind::hash:
		                      case BuiltinTypeKind::iterator:
		                      case BuiltinTypeKind::async_iterator:
			                      return true;
		                      case BuiltinTypeKind::libobject:
		                      case BuiltinTypeKind::library:
			                      return false;
		                      }
	                      },
	                      [&](const UnionType& union_type) -> bool {
		                      return std::ranges::any_of(union_type.elements, [&](const auto& type) {
			                      return can_cast(kind, type);
		                      });
	                      },
	                  },
	    type.definition);
}
