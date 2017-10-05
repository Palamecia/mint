#include "ast/cursor.h"
#include "ast/module.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/casttool.h"
#include "system/error.h"

using namespace std;

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

Cursor::Cursor(AbstractSyntaxTree *ast, Module *module) : m_ast(ast), m_currentCtx(new Context) {

	m_currentCtx->module = module;
	m_currentCtx->iptr = 0;

	m_callbackId = add_error_callback(bind(&Cursor::dumpCallStack, this));
}

Cursor::~Cursor() {

	remove_error_callback(m_callbackId);

	while (!m_callStack.empty()) {
		exitCall();
	}

	delete m_currentCtx;
}

void Cursor::installErrorHandler() {
	set_exit_callback(bind(&Cursor::retrive, this));
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
	return !m_callStack.empty();
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

	bool over = m_callStack.empty();

	if (!over) {
		exitCall();
	}

	return !over;
}

void Cursor::setRetrivePoint(size_t offset) {

	RetiveContext ctx;

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

		RetiveContext &ctx = m_retrivePoints.top();

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

void Cursor::dumpCallStack() {

	fprintf(stderr, "  Module '%s', line %lu\n",
			m_ast->getModuleName(m_currentCtx->module).c_str(),
			m_ast->getDebugInfos(m_ast->getModuleId(m_currentCtx->module))->lineNumber(m_currentCtx->iptr));

	while (!m_callStack.empty()) {
		fprintf(stderr, "  Module '%s', line %lu\n",
				m_ast->getModuleName(m_callStack.top()->module).c_str(),
				m_ast->getDebugInfos(m_ast->getModuleId(m_callStack.top()->module))->lineNumber(m_callStack.top()->iptr));
		m_callStack.pop();
	}
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
	next();
	throw MintSystemError();
}
