#include "ast/module.h"

#include <memory>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace mint;

Module::Module() {

}

Module::~Module() {

	for_each(m_symbols.begin(), m_symbols.end(), default_delete<const char []>());
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

Reference *Module::makeConstant(Data *data) {

	Reference *constant = StrongReference::create(data);
	m_constants.push_back(constant);
	return constant;
}

const char *Module::makeSymbol(const char *name) {

	auto it = m_symbols.find(name);

	if (it == m_symbols.end()) {
		char *symbol = new char [strlen(name) + 1];
		strcpy(symbol, name);
		it = m_symbols.insert(symbol).first;
	}

	return *it;
}

void Module::pushNode(const Node &node) {
	m_tree.push_back(node);
}

void Module::replaceNode(size_t offset, const Node &node) {
	m_tree[offset] = node;
}

bool Module::symbol_comp::operator ()(const char *left, const char *right) const {
	return strcmp(left, right) < 0;
}
