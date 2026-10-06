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
#include "mint/compiler/symbol_scope.h"
#include "mint/program/symbol.h"
#include "mint/compiler/build_context.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <variant>

using namespace mint;

VariableDefinition mint::create_variable_definition(FunctionContext& def, const VariableDescription& description) {
	if (def.description) {
		return def.description->create_variable(description);
	}
	return def.capture_parameters.emplace_back(description);
}

std::size_t mint::find_fast_symbol_index(const FunctionContext& def, const Symbol& symbol) {
	if (const auto it = std::ranges::find(def.local_variables, symbol, &VariableDescription::name);
	    it != def.local_variables.end()) {
		return std::distance(def.local_variables.begin(), it);
	}
	return invalid_index;
}

std::size_t mint::create_fast_symbol_index(FunctionContext& def, SymbolDefinition definition) {
	if (const auto* variable = std::get_if<VariableDefinition>(&definition)) {
		return mint::create_fast_variable_index(def, *variable);
	}
	return mint::create_fast_variable_index(def,
	    mint::create_variable_definition(def, {
	                                              .name = definition.get_name(),
	                                              .type = definition.get_type(),
	                                          }));
}

std::size_t mint::create_fast_variable_index(FunctionContext& def, VariableDefinition definition) {
	const auto index = def.local_variables.size();
	def.local_variables.emplace_back(definition);
	return index;
}

std::size_t mint::fast_symbol_index(FunctionContext& def, const VariableDescription& description) {
	if (const auto it = std::ranges::find(def.local_variables, description.name, &VariableDescription::name);
	    it != def.local_variables.end()) {
		return std::distance(def.local_variables.begin(), it);
	}
	return create_fast_variable_index(def, create_variable_definition(def, description));
}
