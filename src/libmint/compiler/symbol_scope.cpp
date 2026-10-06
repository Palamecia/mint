/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/descriptions.h"
#include "mint/config.h"
#include "mint/program/symbol.h"
#include "mint/memory/class.h"
#include <cassert>
#include <variant>
#include <vector>

using namespace mint;

mint::SymbolScope::SymbolScope(SymbolScope* owner) :
    _owner(owner) {}

const SymbolScope& SymbolScope::get_root_scope() const {
	if (_owner) {
		return _owner->get_root_scope();
	}
	return *this;
}

SymbolScope& SymbolScope::get_root_scope() {
	if (_owner) {
		return _owner->get_root_scope();
	}
	return *this;
}

const SymbolScope* SymbolScope::get_owner_scope() const {
	return _owner;
}

SymbolScope* SymbolScope::get_owner_scope() {
	return _owner;
}

SymbolDefinition SymbolScope::locate(const SymbolPath& symbols) const {

	// error("expected package or class name got empty path");
	// error("expected package or class name got '{}'", symbol->str());
	// error("class '{}' was not declared", symbols.to_string());

	auto symbol = symbols.begin();
	if (symbol == symbols.end()) [[unlikely]] {
		return {};
	}

	auto definition = locate(*symbol);
	while (++symbol != symbols.end()) {
		definition = std::visit(Overloaded {
		                            [&symbol](const PackageDescription& definition) -> SymbolDefinition {
			                            return definition.locate(*symbol);
		                            },
		                            [&symbol](const ClassDescription& definition) -> SymbolDefinition {
			                            return definition.locate(*symbol);
		                            },
		                            [](const FunctionDescription&) -> SymbolDefinition {
			                            return {};
		                            },
		                            [](const VariableDescription&) -> SymbolDefinition {
			                            return {};
		                            },
		                            [](std::monostate&) -> SymbolDefinition {
			                            return {};
		                            },
		                        },
		    definition);
	}

	return definition;
}

/*void SymbolScope::cleanup_memory() {
	std::ranges::for_each(std::views::reverse(_defined_classes), [](const auto& entry) {
		entry.desc.get().cleanup_memory();
	});
}

void SymbolScope::cleanup_metadata() {
	std::ranges::for_each(std::views::reverse(_defined_classes), [](const auto& entry) {
		entry.desc.get().cleanup_metadata();
	});
	_defined_classes.clear();
}*/
