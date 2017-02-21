#include "ast/abstractsyntaxtree.h"
#include "memory/casttool.h"
#include "system/error.h"

using namespace std;

map<int, map<int, AbstractSyntaxTree::Builtin>> AbstractSyntaxTree::g_builtinMembers;

Call::Call(Reference *ref) : m_ref(ref), m_member(false) {}

Call::Call(const SharedReference &ref) : m_ref(ref), m_member(false) {}

void Call::setMember(bool member) {
	m_member = member;
}

Reference &Call::function() {
	return *m_ref;
}

bool Call::isMember() const {
	return m_member;
}

AbstractSyntaxTree::AbstractSyntaxTree(size_t rootModuleId) : m_currentCtx(new Context) {

	m_currentCtx->module = Module::get(rootModuleId);
	m_currentCtx->iptr = 0;

	m_callbackId = add_error_callback(bind(&AbstractSyntaxTree::dumpCallStack, this));
}

AbstractSyntaxTree::~AbstractSyntaxTree() {

	remove_error_callback(m_callbackId);

	while (!m_callStack.empty()) {
		exitCall();
	}

	delete m_currentCtx;
}

Instruction &AbstractSyntaxTree::next() {
	return m_currentCtx->module->at(m_currentCtx->iptr++);
}

void AbstractSyntaxTree::jmp(size_t pos) {
	m_currentCtx->iptr = pos;
}

bool AbstractSyntaxTree::call(int module, size_t pos) {

	if (module < 0) {
		g_builtinMembers[module][pos](this);
		return false;
	}

	m_callStack.push(m_currentCtx);

	m_currentCtx = new Context;
	m_currentCtx->module = Module::get(module);
	m_currentCtx->iptr = pos;

	return true;
}

void AbstractSyntaxTree::exitCall() {
	delete m_currentCtx;
	m_currentCtx = m_callStack.top();
	m_callStack.pop();
}

void AbstractSyntaxTree::openPrinter(Printer *printer) {
	m_currentCtx->printers.push(printer);
}

void AbstractSyntaxTree::closePrinter() {
	delete m_currentCtx->printers.top();
	m_currentCtx->printers.pop();
}

vector<SharedReference> &AbstractSyntaxTree::stack() {
	return m_stack;
}

stack<Call> &AbstractSyntaxTree::waitingCalls() {
	return m_waitingCalls;
}

SymbolTable &AbstractSyntaxTree::symbols() {
	return m_currentCtx->symbols;
}

Printer *AbstractSyntaxTree::printer() {
	if (m_currentCtx->printers.empty()) {
		return nullptr;
	}
	return m_currentCtx->printers.top();
}

void AbstractSyntaxTree::loadModule(const std::string &module) {
	call(Module::load(module).moduleId, 0);
}

bool AbstractSyntaxTree::exitModule() {

	bool over = m_callStack.empty();

	if (!over) {
		exitCall();
	}

	return !over;
}

void AbstractSyntaxTree::setRetrivePoint(size_t offset) {

	RetiveContext ctx;

	ctx.retriveOffset = offset;
	ctx.stackSize = m_stack.size();
	ctx.callStackSize = m_callStack.size();
	ctx.waitingCallsCount = m_waitingCalls.size();

	m_retrivePoints.push(ctx);
}

void AbstractSyntaxTree::unsetRetivePoint() {
	m_retrivePoints.pop();
}

void AbstractSyntaxTree::raise(SharedReference exception) {

	if (m_retrivePoints.empty()) {
		error("exception : %s", to_string(*exception).c_str());
	}
	else {

		RetiveContext &ctx = m_retrivePoints.top();

		while (m_waitingCalls.size() > ctx.waitingCallsCount) {
			m_waitingCalls.pop();
		}

		while (m_callStack.size() > ctx.callStackSize) {
			m_callStack.pop();
		}

		while (m_stack.size() > ctx.stackSize) {
			m_stack.pop_back();
		}

		m_stack.push_back(exception);
		jmp(ctx.retriveOffset);

		unsetRetivePoint();
	}
}

AbstractSyntaxTree::CallHandler AbstractSyntaxTree::getCallHandler() const {
	return m_callStack.size();
}

bool AbstractSyntaxTree::callInProgress(CallHandler handler) const {
	return handler < m_callStack.size();
}

pair<int, int> AbstractSyntaxTree::createBuiltinMethode(int type, Builtin methode) {

	auto &methodes = g_builtinMembers[-type];
	int offset = methodes.size();

	methodes[offset] = methode;

	return pair<int, int>(-type, offset);
}

void AbstractSyntaxTree::dumpCallStack() {

	fprintf(stderr, "  Module '%s', line %lu\n", Module::name(m_currentCtx->module).c_str(), Module::debug(Module::id(m_currentCtx->module))->lineNumber(m_currentCtx->iptr));

	while (!m_callStack.empty()) {
		fprintf(stderr, "  Module '%s', line %lu\n", Module::name(m_callStack.top()->module).c_str(), Module::debug(Module::id(m_callStack.top()->module))->lineNumber(m_callStack.top()->iptr));
		m_callStack.pop();
	}
}
