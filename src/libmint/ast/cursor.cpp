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

#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/ast/printer.h"
#include "mint/ast/savedstate.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/debug/debuginfo.h"
#include "mint/debug/lineinfo.h"
#include "mint/memory/data.h"
#include "mint/memory/garbagecollector.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/symboltable.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/exception.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/system/assert.h"
#include "mint/system/poolallocator.h"
#include "threadentrypoint.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace mint;

PoolAllocator<Cursor::Context> Cursor::g_pool;

namespace {

void dump_module(LineInfoList& dumped_infos, AbstractSyntaxTree& ast, const Module& module, std::size_t offset) {

	if (&module != &ThreadEntryPoint::instance()) {

		const Module::Id module_id = ast.get_module_id(module);
		const std::string module_name = ast.get_module_name(module);

		if (DebugInfo* infos = ast.find_debug_info(module_id)) {
			dumped_infos.emplace_back(module_id, module_name, infos->line_number(offset));
		}
		else {
			dumped_infos.emplace_back(module_id, module_name);
		}
	}
}

std::size_t last_executed_offset(std::size_t next_offset) {
	return next_offset ? next_offset - 1 : 0;
}

}

Cursor::Call::Call(const Reference& function, Class* metadata) :
    _function(function),
    _metadata(metadata) {}

Cursor::Call::Call(const Reference& function, Class& metadata) :
    _function(function),
    _metadata(&metadata) {}

Cursor::Call::Call(Reference&& function, Class* metadata) :
    _function(std::move(function)),
    _metadata(metadata) {}

Cursor::Call::Call(Reference&& function, Class& metadata) :
    _function(std::move(function)),
    _metadata(&metadata) {}

Cursor::Call::Flags Cursor::Call::get_flags() const {
	return _flags;
}

void Cursor::Call::set_flags(Flags flags) {
	_flags = flags;
}

Class* Cursor::Call::get_metadata() const {
	return _metadata;
}

void Cursor::Call::set_metadata(Class* metadata) {
	_metadata = metadata;
}

void mint::Cursor::Call::set_metadata(Class& metadata) {
	_metadata = &metadata;
}

int Cursor::Call::extra_argument_count() const {
	return _extra_args;
}

void Cursor::Call::add_extra_argument(std::size_t count) {
	_extra_args += static_cast<int>(count);
}

Reference& Cursor::Call::function() {
	return _function;
}

Cursor::WaitingCallStack::WaitingCallStack() {
	register_root();
}

Cursor::WaitingCallStack::~WaitingCallStack() {
	unregister_root();
}

void Cursor::WaitingCallStack::mark() {
	for (auto& call : _calls) {
		call.function().data().mark();
	}
}

Cursor::Cursor(AbstractSyntaxTree& ast, Module& module, Cursor* parent) :
    _stack(parent ? parent->_stack : GarbageCollector::instance().create_stack()),
    _current_context(g_pool.allocate()),
    _ast(ast),
    _parent(parent),
    _child(nullptr) {

	std::construct_at(_current_context, module);
	_current_context->symbols = std::make_shared<SymbolTable>(_ast.get().global_data());

	if (_parent) {
		assert(_parent->_child == nullptr);
		_parent->_child = this;
	}
}

Cursor::Cursor(AbstractSyntaxTree& ast, Cursor* parent) :
    Cursor(ast, ThreadEntryPoint::instance(), parent) {}

Cursor::~Cursor() {

	if (_parent) {
		assert(_parent->_child == this);
		_parent->_child = nullptr;
	}
	else {
		GarbageCollector::instance().remove_stack(_stack);
	}

	while (!_call_stack.empty()) {
		exit_call();
	}

	std::destroy_at(_current_context);
	g_pool.deallocate(_current_context);
}

std::unique_ptr<Cursor> Cursor::make_thread() {
	return std::make_unique<Cursor>(_ast, this);
}

bool Cursor::is_thread() const {

	if (_parent != nullptr) {
		return false;
	}

	if (_call_stack.empty()) {
		return &_current_context->module == &ThreadEntryPoint::instance();
	}

	return &_call_stack.front()->module == &ThreadEntryPoint::instance();
}

void Cursor::jmp(std::size_t pos) {
	_current_context->iptr = pos;
}

bool Cursor::call_in_progress() const {
	if (&_current_context->module != &ThreadEntryPoint::instance()) {
		return !_call_stack.empty();
	}
	return false;
}

void Cursor::call_generator_expression(std::size_t offset) {

	assert(!_current_context->coroutine);

	auto* expression_context = g_pool.allocate();
	std::construct_at(expression_context, _current_context->module);
	expression_context->iptr = _current_context->iptr;
	expression_context->symbols = _current_context->symbols;
	_current_context->iptr = offset;

	const auto stack_base = _stack->size();

	expression_context->generator = std::make_unique<WeakReference>(Reference::default_flags,
	    std::in_place_type<Iterator>, from_generator, _ast, stack_base + 1);
	_stack->emplace_back(*expression_context->generator);
	expression_context->generator->data<Iterator>().construct();

	_call_stack.emplace_back(_current_context);
	_current_context = expression_context;
}

void Cursor::call_async_generator_expression(std::size_t offset) {

	assert(_current_context->coroutine);

	auto* expression_context = g_pool.allocate();
	std::construct_at(expression_context, _current_context->module);
	expression_context->iptr = _current_context->iptr;
	expression_context->symbols = _current_context->symbols;
	_current_context->iptr = offset;

	const auto stack_base = _stack->size();

	expression_context->coroutine = std::make_unique<WeakReference>(Reference::default_flags,
	    std::in_place_type<Coroutine>, std::make_unique<SavedState>(*this, expression_context), stack_base);

	expression_context->generator = std::make_unique<WeakReference>(Reference::default_flags,
	    std::in_place_type<Iterator>, from_async_generator, _ast, expression_context->coroutine->data<Coroutine>(),
	    stack_base + 1);
	_stack->emplace_back(*expression_context->generator);
	expression_context->generator->data<Iterator>().construct();

	expression_context->coroutine->data<Coroutine>().await(*this, WeakReference(*expression_context->coroutine));
}

void Cursor::call(const Module::Handle& handle, int signature, Class* metadata) {

	const auto stack_base = _stack->size() - static_cast<std::size_t>(signature >= 0 ? signature : (~signature) + 1);

	auto* call_context = g_pool.allocate();
	std::construct_at(call_context, handle.module);
	call_context->iptr = handle.offset;

	if (handle.symbols) {
		call_context->symbols = std::make_shared<SymbolTable>(_ast.get().global_data(), metadata);
		call_context->symbols->reserve_fast(handle.fast_count);
		call_context->symbols->open_package(handle.package);
	}

	if (handle.async) {

		call_context->coroutine = std::make_unique<WeakReference>(Reference::default_flags,
		    std::in_place_type<Coroutine>, std::make_unique<SavedState>(*this, call_context), stack_base);

		if (handle.generator) {
			call_context->generator = std::make_unique<WeakReference>(Reference::default_flags,
			    std::in_place_type<Iterator>, from_async_generator, _ast, call_context->coroutine->data<Coroutine>(),
			    stack_base + 1);
			_stack->emplace(std::next(_stack->begin(), static_cast<std::ptrdiff_t>(stack_base)),
			    *call_context->generator);
			call_context->generator->data<Iterator>().construct();
		}

		_stack->emplace_back(*call_context->coroutine);
	}
	else {

		if (handle.generator) {
			call_context->generator = std::make_unique<WeakReference>(Reference::default_flags,
			    std::in_place_type<Iterator>, from_generator, _ast, stack_base + 1);
			_stack->emplace(std::next(_stack->begin(), static_cast<std::ptrdiff_t>(stack_base)),
			    *call_context->generator);
			call_context->generator->data<Iterator>().construct();
		}

		_call_stack.emplace_back(_current_context);
		_current_context = call_context;
	}
}

void Cursor::call(const Module& module, std::size_t pos, PackageData& package, Class* metadata) {

	_call_stack.emplace_back(_current_context);

	_current_context = g_pool.allocate();
	std::construct_at(_current_context, module);
	_current_context->symbols = std::make_shared<SymbolTable>(_ast.get().global_data(), metadata);
	_current_context->symbols->open_package(package);
	_current_context->iptr = pos;
}

void Cursor::exit_call() {
	std::destroy_at(_current_context);
	g_pool.deallocate(_current_context);
	_current_context = _call_stack.back();
	_call_stack.pop_back();
}

bool Cursor::is_in_builtin() const {
	return _current_context->symbols == nullptr;
}

bool Cursor::is_in_generator() const {
	return _current_context->generator != nullptr;
}

bool Cursor::is_in_coroutine() const {
	return _current_context->coroutine != nullptr;
}

std::unique_ptr<SavedState> Cursor::suspend(std::unique_ptr<SavedState> state, std::size_t stack_offset) {

	auto previous_state = std::make_unique<SavedState>(*this, _current_context);
	_current_context = state->context;

	while (!state->retrieve_points.empty()) {
		auto& retrieve_point = state->retrieve_points.top();
		retrieve_point.stack_size += stack_offset;
		_retrieve_points.push(retrieve_point);
		state->retrieve_points.pop();
	}

	state->context = nullptr;
	return previous_state;
}

std::unique_ptr<SavedState> Cursor::interrupt(std::size_t stack_offset) {

	auto state = std::make_unique<SavedState>(*this, _current_context);
	_current_context = _call_stack.back();
	_call_stack.pop_back();

	while (!_retrieve_points.empty() && _retrieve_points.top().call_stack_size > _call_stack.size()) {
		auto& retrieve_point = _retrieve_points.top();
		retrieve_point.stack_size -= stack_offset;
		state->retrieve_points.push(retrieve_point);
		_retrieve_points.pop();
	}

	return state;
}

void Cursor::restore(std::unique_ptr<SavedState> state, std::size_t stack_offset) {

	_call_stack.push_back(_current_context);
	_current_context = state->context;

	while (!state->retrieve_points.empty()) {
		auto& retrieve_point = state->retrieve_points.top();
		retrieve_point.stack_size += stack_offset;
		_retrieve_points.push(retrieve_point);
		state->retrieve_points.pop();
	}

	state->context = nullptr;
}

void Cursor::destroy(SavedState* state) {
	assert(&state->cursor.get() == this);
	if (state->context) {
		std::destroy_at(state->context);
		g_pool.deallocate(state->context);
	}
}

void Cursor::open_printer(std::unique_ptr<Printer>&& printer) {
	_current_context->printers.emplace_back(std::move(printer));
}

void Cursor::close_printer() {
	_current_context->printers.pop_back();
}

Printer* Cursor::printer() {
	if (_current_context->printers.empty()) {
		return nullptr;
	}
	return _current_context->printers.back().get();
}

bool Cursor::load_module(const std::string& module) {

	const auto info = _ast.get().load_module(module);

	if (info.id == Module::invalid_id) [[unlikely]] {
		return false;
	}

	if (info.state == Module::State::not_loaded) {
		call(*info.module, 0, _ast.get().global_data());
		_ast.get().set_module_state(info.id, Module::State::ready);
	}

	return true;
}

bool Cursor::exit_module() {

	if (call_in_progress()) {
		exit_call();
		return true;
	}

	return false;
}

void Cursor::set_retrieve_point(std::size_t offset) {
	_retrieve_points.push({
	    .stack_size = _stack->size(),
	    .call_stack_size = _call_stack.size(),
	    .retrieve_offset = offset,
	    .current_context = _current_context,
	});
}

void Cursor::unset_retrieve_point() {
	_retrieve_points.pop();
}

void Cursor::raise(WeakReference&& exception) {

	if (!_retrieve_points.empty()) {

		const RetrievePoint& state = _retrieve_points.top();

		while (state.current_context != _current_context) {
			if (_current_context->coroutine) {
				_current_context->coroutine->data<Coroutine>().raise(*this);
			}
			else {
				exit_call();
			}
		}

		while (!_current_context->waiting_calls.empty()) {
			_current_context->waiting_calls.pop();
		}

		_stack->resize(state.stack_size);
		_stack->emplace_back(std::move(exception));
		jmp(state.retrieve_offset);

		_retrieve_points.pop();
	}
	else if (_parent) {
		throw MintException(*_parent, std::move(exception));
	}
	else {
		auto* scheduler = Scheduler::instance();
		assert_x(scheduler, __func__, "execution should be done using a scheduler");
		scheduler->create_exception(std::move(exception));
	}
}

LineInfoList Cursor::dump() const {

	LineInfoList dumped_infos;
	dump_module(dumped_infos, _ast, _current_context->module, last_executed_offset(_current_context->iptr));

	for (const auto* context : std::views::reverse(_call_stack)) {
		dump_module(dumped_infos, _ast, context->module, last_executed_offset(context->iptr));
	}

	if (_child) {
		std::ranges::copy(_child->dump(), std::back_inserter(dumped_infos));
	}

	return dumped_infos;
}

std::size_t Cursor::offset() const {
	return _current_context->iptr;
}

void Cursor::resume() {
	jmp(_current_context->module.next_node_offset());
	_stack->clear();
}

void Cursor::retrieve() {

	while (!_call_stack.empty()) {
		exit_call();
	}

	while (!_current_context->waiting_calls.empty()) {
		_current_context->waiting_calls.pop();
	}

	while (!_stack->empty()) {
		_stack->pop_back();
	}

	jmp(_current_context->module.end());
}

void Cursor::cleanup() {

	if (_parent == nullptr) {

		while (!_call_stack.empty()) {
			exit_call();
		}

		_current_context->printers.clear();
		_current_context->symbols->clear();
		_stack->clear();
	}
}

Cursor::Context::Context(const Module& module) :
    module(module) {}
