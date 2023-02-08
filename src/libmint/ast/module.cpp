/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/ast/module.h"

#include <memory>
#include <algorithm>
#include <cstring>

using namespace std;
using namespace mint;

Module::Module() {

}

Module::~Module() {
	for_each(m_symbols.begin(), m_symbols.end(), [] (auto &ptr) { delete ptr.second; });
	for_each(m_constants.begin(), m_constants.end(), default_delete<Reference>());
	for_each(m_handles.begin(), m_handles.end(), default_delete<Handle>());
}

Module::Handle *Module::find_handle(Id module, size_t offset) const {

	for (auto i = m_handles.rbegin(); i != m_handles.rend(); ++i) {
		if (((*i)->module == module) && ((*i)->offset == offset)) {
			return *i;
		}
	}

	return nullptr;
}

Module::Handle *Module::make_handle(PackageData *package, Id module, size_t offset) {
	Handle *handler = new Handle { module, offset, package, 0, false, true };
	m_handles.push_back(handler);
	return handler;
}

Module::Handle *Module::make_builtin_handle(PackageData *package, Id module, size_t offset) {
	Handle *handler = new Handle { module, offset, package, 0, false, false };
	m_handles.push_back(handler);
	return handler;
}

Reference *Module::make_constant(Data *data) {

	Reference *constant = new StrongReference(Reference::const_address | Reference::const_value, data);
	m_constants.push_back(constant);
	return constant;
}

Symbol *Module::make_symbol(const char *name) {

	auto it = m_symbols.find(name);

	if (it == m_symbols.end()) {
		it = m_symbols.emplace(name, new Symbol(name)).first;
	}

	return it->second;
}

void Module::push_node(const Node &node) {
	m_tree.emplace_back(node);
}

void Module::push_nodes(const std::vector<Node> &nodes) {
	m_tree.insert(m_tree.end(), nodes.begin(), nodes.end());
}

void Module::push_nodes(const initializer_list<Node> &nodes) {
	m_tree.insert(m_tree.end(), nodes.begin(), nodes.end());
}

void Module::replace_node(size_t offset, const Node &node) {
	m_tree[offset] = node;
}
