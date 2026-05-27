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

#include "mint/memory/global_data.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/class_register.h"
#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/class.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

using namespace mint;

FunctionData::FunctionData(AbstractSyntaxTree& ast) :
    ClassRegister(ast) {}

const FunctionData* FunctionData::get_function_data() const {
	return this;
}

FunctionData* FunctionData::get_function_data() {
	return this;
}

PackageData::PackageData(AbstractSyntaxTree& ast, const std::string& name) :
    ClassRegister(ast),
    _name(name),
    _symbols(ast.global_data()) {
	register_root();
}

PackageData::~PackageData() {
	unregister_root();
}

Symbol PackageData::name() const {
	return _name;
}

std::string PackageData::full_name() const {
	if (const auto* package = get_owner_package()) {
		if (package != &ast().global_data()) {
			return package->full_name() + "." + name().str();
		}
	}
	return name().str();
}

PackageData::Path PackageData::get_path() const {
	if (const auto* package = get_owner_package()) {
		return {package->get_path(), name()};
	}
	return {name()};
}

PackageData& PackageData::get_package(const Symbol& name) {
	auto it = _packages.find(name);
	if (it == _packages.end()) {
		constexpr auto flags = Reference::global | Reference::const_address | Reference::const_value;
		auto package = std::make_unique<PackageData>(ast(), name.str());
		package->set_owner_register(this);
		_symbols.emplace(name, make_reference<Package>(flags, *package));
		it = _packages.emplace(name, std::move(package)).first;
	}
	return *it->second;
}

PackageData* PackageData::find_package(const Symbol& name) const {
	auto it = _packages.find(name);
	if (it != _packages.end()) {
		return it->second.get();
	}
	return nullptr;
}

Class* PackageData::find_class(const Symbol& name) const {
	if (auto it = _symbols.find(name); it != _symbols.end() && it->second.data().format() == Data::Format::object
	                                   && is_class(it->second.data<Object>())) {
		return &it->second.data<Object>().metadata;
	}
	return nullptr;
}

ClassRegister* PackageData::locate(const Symbol& symbol) const {
	if (auto* class_decription = find_class_description(symbol)) {
		return class_decription;
	}
	if (auto* child_package = find_package(symbol)) {
		return child_package;
	}
	return nullptr;
}

const PackageData* PackageData::get_package_data() const {
	return this;
}

PackageData* PackageData::get_package_data() {
	return this;
}

void PackageData::cleanup_memory() {

	ClassRegister::cleanup_memory();

	for (auto& package : _packages) {
		package.second->cleanup_memory();
	}

	for (auto symbol = _symbols.begin(); symbol != _symbols.end();) {
		if (is_class(symbol->second)) {
			symbol = next(symbol);
		}
		else {
			symbol = _symbols.erase(symbol);
		}
	}
}

void PackageData::cleanup_metadata() {

	ClassRegister::cleanup_metadata();

	_symbols.clear();

	for (auto& package : _packages) {
		package.second->cleanup_metadata();
	}

	_packages.clear();
}

void PackageData::mark() {
	_symbols.mark();
}

GlobalData::GlobalData(AbstractSyntaxTree& ast) :
    PackageData(ast, "(default)") {}

void GlobalData::cleanup_builtin() {
	// cleanup builtin classes
	std::ranges::for_each(_builtin, [](auto& builtin) {
		builtin.reset();
	});
}
