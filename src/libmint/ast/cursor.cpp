#include "ast/cursor.h"
#include "ast/module.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/casttool.h"
#include "system/error.h"
#include "threadentrypoint.h"

using namespace std;

void dump_module(AbstractSyntaxTree *ast, Module *module, size_t offset);

Cursor::Call::Call(Reference *ref) : m_ref(ref), m_member(false) {}

Cursor::Call::Call(const SharedReference &ref) : m_ref(ref), m_member(false) {}

void Cursor::Call::setMember(bool member) {
	m_member = member;
}

Reference &Cursor::Call::function() {
	return *m_ref;
}

bool Cursor::Call::isMember() const {
	return m_member;
}

Cursor::Cursor(AbstractSyntaxTree *ast, Module *module) :
	m_ast(ast),
	m_currentCtx(new Context) {

	m_currentCtx->module = module;
	m_currentCtx->iptr = 0;
}

Cursor::~Cursor() {

	while (!m_callStack.empty()) {
		exitCall();
	}

	delete m_currentCtx;
}

Node &Cursor::next() {
	return m_currentCtx->module->at(m_currentCtx->iptr++);
}

void Cursor::jmp(size_t pos) {
	m_currentCtx->iptr = pos;
}

bool Cursor::call(int module, size_t pos) {

	if (module < 0) {
		m_ast->callBuiltinMethode(module, pos, this);
		return false;
	}

	m_callStack.push(m_currentCtx);

	m_currentCtx = new Context;
	m_currentCtx->module = m_ast->getModule(module);
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

void Cursor::loadModule(const std::string &module) {
	call(m_ast->loadModule(module).id, 0);
}

bool Cursor::exitModule() {

	if (callInProgress()) {
		exitCall();
		return true;
	}

	return false;
}

void Cursor::setRetrivePoint(size_t offset) {

	RetivePoint ctx;

	ctx.retriveOffset = offset;
	ctx.stackSize = m_stack.size();
	ctx.callStackSize = m_callStack.size();
	ctx.waitingCallsCount = m_waitingCalls.size();

	m_retrivePoints.push(ctx);
}

void Cursor::unsetRetivePoint() {
	m_retrivePoints.pop();
}

void Cursor::raise(SharedReference exception) {

	if (m_retrivePoints.empty()) {
		error("exception : %s", to_string(*exception).c_str());
	}
	else {

		RetivePoint &ctx = m_retrivePoints.top();

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
		jmp(ctx.retriveOffset);

		unsetRetivePoint();
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

void Cursor::retrive() {

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

void dump_module(AbstractSyntaxTree *ast, Module *module, size_t offset) {

	if (module != ThreadEntryPoint::instance()) {

		Module::Id id = ast->getModuleId(module);

		fprintf(stderr, "  Module '%s', line %lu\n",
				ast->getModuleName(module).c_str(),
				ast->getDebugInfos(id)->lineNumber(offset));
	}
}
