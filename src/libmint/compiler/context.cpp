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

#include "context.h"
#include "mint/compiler/descriptions.h"
#include "mint/compiler/parser_data.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/config.h"
#include "mint/program/symbol.h"
#include "mint/compiler/build_context.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <variant>

using namespace mint;

mint::SymbolEntry::SymbolEntry(const PackageDescription& description) :
    variant(description) {}

mint::SymbolEntry::SymbolEntry(const ClassDescription& description) :
    variant(description) {}

mint::SymbolEntry::SymbolEntry(FunctionDefinition description) :
    variant(description) {}

mint::SymbolEntry::SymbolEntry(const VariableDescription& description) :
    variant(description) {}

mint::SymbolEntry::SymbolEntry(const AliasDescription& description) :
    variant(description) {}

Symbol mint::SymbolEntry::get_name() const {
	return std::visit(Overloaded {
	                      [](const PackageDescription& description) -> Symbol {
		                      return description.get_name();
	                      },
	                      [](const ClassDescription& description) -> Symbol {
		                      return description.get_name();
	                      },
	                      [](const FunctionDefinition& definition) -> Symbol {
		                      return definition.front().get().get_name();
	                      },
	                      [](const VariableDescription& description) -> Symbol {
		                      return description.name;
	                      },
	                      [](const AliasDescription& description) -> Symbol {
		                      return description.name;
	                      },
	                  },
	    *this);
}

ParserData mint::SymbolEntry::to_parser_data() const {
	return std::visit(Overloaded {
	                      [](const PackageDescription& description) -> ParserData {
		                      return description;
	                      },
	                      [](const ClassDescription& description) -> ParserData {
		                      return description;
	                      },
	                      [](const FunctionDefinition& definition) -> ParserData {
		                      return definition;
	                      },
	                      [](const VariableDescription& description) -> ParserData {
		                      return description;
	                      },
	                      [](const AliasDescription& description) -> ParserData {
		                      return description;
	                      },
	                  },
	    *this);
}

VariableDefinition mint::create_variable_definition(FunctionContext& def, const VariableDescription& description) {
	if (def.description) {
		return def.description->create_variable(description);
	}
	return *def.capture_parameters.emplace_back(std::make_unique<VariableDescription>(description));
}

std::size_t mint::find_fast_symbol_index(const FunctionContext& def, const Symbol& symbol) {
	if (const auto it = std::ranges::find(def.local_symbols, symbol, &SymbolEntry::get_name);
	    it != def.local_symbols.end()) {
		return std::distance(def.local_symbols.begin(), it);
	}
	return invalid_index;
}

std::size_t mint::create_fast_symbol_index(FunctionContext& def, SymbolDefinition definition) {
	return std::visit(Overloaded {
	                      [](std::monostate) -> std::size_t {
		                      std::unreachable();
	                      },
	                      [&](PackageDefinition definition) -> std::size_t {
		                      return mint::create_fast_package_index(def, definition);
	                      },
	                      [&](ClassDefinition definition) -> std::size_t {
		                      return mint::create_fast_class_index(def, definition);
	                      },
	                      [&](const FunctionDefinition& definition) -> std::size_t {
		                      return mint::create_fast_function_index(def, definition);
	                      },
	                      [&](VariableDefinition definition) -> std::size_t {
		                      return mint::create_fast_variable_index(def, definition);
	                      },
	                      [&](const AliasDescription& description) -> std::size_t {
		                      return create_fast_symbol_index(def, description.target_definition());
	                      },
	                  },
	    definition);
}

std::size_t mint::create_fast_package_index(FunctionContext& def, PackageDefinition definition) {
	const auto index = def.local_symbols.size();
	def.local_symbols.emplace_back(definition);
	return index;
}

std::size_t mint::create_fast_class_index(FunctionContext& def, ClassDefinition definition) {
	const auto index = def.local_symbols.size();
	def.local_symbols.emplace_back(definition);
	return index;
}

std::size_t mint::create_fast_function_index(FunctionContext& def, const FunctionDefinition& definition) {
	const auto index = def.local_symbols.size();
	def.local_symbols.emplace_back(definition);
	return index;
}

std::size_t mint::create_fast_variable_index(FunctionContext& def, VariableDefinition definition) {
	const auto index = def.local_symbols.size();
	def.local_symbols.emplace_back(definition);
	return index;
}

std::size_t mint::fast_symbol_index(FunctionContext& def, const VariableDescription& description) {
	if (const auto it = std::ranges::find(def.local_symbols, description.name, &SymbolEntry::get_name);
	    it != def.local_symbols.end()) {
		return std::distance(def.local_symbols.begin(), it);
	}
	return create_fast_variable_index(def, create_variable_definition(def, description));
}
