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
#include "block.h"
#include "mint/compiler/descriptions.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/config.h"
#include "mint/program/symbol.h"
#include "mint/compiler/build_context.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <utility>
#include <variant>

using namespace mint;

mint::SymbolEntry::SymbolEntry(Symbol symbol) :
    variant(std::move(symbol)) {}

mint::SymbolEntry::SymbolEntry(const VariableDescription& description) :
    variant(description) {}

mint::SymbolEntry::SymbolEntry(FunctionDefinition description) :
    variant(std::move(description)) {}

mint::SymbolEntry::SymbolEntry(const PackageDescription& description) :
    variant(description) {}

mint::SymbolEntry::SymbolEntry(const ClassDescription& description) :
    variant(description) {}

Symbol mint::SymbolEntry::get_name() const {
	return std::visit(Overloaded {
	                      [](const Symbol& symbol) -> Symbol {
		                      return symbol;
	                      },
	                      [](const VariableDescription& description) -> Symbol {
		                      return description.name;
	                      },
	                      [](const FunctionDefinition& definition) -> Symbol {
		                      return definition.front().get().get_name();
	                      },
	                      [](const PackageDescription& description) -> Symbol {
		                      return description.get_name();
	                      },
	                      [](const ClassDescription& description) -> Symbol {
		                      return description.get_name();
	                      },
	                  },
	    *this);
}

SymbolDefinition mint::SymbolEntry::to_definition() const {
	return std::visit(Overloaded {
	                      [](const Symbol&) -> SymbolDefinition {
		                      return {};
	                      },
	                      [](const VariableDescription& description) -> SymbolDefinition {
		                      return description;
	                      },
	                      [](const FunctionDefinition& definition) -> SymbolDefinition {
		                      return definition;
	                      },
	                      [](const PackageDescription& description) -> SymbolDefinition {
		                      return description;
	                      },
	                      [](const ClassDescription& description) -> SymbolDefinition {
		                      return description;
	                      },
	                  },
	    *this);
}

std::unique_ptr<IfBaseBlock> mint::take_resolved_if_block(Context& context) {
	assert(context.resolved_block && dynamic_cast<IfBaseBlock*>(context.resolved_block.get()) != nullptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
	return std::unique_ptr<IfBaseBlock>(static_cast<IfBaseBlock*>(context.resolved_block.release()));
}

std::unique_ptr<TryBlock> mint::take_resolved_try_block(Context& context) {
	assert(context.resolved_block && dynamic_cast<TryBlock*>(context.resolved_block.get()) != nullptr);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
	return std::unique_ptr<TryBlock>(static_cast<TryBlock*>(context.resolved_block.release()));
}

std::size_t mint::find_fast_symbol_index(const FunctionContext& def, const Symbol& symbol) {
	// Reverse to search the most recent symbols first, as they may shadow earlier ones.
	const auto reversed = std::views::reverse(def.local_symbols);
	if (const auto it = std::ranges::find(reversed, symbol, &SymbolEntry::get_name); it != reversed.end()) {
		return std::distance(def.local_symbols.begin(), it.base()) - 1;
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
	                  },
	    definition);
}

std::size_t mint::create_fast_symbol_index(FunctionContext& def, Symbol symbol) {
	const auto index = def.local_symbols.size();
	def.local_symbols.emplace_back(std::move(symbol));
	return index;
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
