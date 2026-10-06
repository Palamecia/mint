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

#include "completer.h"

#include "mint/compiler/descriptions.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/program/symbol.h"
#include "mint/memory/global_data.h"
#include "mint/debug/debug_tools.h"
#include "mint/memory/object.h"
#include "mint/system/filesystem.h"
#include "mint/system/terminal.h"
#include "mint/system/utf8.h"
#include "mint/program/program.h"
#include "mint/program/cursor.h"

#include <compare>
#include <filesystem>
#include <format>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace mint;

namespace {

SymbolPath make_path(const std::vector<std::string>& context) {
	return context | std::views::transform([](const auto& symbol) {
		return Symbol(symbol);
	}) | std::ranges::to<SymbolPath>();
}

std::string to_return_type_hint(const TypeAnnotation& type) {
	if (type.is_undefined()) {
		return {};
	}
	return std::format(" -> {}", type.to_string());
}

std::string to_type_hint(const TypeAnnotation& type) {
	if (type.is_undefined()) {
		return {};
	}
	return std::format(": {}", type.to_string());
}

std::string to_hint(const VariableDescription& description) {
	return to_type_hint(description.type);
}

std::string to_hint(const FunctionDescription& description) {
	return std::format("def ({}){}",
	    description.parameters() | std::views::transform([](const ParameterDescription& parameter) {
		    return std::format("{}{}", parameter.name.str(), to_type_hint(parameter.type));
	    }) | std::views::join_with(std::string(", ")),
	    to_return_type_hint(description.get_return_type()));
}

std::string to_hint(const MemberDescriptionEntry& entry) {
	return std::visit(Overloaded {
	                      [](const VariableDescription& description) {
		                      return to_hint(description);
	                      },
	                      [](const FunctionDescription& description) {
		                      return to_hint(description);
	                      },
	                  },
	    entry.description);
}

}

Completer::Completer(std::string_view::size_type offset, const Cursor& cursor) :
    _offset(offset),
    _cursor(cursor) {}

std::vector<Completion> Completer::completions() const {
	return _completions;
}

bool Completer::on_module_path_token(const std::vector<std::string>& context, const std::string& token,
    std::string::size_type offset) {
	if (_offset > offset + token.size()) {
		return true;
	}
	if (_offset >= offset) {
		std::string token_path;
		for (const std::string& module : context) {
			token_path.append(module);
		}
		token_path.append(token);
		for (const std::filesystem::path& path : FileSystem::instance().library_path()) {
			const std::filesystem::path root_path = std::filesystem::absolute(path);
			find_module_recursive_helper(root_path, root_path, token_path);
		}
	}
	return false;
}

bool Completer::on_symbol_token(const std::vector<std::string>& context, const std::string& token,
    std::string::size_type offset) {
	if (_offset > offset + token.size()) {
		return true;
	}
	if (_offset >= offset) {

		if (context.empty()) {
			for (const auto& [symbol, _] : _cursor.get().symbols()) {
				if (token_match(symbol.str(), token)) {
					_completions.push_back({
					    .offset = offset,
					    .token = symbol.str(),
					    .hint = {},
					});
				}
			}
		}

		find_context_symbols_helper(find_defined_symbol(make_path(context)), token, offset);
	}
	return false;
}

bool Completer::on_symbol_token(const std::vector<std::string>& context, std::string::size_type offset) {
	if (_offset > offset) {
		return true;
	}
	if (_offset >= offset) {
		find_context_symbols_helper(find_defined_symbol(make_path(context)), {}, offset);
	}
	return false;
}

void Completer::find_module_recursive_helper(const std::filesystem::path& root_path,
    const std::filesystem::path& directory_path, const std::string& token_path) {
	for (const auto& entry : std::filesystem::directory_iterator {directory_path}) {
		if (entry.is_directory()) {
			find_module_recursive_helper(root_path, entry.path(), token_path);
		}
		else if (is_module_file(entry.path())) {
			const std::string module_path = FileSystem::to_module_path(root_path, entry.path());
			if (token_match(module_path, token_path)) {
				_completions.push_back({
				    .offset = _offset - token_path.size(),
				    .token = module_path,
				});
			}
		}
	}
}

void Completer::find_context_symbols_helper(const SymbolDefinition& definition, const std::string& token,
    std::string::size_type offset) {
	std::visit(Overloaded {
	               [&](std::monostate) {},
	               [&](const PackageDescription& definition) {
		               for (const auto& variable : definition.variables()) {
			               if (token_match(variable.name.str(), token)) {
				               _completions.push_back({
				                   .offset = offset,
				                   .token = variable.name.str(),
				                   .hint = to_hint(variable),
				               });
			               }
		               }
	               },
	               [&](const ClassDescription& definition) {
		               for (const auto& member : definition.all_members()) {
			               if (token_match(member.get_name().str(), token)) {
				               _completions.push_back({
				                   .offset = offset,
				                   .token = member.get_name().str(),
				                   .hint = to_hint(member),
				               });
			               }
		               }
	               },
	               [&](const FunctionDefinition& /*definition*/) {},
	               [&](const VariableDescription& definition) {
		               if (const auto* type = std::get_if<UserType>(&definition.type.definition)) {
			               find_context_symbols_helper(_cursor.get().program().global_data_description().locate(
			                                               type->path),
			                   token, offset);
		               }
	               },
	           },
	    definition);
}

bool Completer::token_match(const std::string& symbol, const std::string& pattern) {
	return symbol.size() >= pattern.size()
	       && utf8_compare_substring_case_insensitive(pattern, symbol, utf8_code_point_count(pattern))
	              == std::strong_ordering::equal;
}

SymbolDefinition Completer::find_defined_symbol(const SymbolPath& symbols) const {
	return _cursor.get().program().global_data_description().locate(symbols);
}
