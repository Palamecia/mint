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

#include "mint/ast/cursor.h"
#include "mint/ast/savedstate.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/exception.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/builtin/iterator.h"
#include "threadentrypoint.h"

using namespace std;
using namespace mint;

pool_allocator<Cursor::Context> Cursor::g_pool;

void dump_module(LineInfoList &dumped_infos, AbstractSyntaxTree *ast, Module *module, size_t offset);

Cursor::Call::Call(Call &&other) :
	m_function(std::forward<Reference>(other.function())),
	m_metadata(other.m_metadata),
	m_extra_args(other.m_extra_args),
	m_flags(other.m_flags) {

}

Cursor::Call::Call(Reference &&function) :
	m_function(std::forward<Reference>(function)) {

}

Cursor::Call &Cursor::Call::operator =(Call &&other) {
	m_function = std::move(other.m_function);
	m_metadata = other.m_metadata;
	m_extra_args = other.m_extra_args;
	m_flags = other.m_flags;
	return *this;
}

Cursor::Call::Flags Cursor::Call::get_flags() const {
	return m_flags;
}

void Cursor::Call::set_flags(Flags flags) {
	m_flags = flags;
}

Class *Cursor::Call::get_metadata() const {
	return m_metadata;
}

void Cursor::Call::set_metadata(Class *metadata) {
	m_metadata = metadata;
}

int Cursor::Call::extra_argument_count() const {
	return m_extra_args;
}

void Cursor::Call::add_extra_argument(size_t count) {
	m_extra_args += static_cast<int>(count);
}

Reference &Cursor::Call::function() {
	return m_function;
}

Cursor::Cursor(AbstractSyntaxTree *ast, Module *module, Cursor *parent) :
	m_ast(ast),
	m_parent(parent),
	m_child(nullptr),
	m_stack(parent ? parent->m_stack : GarbageCollector::instance().create_stack()),
	m_current_context(g_pool.allocate()) {
	new (m_current_context) Context(module);
	m_current_context->symbols = new SymbolTable;

	if (m_parent) {
		assert(m_parent->m_child == nullptr);
		m_parent->m_child = this;
	}
}

Cursor::~Cursor() {

	if (m_parent) {
		assert(m_parent->m_child == this);
		m_parent->m_child = nullptr;
	}
	else {
		GarbageCollector::instance().remove_stack(m_stack);
	}

	while (!m_call_stack.empty()) {
		exit_call();
	}

	m_current_context->~Context();
	g_pool.deallocate(m_current_context);

	m_ast->remove_cursor(this);
}

AbstractSyntaxTree *Cursor::ast() const {
	return m_ast;
}

Cursor *Cursor::parent() const {
	return m_parent;
}

void Cursor::jmp(size_t pos) {
	m_current_context->iptr = pos;
}

void Cursor::call(Module::Handle *handle, int signature, Class *metadata) {

	m_call_stack.emplace_back(m_current_context);

	new (m_current_context = g_pool.allocate()) Context(m_ast->get_module(handle->module));
	m_current_context->iptr = handle->offset;

	if (handle->symbols) {
		m_current_context->symbols = new SymbolTable(metadata);
		m_current_context->symbols->open_package(handle->package);
		m_current_context->symbols->reserve_fast(handle->fast_count);
	}

	if (handle->generator) {
		const size_t stack_base = m_stack->size() - static_cast<size_t>(signature >= 0 ? signature : (~signature) + 1);
		m_current_context->generator = new WeakReference(Reference::standard, GarbageCollector::instance().alloc<Iterator>(stack_base + 1));
		m_stack->emplace(std::next(m_stack->begin(), stack_base), std::forward<Reference>(*m_current_context->generator));
		m_current_context->generator->data<Iterator>()->construct();
	}
}

void Cursor::call(Module *module, size_t pos, PackageData *package, Class *metadata) {

	m_call_stack.emplace_back(m_current_context);

	new (m_current_context = g_pool.allocate()) Context(module);
	m_current_context->symbols = new SymbolTable(metadata);
	m_current_context->symbols->open_package(package);
	m_current_context->iptr = pos;
}

void Cursor::exit_call() {
	m_current_context->~Context();
	g_pool.deallocate(m_current_context);
	m_current_context = m_call_stack.back();
	m_call_stack.pop_back();
}

bool Cursor::call_in_progress() const {

	if (m_current_context->module != ThreadEntryPoint::instance()) {
		return !m_call_stack.empty();
	}

	return false;
}

bool Cursor::is_in_builtin() const {
	return m_current_context->symbols == nullptr;
}

bool Cursor::is_in_generator() const {
	return m_current_context->generator != nullptr;
}

unique_ptr<SavedState> Cursor::interrupt() {

	unique_ptr<SavedState> state(new SavedState(this, m_current_context));
	m_current_context = m_call_stack.back();
	m_call_stack.pop_back();
	
	while (!m_retrieve_points.empty() && m_retrieve_points.top().call_stack_size > m_call_stack.size()) {
		state->retrieve_points.push(m_retrieve_points.top());
		m_retrieve_points.pop();
	}

	return state;
}

void Cursor::restore(unique_ptr<SavedState> state) {

	m_call_stack.push_back(m_current_context);
	m_current_context = state->context;
	
	while (!state->retrieve_points.empty()) {
		m_retrieve_points.push(state->retrieve_points.top());
		state->retrieve_points.pop();
	}

	state->context = nullptr;
}

void Cursor::destroy(SavedState *state) {
	if (state->context) {
		state->context->~Context();
		g_pool.deallocate(state->context);
	}
}

void Cursor::open_printer(Printer *printer) {
	m_current_context->printers.emplace_back(printer);
}

void Cursor::close_printer() {
	delete m_current_context->printers.back();
	m_current_context->printers.pop_back();
}

Printer *Cursor::printer() {
	if (m_current_context->printers.empty()) {
		return nullptr;
	}
	return m_current_context->printers.back();
}

bool Cursor::load_module(const string &module) {
	
	Module::Info info = m_ast->load_module(module);

	if (UNLIKELY(info.id == Module::invalid_id)) {
		return false;
	}

	if (info.state == Module::not_loaded) {
		call(info.module, 0, &m_ast->global_data());
		m_ast->set_module_state(info.id, Module::ready);
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

void Cursor::set_retrieve_point(size_t offset) {

	RetrievePoint ctx;
	
	ctx.retrieve_offset = offset;
	ctx.stack_size = m_stack->size();
	ctx.call_stack_size = m_call_stack.size();
	ctx.waiting_calls_count = m_waiting_calls.size();
	
	m_retrieve_points.push(ctx);
}

void Cursor::unset_retrieve_point() {
	m_retrieve_points.pop();
}

void Cursor::raise(WeakReference exception) {
	
	if (!m_retrieve_points.empty()) {
		
		RetrievePoint &state = m_retrieve_points.top();
		
		while (state.waiting_calls_count < m_waiting_calls.size()) {
			m_waiting_calls.pop();
		}
		
		while (state.call_stack_size < m_call_stack.size()) {
			exit_call();
		}
		
		m_stack->resize(state.stack_size);
		m_stack->emplace_back(std::forward<Reference>(exception));
		jmp(state.retrieve_offset);
		
		unset_retrieve_point();
	}
	else if (m_parent) {
		throw MintException(m_parent, std::forward<Reference>(exception));
	}
	else {
		Scheduler::instance()->create_exception(std::forward<Reference>(exception));
	}
}

static size_t last_executed_offset(size_t next_offset) {
	return next_offset ? next_offset - 1 : 0;
}

LineInfoList Cursor::dump() {

	LineInfoList dumped_infos;
	dump_module(dumped_infos, m_ast, m_current_context->module, last_executed_offset(m_current_context->iptr));

	for (auto context = m_call_stack.rbegin(); context != m_call_stack.rend(); ++context) {
		dump_module(dumped_infos, m_ast, (*context)->module, last_executed_offset((*context)->iptr));
	}

	if (m_child) {
		for (const LineInfo &info : m_child->dump()) {
			dumped_infos.push_back(info);
		}
	}

	return dumped_infos;
}

size_t Cursor::offset() const {
	return m_current_context->iptr;
}

void Cursor::resume() {
	jmp(m_current_context->module->next_node_offset());
	m_stack->clear();
}

void Cursor::retrieve() {
	
	while (!m_waiting_calls.empty()) {
		m_waiting_calls.pop();
	}

	while (!m_call_stack.empty()) {
		exit_call();
	}

	while (!m_stack->empty()) {
		m_stack->pop_back();
	}

	jmp(m_current_context->module->end());
}

void Cursor::cleanup() {

	if (m_parent == nullptr) {

		while (!m_call_stack.empty()) {
			exit_call();
		}

		m_current_context->printers.clear();
		m_current_context->symbols->clear();
		m_stack->clear();
	}
}

Cursor::Context::Context(Module *module) :
	module(module) {

}

Cursor::Context::~Context() {
	for (Printer *printer : printers) {
		if (!printer->global()) {
			delete printer;
		}
	}
	delete generator;
	delete symbols;
}

void dump_module(LineInfoList &dumped_infos, AbstractSyntaxTree *ast, Module *module, size_t offset) {

	if (module != ThreadEntryPoint::instance()) {
		
		Module::Id id = ast->get_module_id(module);
		string moduleName = ast->get_module_name(module);
		
		if (DebugInfo *infos = ast->get_debug_info(id)) {
			dumped_infos.push_back(LineInfo(id, moduleName, infos->line_number(offset)));
		}
		else {
			dumped_infos.push_back(LineInfo(id, moduleName));
		}
	}
}
