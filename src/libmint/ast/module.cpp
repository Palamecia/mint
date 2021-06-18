#include "ast/module.h"

#include <memory>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace mint;

Module::Module() {

}

Module::~Module() {

	for_each(m_symbols.begin(), m_symbols.end(), [] (pair<string, Symbol *> ptr) { delete ptr.second; });
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

	Reference *constant = new StrongReference(Reference::const_address | Reference::const_value, data);
	m_constants.push_back(constant);
	return constant;
}

Symbol *Module::makeSymbol(const char *name) {

	auto it = m_symbols.find(name);

	if (it == m_symbols.end()) {
		it = m_symbols.emplace(name, new Symbol(name)).first;
	}

	return it->second;
}

void Module::pushNode(const Node &node) {
	m_tree.push_back(node);
}

void Module::replaceNode(size_t offset, const Node &node) {
	m_tree[offset] = node;
}
