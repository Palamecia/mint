#include "abstractsyntaxtree.h"
#include "Memory/casttool.h"
#include "Compiler/compiler.h"
#include "System/filestream.h"
#include "System/error.h"

using namespace std;

vector<Modul *> AbstractSynatxTree::g_moduls;
map<int, map<int, AbstractSynatxTree::Builtin>> AbstractSynatxTree::g_builtinMembers;

Call::Call(Reference *ref) : m_ref(ref), m_isMember(false) {}

Call::Call(const SharedReference &ref) : m_ref(ref), m_isMember(false) {}

void Call::setMember(bool member) {
	m_isMember = member;
}

Reference &Call::get() {
	return m_ref.get();
}

bool Call::isMember() const {
	return m_isMember;
}

AbstractSynatxTree::AbstractSynatxTree() {}

AbstractSynatxTree::~AbstractSynatxTree() {}

Instruction &AbstractSynatxTree::next() {
	return m_currentCtx->modul->at(m_currentCtx->iptr++);
}

void AbstractSynatxTree::jmp(size_t pos) {
	m_currentCtx->iptr = pos;
}

void AbstractSynatxTree::call(int modul, size_t pos) {

	if (modul < 0) {
		g_builtinMembers[modul][pos](this);
	}
	else {
		if (m_currentCtx) {
			m_callStack.push(m_currentCtx);
		}
		m_currentCtx = new Context;
		m_currentCtx->modul = g_moduls[modul];
		m_currentCtx->iptr = pos;
	}
}

void AbstractSynatxTree::exit_call() {
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

Modul::Context AbstractSynatxTree::createModul() {

	Modul::Context ctx;

	ctx.modulId = g_moduls.size();
	ctx.modul = new Modul;
	g_moduls.push_back(ctx.modul);

	return ctx;
}

Modul::Context AbstractSynatxTree::continueModul() {

	Modul::Context ctx;

	ctx.modulId = 0;
	ctx.modul = g_moduls.front();
	/// \todo remove last instruction

	return ctx;
}

void AbstractSynatxTree::loadModul(const std::string &path) {

	auto it = Modul::cache.find(path);
	if (it == Modul::cache.end()) {
		it = Modul::cache.insert({path, createModul()}).first;

		Compiler compiler;
		FileStream stream(path); /// \todo make system path using inculde path

		compiler.build(&stream, it->second);
	}

	call(it->second.modulId, 0);
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

pair<int, int> AbstractSynatxTree::createBuiltinMethode(int type, Builtin methode) {

	auto &methodes = g_builtinMembers[type];
	int offset = methodes.size();

	methodes[offset] = methode;

	return pair<int, int>(type, offset);
}

void AbstractSynatxTree::clearCache() {

	for (Modul *modul : g_moduls) {
		delete modul;
	}

	Modul::cache.clear();
	g_moduls.clear();
}
