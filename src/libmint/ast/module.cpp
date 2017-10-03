#include "ast/module.h"

#include <cstring>
#include <algorithm>

using namespace std;

Instruction &Module::at(size_t idx) {
	return m_data[idx];
}

size_t Module::end() const {
	return m_data.size() - 1;
}

Module::Module() {}

Module::~Module() {

	for_each(m_symbols.begin(), m_symbols.end(), [](char *symbol) { delete [] symbol; });

	for_each(m_constants.begin(), m_constants.end(), [](Reference *constant) { delete constant; });
}

char *Module::makeSymbol(const char *name) {

	for (char *symbol : m_symbols) {
		if (!strcmp(symbol, name)) {
			return symbol;
		}
	}

	char *symbol = new char [strlen(name) + 1];
	strcpy(symbol, name);
	m_symbols.push_back(symbol);
	return symbol;
}

Reference *Module::makeConstant(Data *data) {

	Reference *constant = new Reference(Reference::const_ref | Reference::const_value, data);
	m_constants.push_back(constant);
	return constant;
}

void Module::pushInstruction(const Instruction &instruction) {
	m_data.push_back(instruction);
}

void Module::replaceInstruction(size_t offset, const Instruction &instruction) {
	m_data[offset] = instruction;
}

size_t Module::nextInstructionOffset() const {
	return m_data.size();
}
