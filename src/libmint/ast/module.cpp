#include "ast/module.h"

#include <memory>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace mint;

Module::Module() {

}

Module::~Module() {

	for_each(m_symbols.begin(), m_symbols.end(), default_delete<char []>());
	for_each(m_constants.begin(), m_constants.end(), default_delete<Reference>());
}

Node &Module::at(size_t idx) {
	return m_tree[idx];
}

size_t Module::end() const {
	return m_tree.size() - 1;
}

size_t Module::nextNodeOffset() const {
	return m_tree.size();
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

void Module::pushNode(const Node &node) {
	m_tree.push_back(node);
}

void Module::replaceNode(size_t offset, const Node &node) {
	m_tree[offset] = node;
}
