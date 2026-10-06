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
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/program/symbol.h"
#include "mint/memory/class.h"
#include <cassert>
#include <utility>
#include <variant>
#include <vector>

using namespace mint;

mint::SymbolDefinition::SymbolDefinition(const PackageDescription& description) :
    variant(description) {}

mint::SymbolDefinition::SymbolDefinition(const ClassDescription& description) :
    variant(description) {}

mint::SymbolDefinition::SymbolDefinition(FunctionDefinition description) :
    variant(std::move(description)) {}

mint::SymbolDefinition::SymbolDefinition(const VariableDescription& description) :
    variant(description) {}

Symbol mint::SymbolDefinition::get_name() const {
	return std::visit(Overloaded {
	                      [](std::monostate) -> Symbol {
		                      return "<unknown>";
	                      },
	                      [](PackageDefinition definition) -> Symbol {
		                      return definition.get().get_name();
	                      },
	                      [](ClassDefinition definition) -> Symbol {
		                      return definition.get().get_name();
	                      },
	                      [](const FunctionDefinition& definition) -> Symbol {
		                      return definition.front().get().get_name();
	                      },
	                      [](VariableDefinition definition) -> Symbol {
		                      return definition.get().name;
	                      },
	                  },
	    *this);
}

TypeAnnotation mint::SymbolDefinition::get_type() const {
	return std::visit(Overloaded {
	                      [](std::monostate) -> TypeAnnotation {
		                      return {};
	                      },
	                      [](const PackageDescription& description) -> TypeAnnotation {
		                      return description.get_path();
	                      },
	                      [](const ClassDescription& description) -> TypeAnnotation {
		                      return description.get_path();
	                      },
	                      [](const FunctionDefinition&) -> TypeAnnotation {
		                      return PrimitiveTypeKind::function;
	                      },
	                      [](const VariableDescription& description) -> TypeAnnotation {
		                      return description.type;
	                      },
	                  },
	    *this);
}

mint::SymbolScope::SymbolScope(SymbolScope* owner) :
    _owner(owner) {}

const SymbolScope& SymbolScope::get_root_scope() const {
	if (is_root_scope()) {
		return *this;
	}
	if (_owner) {
		return _owner->get_root_scope();
	}
	return *this;
}

SymbolScope& SymbolScope::get_root_scope() {
	if (is_root_scope()) {
		return *this;
	}
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
		                            [](const FunctionDefinition&) -> SymbolDefinition {
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
