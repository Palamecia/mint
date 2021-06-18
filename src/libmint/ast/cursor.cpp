#include "ast/cursor.h"
#include "ast/module.h"
#include "ast/savedstate.h"
#include "ast/abstractsyntaxtree.h"
#include "scheduler/scheduler.h"
#include "scheduler/exception.h"
#include "memory/globaldata.h"
#include "system/assert.h"
#include "system/error.h"
#include "threadentrypoint.h"

using namespace std;
using namespace mint;

void dump_module(LineInfoList &dumped_infos, Module *module, size_t offset);

Cursor::Call::Call(Call &&other) :
	m_function(move(other.function())),
	m_metadata(other.m_metadata),
	m_extraArgs(other.m_extraArgs),
	m_flags(other.m_flags) {

}

Cursor::Call::Call(SharedReference &&function) :
	m_function(move(function)),
	m_metadata(nullptr),
	m_extraArgs(0),
	m_flags(standard_call) {

}

Cursor::Call &Cursor::Call::operator =(Call &&other) {
	m_function = move(other.m_function);
	m_metadata = other.m_metadata;
	m_extraArgs = other.m_extraArgs;
	m_flags = other.m_flags;
	return *this;
}

Cursor::Call::Flags Cursor::Call::getFlags() const {
	return m_flags;
}

void Cursor::Call::setFlags(Flags flags) {
	m_flags = flags;
}

Class *Cursor::Call::getMetadata() const {
	return m_metadata;
}

void Cursor::Call::setMetadata(Class *metadata) {
	m_metadata = metadata;
}

int Cursor::Call::extraArgumentCount() const {
	return m_extraArgs;
}

void Cursor::Call::addExtraArgument() {
	m_extraArgs++;
}

SharedReference &Cursor::Call::function() {
	return m_function;
}

Cursor::Cursor(Module *module, Cursor *parent) :
	m_parent(parent),
	m_child(nullptr),
	m_currentContext(new Context(module)) {

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

	while (!m_callStack.empty()) {
		exitCall();
	}

	delete m_currentContext;

	AbstractSyntaxTree::instance().removeCursor(this);
}

Cursor *Cursor::parent() const {
	return m_parent;
}

Node &Cursor::next() {
	assert(m_currentContext->iptr <= m_currentContext->module->end());
	return m_currentContext->module->at(m_currentContext->iptr++);
}

void Cursor::jmp(size_t pos) {
	m_currentContext->iptr = pos;
}

bool Cursor::call(int module, size_t pos, PackageData *package, Class *metadata) {

	if (module < 0) {
		AbstractSyntaxTree::instance().callBuiltinMethode(module, static_cast<int>(pos), this);
		return false;
	}

	return call(AbstractSyntaxTree::instance().getModule(static_cast<Module::Id>(module)), pos, package, metadata);
}

bool Cursor::call(Module *module, size_t pos, PackageData *package, Class *metadata) {

	m_callStack.push_back(m_currentContext);

	m_currentContext = new Context(module, metadata);
	m_currentContext->symbols.openPackage(package);
	m_currentContext->iptr = pos;

	return true;
}

void Cursor::exitCall() {
	delete m_currentContext;
	m_currentContext = m_callStack.back();
	m_callStack.pop_back();
}

bool Cursor::callInProgress() const {

	if (m_currentContext->module != ThreadEntryPoint::instance()) {
		return !m_callStack.empty();
	}

	return false;
}

Cursor::ExecutionMode Cursor::executionMode() const {
	return m_currentContext->executionMode;
}

void Cursor::setExecutionMode(ExecutionMode mode) {
	m_currentContext->executionMode = mode;
}

unique_ptr<SavedState> Cursor::interrupt() {

	unique_ptr<SavedState> state(new SavedState(m_currentContext));
	m_currentContext = m_callStack.back();
	m_callStack.pop_back();

	while (!m_retrievePoints.empty() && m_retrievePoints.top().callStackSize > m_callStack.size()) {
		state->retrievePoints.push(m_retrievePoints.top());
		m_retrievePoints.pop();
	}

	return state;
}

void Cursor::restore(unique_ptr<SavedState> state) {

	m_callStack.push_back(m_currentContext);
	m_currentContext = state->context;

	while (!state->retrievePoints.empty()) {
		m_retrievePoints.push(state->retrievePoints.top());
		state->retrievePoints.pop();
	}

	state->context = nullptr;
}

void Cursor::openPrinter(Printer *printer) {
	m_currentContext->printers.push(printer);
}

void Cursor::closePrinter() {
	delete m_currentContext->printers.top();
	m_currentContext->printers.pop();
}

vector<SharedReference> &Cursor::stack() {
	return m_stack;
}

Cursor::waiting_call_stack_t &Cursor::waitingCalls() {
	return m_waitingCalls;
}

SymbolTable &Cursor::symbols() {
	return m_currentContext->symbols;
}

Printer *Cursor::printer() {
	if (m_currentContext->printers.empty()) {
		return nullptr;
	}
	return m_currentContext->printers.top();
}

void Cursor::loadModule(const string &module) {

	Module::Infos infos = AbstractSyntaxTree::instance().loadModule(module);

	if (!infos.loaded) {
		call(static_cast<int>(infos.id), 0, &GlobalData::instance());
	}
}

bool Cursor::exitModule() {

	if (callInProgress()) {
		exitCall();
		return true;
	}

	return false;
}

void Cursor::setRetrievePoint(size_t offset) {

	RetrievePoint ctx;

	ctx.retrieveOffset = offset;
	ctx.stackSize = m_stack.size();
	ctx.callStackSize = m_callStack.size();
	ctx.waitingCallsCount = m_waitingCalls.size();

	m_retrievePoints.push(ctx);
}

void Cursor::unsetRetrievePoint() {
	m_retrievePoints.pop();
}

void Cursor::raise(SharedReference exception) {

	if (m_retrievePoints.empty()) {
		Scheduler::instance()->createException(move(exception));
	}
	else {

		RetrievePoint &ctx = m_retrievePoints.top();

		while (ctx.waitingCallsCount < m_waitingCalls.size()) {
			m_waitingCalls.pop();
		}

		while (ctx.callStackSize < m_callStack.size()) {
			exitCall();
		}

		while (ctx.stackSize < m_stack.size()) {
			m_stack.pop_back();
		}

		m_stack.emplace_back(move(exception));
		jmp(ctx.retrieveOffset);

		unsetRetrievePoint();
	}
}

LineInfoList Cursor::dump() {

	LineInfoList dumped_infos;
	dump_module(dumped_infos, m_currentContext->module, m_currentContext->iptr);

	for (auto context = m_callStack.rbegin(); context != m_callStack.rend(); ++context) {
		dump_module(dumped_infos, (*context)->module, (*context)->iptr);
	}

	if (m_child) {
		for (const LineInfo &info : m_child->dump()) {
			dumped_infos.push_back(info);
		}
	}

	return dumped_infos;
}

void Cursor::resume() {
	jmp(m_currentContext->module->nextNodeOffset());
	m_stack.clear();
}

void Cursor::retrieve() {

	while (!m_waitingCalls.empty()) {
		m_waitingCalls.pop();
	}

	while (!m_callStack.empty()) {
		exitCall();
	}

	while (!m_stack.empty()) {
		m_stack.pop_back();
	}

	jmp(m_currentContext->module->end());
}

Cursor::Context::Context(Module *module, Class *metadata) :
	executionMode(Cursor::single_pass),
	symbols(metadata),
	module(module),
	iptr(0) {

}

Cursor::Context::~Context() {
	while (!printers.empty()) {
		Printer *printer = printers.top();
		if (!printer->global()) {
			delete printer;
		}
		printers.pop();
	}
}

void dump_module(LineInfoList &dumped_infos, Module *module, size_t offset) {

	if (module != ThreadEntryPoint::instance()) {

		Module::Id id = AbstractSyntaxTree::instance().getModuleId(module);
		string moduleName = AbstractSyntaxTree::instance().getModuleName(module);

		if (DebugInfos *infos = AbstractSyntaxTree::instance().getDebugInfos(id)) {
			dumped_infos.push_back(LineInfo(moduleName, infos->lineNumber(offset)));
		}
		else {
			dumped_infos.push_back(LineInfo(moduleName));
		}
	}
}
