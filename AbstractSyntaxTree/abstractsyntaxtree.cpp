#include "abstractsyntaxtree.h"
#include "Compiler/compiler.h"
#include "System/filestream.h"

using namespace std;

vector<Modul *> AbstractSynatxTree::g_moduls;

AbstractSynatxTree::AbstractSynatxTree() {}

AbstractSynatxTree::~AbstractSynatxTree() {}

Instruction &AbstractSynatxTree::next() {
	return m_currentCtx->modul->at(m_currentCtx->iptr++);
}

void AbstractSynatxTree::jmp(size_t pos) {
	m_currentCtx->iptr = pos;
}

void AbstractSynatxTree::call(size_t modul, size_t pos) {
	if (m_currentCtx) {
		m_callStack.push(m_currentCtx);
	}
	m_currentCtx = new Context;
	m_currentCtx->modul = g_moduls[modul];
	m_currentCtx->iptr = pos;
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
