#include "abstractsyntaxtree.h"
#include "Memory/casttool.h"
#include "System/error.h"

using namespace std;

map<int, map<int, AbstractSynatxTree::Builtin>> AbstractSynatxTree::g_builtinMembers;

Call::Call(Reference *ref) : m_ref(ref), m_member(false) {}

Call::Call(const SharedReference &ref) : m_ref(ref), m_member(false) {}

void Call::setMember(bool member) {
	m_member = member;
}

Reference &Call::get() {
	return m_ref.get();
}

bool Call::isMember() const {
	return m_member;
}

AbstractSynatxTree::AbstractSynatxTree() {}

AbstractSynatxTree::~AbstractSynatxTree() {}

Instruction &AbstractSynatxTree::next() {
	return m_currentCtx->module->at(m_currentCtx->iptr++);
}

void AbstractSynatxTree::jmp(size_t pos) {
	m_currentCtx->iptr = pos;
}

void AbstractSynatxTree::call(int module, size_t pos) {

	if (module < 0) {
		g_builtinMembers[module][pos](this);
	}
	else {
		if (m_currentCtx) {
			m_callStack.push(m_currentCtx);
		}
		m_currentCtx = new Context;
		m_currentCtx->module = Module::get(module);
		m_currentCtx->iptr = pos;
	}
}

void AbstractSynatxTree::exitCall() {
	delete m_currentCtx;
	m_currentCtx = m_callStack.top();
	m_callStack.pop();
}

void AbstractSynatxTree::openPrinter(Printer *printer) {
	m_currentCtx->printers.push(printer);
}

void AbstractSynatxTree::closePrinter() {

	if (m_currentCtx->printers.empty()) {
		/// \todo error
	}

	delete m_currentCtx->printers.top();
	m_currentCtx->printers.pop();
}

vector<SharedReference> &AbstractSynatxTree::stack() {
	return m_stack;
}

stack<Call> &AbstractSynatxTree::waitingCalls() {
	return m_waitingCalls;
}

SymbolTable &AbstractSynatxTree::symbols() {
	return m_currentCtx->symbols;
}

Printer *AbstractSynatxTree::printer() {
	if (m_currentCtx->printers.empty()) {
		return nullptr;
	}
	return m_currentCtx->printers.top();
}

void AbstractSynatxTree::loadModule(const std::string &module) {
	call(Module::load(module).moduleId, 0);
}

bool AbstractSynatxTree::exitModule() {

	bool over = m_callStack.empty();

	if (!over) {
		exitCall();
	}

	return !over;
}

void AbstractSynatxTree::setRetrivePoint(size_t offset) {

	RetiveContext ctx;

	ctx.retriveOffset = offset;
	ctx.stackSize = m_stack.size();
	ctx.callStackSize = m_callStack.size();
	ctx.waitingCallsCount = m_waitingCalls.size();

	m_retrivePoints.push(ctx);
}

void AbstractSynatxTree::unsetRetivePoint() {
	m_retrivePoints.pop();
}

void AbstractSynatxTree::raise(SharedReference exception) {

	if (m_retrivePoints.empty()) {
		error("exception : %s", to_string(exception).c_str());
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

AbstractSynatxTree::CallHandler AbstractSynatxTree::getCallHandler() const {
	return m_callStack.size();
}

bool AbstractSynatxTree::callInProgress(CallHandler handler) const {
	return handler < m_callStack.size();
}

pair<int, int> AbstractSynatxTree::createBuiltinMethode(int type, Builtin methode) {

	auto &methodes = g_builtinMembers[type];
	int offset = methodes.size();

	methodes[offset] = methode;

	return pair<int, int>(type, offset);
}
