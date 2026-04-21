/**
 * Copyright (c) 2026 Gauvain CHERY.
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
#include "mint/ast/classregister.h"
#include "mint/ast/node.h"
#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/memory/reference.h"

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <cstring>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace mint;

Module::Module() {
	register_root();
}

Module::~Module() {
	unregister_root();
}

Module::Handle* Module::find_handle(std::size_t offset) const {
	auto handles = std::ranges::reverse_view(_handles);
	auto it = std::ranges::find(handles, offset, &Handle::offset);
	if (it != handles.end()) {
		return it->get();
	}
	return nullptr;
}

Module::Handle& Module::get_handle(PackageData& package, std::size_t offset) {
	auto handles = std::ranges::reverse_view(_handles);
	auto it = std::ranges::find(handles, offset, &Handle::offset);
	if (it != handles.end()) {
		return **it;
	}
	return make_handle(package, offset);
}

Module::Handle& Module::make_handle(PackageData& package, std::size_t offset) {
	return *_handles.emplace_back(std::make_unique<Handle>(Handle {
	    .module = *this,
	    .offset = offset,
	    .package = package,
	    .fast_count = 0,
	    .symbols = true,
	}));
}

Module::Handle& Module::make_builtin_handle(PackageData& package, std::size_t offset) {
	return *_handles.emplace_back(std::make_unique<Handle>(Handle {
	    .module = *this,
	    .offset = offset,
	    .package = package,
	    .fast_count = 0,
	}));
}

Module::Handle& Module::make_builtin_async_handle(PackageData& package, std::size_t offset) {
	return *_handles.emplace_back(std::make_unique<Handle>(Handle {
	    .module = *this,
	    .offset = offset,
	    .package = package,
	    .fast_count = 0,
	    .async = true,
	}));
}

Reference* Module::make_constant(Data& data) {
	return _constants
	    .emplace_back(std::make_unique<WeakReference>(Reference::const_address | Reference::const_value, data))
	    .get();
}

ClassDescription* mint::Module::make_class(AbstractSyntaxTree& ast, const std::string& name) {
	return _classes.emplace_back(std::make_unique<ClassDescription>(ast, name)).get();
}

Symbol* Module::make_symbol(const std::string& name) {
	auto it = _symbols.find(name);
	if (it == _symbols.end()) {
		it = _symbols.emplace(name, std::make_unique<Symbol>(name)).first;
	}
	return it->second.get();
}

void Module::add_internal_register(std::unique_ptr<ClassRegister>&& class_register) {
	_internal_registers.emplace_back(std::move(class_register));
}

void Module::cleanup_memory() {
	for (const auto& class_register : _internal_registers) {
		class_register->cleanup_memory();
	}
	_constants.clear();
}

void Module::cleanup_metadata() {
	for (const auto& class_register : _internal_registers) {
		class_register->cleanup_metadata();
	}
	_classes.clear();
}

void Module::mark() {
	for (const auto& constant : _constants) {
		constant->data().mark();
	}
	for (const auto& desc : _classes) {
		desc->mark();
	}
}

void Module::push_node(const Node& node) {
	_tree.emplace_back(node);
}

void Module::push_nodes(const std::vector<Node>& nodes) {
	_tree.append_range(nodes);
}

void Module::push_nodes(const std::initializer_list<Node>& nodes) {
	_tree.append_range(nodes);
}

void Module::replace_node(std::size_t offset, const Node& node) {
	_tree[offset] = node;
}
