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

#include "mint/ast/class_register.h"
#include "mint/ast/class_description.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/global_data.h"
#include "mint/memory/class.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/error.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace mint;

ClassRegister::Path::Path(const Symbol& symbol) :
    _symbols({symbol}) {}

ClassRegister::Path::Path(std::initializer_list<Symbol> symbols) :
    _symbols(symbols) {}

ClassRegister::Path::Path(const Path& other, const Symbol& symbol) :
    _symbols(other._symbols) {
	_symbols.push_back(symbol);
}

ClassRegister::Path::Path(const std::string& path) {
	for (const auto symbol : std::views::split(path, std::string("."))) {
		_symbols.emplace_back(std::string_view(symbol));
	}
}

const ClassDescription& ClassRegister::Path::locate(const ClassRegister& root_register) const {

	auto symbol = _symbols.begin();

	if (symbol == _symbols.end()) [[unlikely]] {
		error("expected package or class name got empty path");
	}

	const auto* class_register = root_register.locate(*symbol);
	if (class_register == nullptr) {
		class_register = root_register.ast().global_data().locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	while (++symbol != _symbols.end()) {
		class_register = class_register->locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	const auto* class_description = dynamic_cast<const ClassDescription*>(class_register);
	if (class_description == nullptr) [[unlikely]] {
		error("class '{}' was not declared", to_string());
	}
	return *class_description;
}

ClassDescription& ClassRegister::Path::locate(ClassRegister& root_register) const {

	auto symbol = _symbols.begin();

	if (symbol == _symbols.end()) [[unlikely]] {
		error("expected package or class name got empty path");
	}

	auto* class_register = root_register.locate(*symbol);
	if (class_register == nullptr) {
		class_register = root_register.ast().global_data().locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	while (++symbol != _symbols.end()) {
		class_register = class_register->locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	auto* class_description = dynamic_cast<ClassDescription*>(class_register);
	if (class_description == nullptr) [[unlikely]] {
		error("class '{}' was not declared", to_string());
	}
	return *class_description;
}

std::string ClassRegister::Path::to_string() const {
	std::string path;
	for (auto i = _symbols.begin(); i != _symbols.end(); ++i) {
		if (i != _symbols.begin()) {
			path += ".";
		}
		path += i->str();
	}
	return path;
}

void ClassRegister::Path::append_symbol(const Symbol& symbol) {
	_symbols.push_back(symbol);
}

void ClassRegister::Path::clear() {
	_symbols.clear();
}

ClassRegister::ClassRegister(AbstractSyntaxTree& ast) :
    _ast(ast) {}

const ClassRegister& ClassRegister::get_root_register() const {
	if (_owner) {
		return _owner->get_root_register();
	}
	return *this;
}

ClassRegister& ClassRegister::get_root_register() {
	if (_owner) {
		return _owner->get_root_register();
	}
	return *this;
}

const ClassRegister* ClassRegister::get_owner_register() const {
	return _owner;
}

ClassRegister* ClassRegister::get_owner_register() {
	return _owner;
}

void ClassRegister::set_owner_register(ClassRegister* owner) {
	_owner = owner;
}

const PackageData* ClassRegister::get_owner_package() const {
	if (_owner == nullptr) {
		return nullptr;
	}
	if (auto* package = _owner->get_package_data()) {
		return package;
	}
	return _owner->get_owner_package();
}

PackageData* ClassRegister::get_owner_package() {
	if (_owner == nullptr) {
		return nullptr;
	}
	if (auto* package = _owner->get_package_data()) {
		return package;
	}
	return _owner->get_owner_package();
}

ClassDescription* ClassRegister::find_class_description(const Symbol& name) const {
	if (auto it = std::ranges::find(_defined_classes, name,
	        [](const auto& entry) {
		        return entry.desc.get().name();
	        });
	    it != _defined_classes.end()) {
		return &it->desc.get();
	}
	return nullptr;
}

void ClassRegister::register_class_description(ClassDescription& desc, Reference::Flags flags) {
	assert(desc._owner == nullptr);
	desc._owner = this;
	_defined_classes.push_back({
	    .desc = std::ref(desc),
	    .flags = flags,
	});
}

ClassRegister* ClassRegister::locate(const Symbol& symbol) const {
	if (auto* class_description = find_class_description(symbol)) {
		return class_description;
	}
	return nullptr;
}

const FunctionData* ClassRegister::get_function_data() const {
	return nullptr;
}

FunctionData* ClassRegister::get_function_data() {
	return nullptr;
}

const PackageData* ClassRegister::get_package_data() const {
	return nullptr;
}

PackageData* ClassRegister::get_package_data() {
	return nullptr;
}

void ClassRegister::cleanup_memory() {
	std::ranges::for_each(std::views::reverse(_defined_classes), [](const auto& entry) {
		entry.desc.get().cleanup_memory();
	});
}

void ClassRegister::cleanup_metadata() {
	std::ranges::for_each(std::views::reverse(_defined_classes), [](const auto& entry) {
		entry.desc.get().cleanup_metadata();
	});
	_defined_classes.clear();
}
