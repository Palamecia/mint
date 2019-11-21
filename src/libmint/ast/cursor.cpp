#include "ast/cursor.h"
#include "ast/module.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/casttool.h"
#include "system/assert.h"
#include "system/error.h"
#include "scheduler/scheduler.h"
#include "scheduler/exception.h"
#include "threadentrypoint.h"

using namespace std;
using namespace mint;

void dump_module(LineInfoList &dumped_infos, Module *module, size_t offset);

Cursor::Call::Call(Call &&other) :
	m_function(other.function()),
	m_metadata(other.m_metadata),
	m_extraArgs(other.m_extraArgs),
	m_member(other.m_member) {

}

Cursor::Call::Call(SharedReference &function) :
	m_function(function),
	m_metadata(nullptr),
	m_extraArgs(0),
	m_member(false) {

}

Cursor::Call::Call(SharedReference &&function) :
	m_function(function),
	m_metadata(nullptr),
	m_extraArgs(0),
	m_member(false) {

}

bool Cursor::Call::isMember() const {
	return m_member;
}

void Cursor::Call::setMember(bool member) {
	m_member = member;
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
	m_currentCtx(new Context(module)) {

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

	delete m_currentCtx;

	AbstractSyntaxTree::instance().removeCursor(this);
}

Cursor *Cursor::parent() const {
	return m_parent;
}

Node &Cursor::next() {
	assert(m_currentCtx->iptr <= m_currentCtx->module->end());
	return m_currentCtx->module->at(m_currentCtx->iptr++);
}

void Cursor::jmp(size_t pos) {
	m_currentCtx->iptr = pos;
}

bool Cursor::call(int module, size_t pos, PackageData *package, Class *metadata) {

	if (module < 0) {
		AbstractSyntaxTree::instance().callBuiltinMethode(module, pos, this);
		return false;
	}

	return call(AbstractSyntaxTree::instance().getModule(module), pos, package, metadata);
}

bool Cursor::call(Module *module, size_t pos, PackageData *package, Class *metadata) {

	m_callStack.push(m_currentCtx);

	m_currentCtx = new Context(module, metadata);
	m_currentCtx->symbols.openPackage(package);
	m_currentCtx->iptr = pos;

	return true;
}

void Cursor::exitCall() {
	delete m_currentCtx;
	m_currentCtx = m_callStack.top();
	m_callStack.pop();
}

bool Cursor::callInProgress() const {

	if (m_currentCtx->module != ThreadEntryPoint::instance()) {
		return !m_callStack.empty();
	}

	return false;
}

Cursor::ExecutionMode Cursor::executionMode() const {
	return m_currentCtx->executionMode;
}

void Cursor::setExecutionMode(ExecutionMode mode) {
	m_currentCtx->executionMode = mode;
}

Cursor::Context *Cursor::interrupt() {
	Context *context = m_currentCtx;
	m_currentCtx = m_callStack.top();
	m_callStack.pop();
	return context;
}

void Cursor::restore(Context *context) {
	m_callStack.push(m_currentCtx);
	m_currentCtx = context;
}

void Cursor::openPrinter(Printer *printer) {
	m_currentCtx->printers.push(printer);
}

void Cursor::closePrinter() {
	delete m_currentCtx->printers.top();
	m_currentCtx->printers.pop();
}

vector<SharedReference> &Cursor::stack() {
	return m_stack;
}

stack<Cursor::Call> &Cursor::waitingCalls() {
	return m_waitingCalls;
}

SymbolTable &Cursor::symbols() {
	return m_currentCtx->symbols;
}

Printer *Cursor::printer() {
	if (m_currentCtx->printers.empty()) {
		return nullptr;
	}
	return m_currentCtx->printers.top();
}

void Cursor::loadModule(const string &module) {

	Module::Infos infos = AbstractSyntaxTree::instance().loadModule(module);

	if (!infos.loaded) {
		call(infos.id, 0, &GlobalData::instance());
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
		Scheduler::instance()->createException(exception);
	}
	else {

		RetrievePoint &ctx = m_retrievePoints.top();

		while (m_waitingCalls.size() > ctx.waitingCallsCount) {
			m_waitingCalls.pop();
		}

		while (m_callStack.size() > ctx.callStackSize) {
			exitCall();
		}

		while (m_stack.size() > ctx.stackSize) {
			m_stack.pop_back();
		}

		m_stack.emplace_back(exception);
		jmp(ctx.retrieveOffset);

		unsetRetrievePoint();
	}
}

LineInfoList Cursor::dump() {

	LineInfoList dumped_infos;
	auto callStack = m_callStack;
	Context *context = m_currentCtx;

	dump_module(dumped_infos, context->module, context->iptr);

	while (!callStack.empty()) {

		context = callStack.top();
		dump_module(dumped_infos, context->module, context->iptr);
		callStack.pop();
	}

	if (m_child) {
		for (const LineInfo &info : m_child->dump()) {
			dumped_infos.push_back(info);
		}
	}

	return dumped_infos;
}

void Cursor::resume() {
	jmp(m_currentCtx->module->nextNodeOffset());
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

	jmp(m_currentCtx->module->end());
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
