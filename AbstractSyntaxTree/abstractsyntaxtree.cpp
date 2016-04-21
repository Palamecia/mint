#include "abstractsyntaxtree.h"

using namespace std;

AbstractSynatxTree::AbstractSynatxTree() {

}

AbstractSynatxTree::~AbstractSynatxTree() {
	for (auto modul : m_moduls) {
		delete modul;
	}
}

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
	m_currentCtx->modul = m_moduls[modul];
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

ModulContext AbstractSynatxTree::createModul() {

	ModulContext ctx;

	ctx.modul.first = m_moduls.size();
	m_moduls.push_back(new Modul);
	ctx.modul.second = m_moduls.back();

	ctx.defs.first = m_moduls.size();
	m_moduls.push_back(new Modul);
	ctx.defs.second = m_moduls.back();

	return ctx;
}
