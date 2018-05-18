#include "ast/cursor.h"
#include "ast/module.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/casttool.h"
#include "system/assert.h"
#include "system/error.h"
#include "threadentrypoint.h"

using namespace std;
using namespace mint;

void dump_module(AbstractSyntaxTree *ast, Module *module, size_t offset);

Cursor::Call::Call(Reference *ref) :
	m_ref(ref),
	m_metadata(nullptr),
	m_member(false) {

}

Cursor::Call::Call(const SharedReference &ref) :
	m_ref(ref),
	m_metadata(nullptr),
	m_member(false) {

}

void Cursor::Call::setMember(bool member) {
	m_member = member;
}

bool Cursor::Call::isMember() const {
	return m_member;
}

void Cursor::Call::setMetadata(Class *metadata) {
	m_metadata = metadata;
}

Class *Cursor::Call::getMetadata() const {
	return m_metadata;
}

Reference &Cursor::Call::function() {
	return *m_ref;
}

Cursor::Cursor(AbstractSyntaxTree *ast, Module *module) :
	m_ast(ast),
	m_currentCtx(new Context(module)) {

}

Cursor::~Cursor() {

	while (!m_callStack.empty()) {
		exitCall();
	}

	delete m_currentCtx;

	m_ast->removeCursor(this);
}

Node &Cursor::next() {
	assert(m_currentCtx->iptr <= m_currentCtx->module->end());
	return m_currentCtx->module->at(m_currentCtx->iptr++);
}

void Cursor::jmp(size_t pos) {
	m_currentCtx->iptr = pos;
}

bool Cursor::call(int module, size_t pos, Class *metadata) {

	if (module < 0) {
		m_ast->callBuiltinMethode(module, pos, this);
		return false;
	}

	m_callStack.push(m_currentCtx);

	m_currentCtx = new Context(m_ast->getModule(module), metadata);
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

	Module::Infos infos = m_ast->loadModule(module);

	if (!infos.loaded) {
		call(infos.id, 0);
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
		error("exception : %s", to_string(*exception).c_str());
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

		m_stack.push_back(exception);
		jmp(ctx.retrieveOffset);

		unsetRetrievePoint();
	}
}

void Cursor::dump() {

	auto callStack = m_callStack;
	Context *context = m_currentCtx;

	dump_module(m_ast, context->module, context->iptr);

	while (!callStack.empty()) {

		context = callStack.top();
		dump_module(m_ast, context->module, context->iptr);
		callStack.pop();
	}
}

void Cursor::resume() {
	jmp(m_currentCtx->module->nextNodeOffset());
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
	throw MintSystemError();
}

Cursor::Context::Context(Module *module, Class *metadata) :
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

void dump_module(AbstractSyntaxTree *ast, Module *module, size_t offset) {

	if (module != ThreadEntryPoint::instance()) {

		Module::Id id = ast->getModuleId(module);

		fprintf(stderr, "  Module '%s', line %lu\n",
				ast->getModuleName(module).c_str(),
				ast->getDebugInfos(id)->lineNumber(offset));
	}
}
