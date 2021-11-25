#include "ast/module.h"

#include <memory>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace mint;

Module::Module() {

}

Module::~Module() {
	for_each(m_symbols.begin(), m_symbols.end(), [] (const pair<string, Symbol *> &ptr) { delete ptr.second; });
	for_each(m_constants.begin(), m_constants.end(), default_delete<Reference>());
	for_each(m_handles.begin(), m_handles.end(), default_delete<Handle>());
}

Module::Handle *Module::findHandle(Id module, size_t offset) const {

	for (auto i = m_handles.rbegin(); i != m_handles.rend(); ++i) {
		if (((*i)->module == module) && ((*i)->offset == offset)) {
			return *i;
		}
	}

	return nullptr;
}

Module::Handle *Module::makeHandle(PackageData *package, Id module, size_t offset) {
	Handle *handler = new Handle { module, offset, package, 0, false, true };
	m_handles.push_back(handler);
	return handler;
}

Module::Handle *Module::makeBuiltinHandle(PackageData *package, Id module, size_t offset) {
	Handle *handler = new Handle { module, offset, package, 0, false, false };
	m_handles.push_back(handler);
	return handler;
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
	m_tree.emplace_back(node);
}

void Module::pushNodes(const std::vector<Node> &nodes) {
	m_tree.insert(m_tree.end(), nodes.begin(), nodes.end());
}

void Module::pushNodes(const initializer_list<Node> &nodes) {
	m_tree.insert(m_tree.end(), nodes.begin(), nodes.end());
}

void Module::replaceNode(size_t offset, const Node &node) {
	m_tree[offset] = node;
}
