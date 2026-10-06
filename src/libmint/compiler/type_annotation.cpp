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

#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/program/symbol.h"
#include "mint/system/error.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

mint::TypeAnnotation::TypeAnnotation() = default;

mint::TypeAnnotation::TypeAnnotation(PrimitiveTypeKind kind) :
    definition(PrimitiveType(kind)) {}

mint::TypeAnnotation::TypeAnnotation(BuiltinTypeKind kind) :
    definition(BuiltinType(kind)) {}

mint::TypeAnnotation::TypeAnnotation(SymbolPath path) :
    definition(UserType(std::move(path))) {}

mint::TypeAnnotation::TypeAnnotation(std::vector<TypeAnnotation> elements) :
    definition(UnionType(std::move(elements))) {}

void mint::TypeAnnotation::add_type(TypeAnnotation&& other) {
	std::visit(mint::Overloaded {
	               [this, &other](PrimitiveType& current_def) {
		               auto new_union = UnionType();
		               new_union.elements.emplace_back(current_def.kind);
		               new_union.elements.emplace_back(std::move(other));
		               definition = std::move(new_union);
	               },
	               [this, &other](BuiltinType& current_def) {
		               auto new_union = UnionType();
		               new_union.elements.emplace_back(current_def.kind);
		               new_union.elements.emplace_back(std::move(other));
		               definition = std::move(new_union);
	               },
	               [this, &other](UserType& current_def) {
		               auto new_union = UnionType();
		               new_union.elements.emplace_back(current_def.path);
		               new_union.elements.emplace_back(std::move(other));
		               definition = std::move(new_union);
	               },
	               [&other](UnionType& current_union) {
		               current_union.elements.emplace_back(std::move(other));
	               },
	               [](std::monostate&) {
		               mint::error("cannot add type to an empty type annotation");
	               },
	           },
	    definition);
}

void mint::TypeAnnotation::add_type(const TypeAnnotation& other) {
	auto copy = other;
	add_type(std::move(copy));
}

void mint::TypeAnnotation::add_type(const std::string& path) {
	add_type(make_type_annotation(path));
}

mint::TypeAnnotation mint::make_type_annotation(const std::string& type_name) {

	static const auto primitive_types = std::unordered_map<std::string, PrimitiveTypeKind> {
	    {"boolean", PrimitiveTypeKind::boolean},
	    {"coroutine", PrimitiveTypeKind::coroutine},
	    {"function", PrimitiveTypeKind::function},
	    {"none", PrimitiveTypeKind::none},
	    {"null", PrimitiveTypeKind::null},
	    {"number", PrimitiveTypeKind::number},
	    {"package", PrimitiveTypeKind::package},
	};

	if (const auto it = primitive_types.find(type_name); it != primitive_types.end()) {
		return it->second;
	}

	static const auto builtin_types = std::unordered_map<std::string, BuiltinTypeKind> {
	    {"array", BuiltinTypeKind::array},
	    {"hash", BuiltinTypeKind::hash},
	    {"iterator", BuiltinTypeKind::iterator},
	    {"async_iterator", BuiltinTypeKind::async_iterator},
	    {"libobject", BuiltinTypeKind::libobject},
	    {"library", BuiltinTypeKind::library},
	    {"object", BuiltinTypeKind::object},
	    {"regex", BuiltinTypeKind::regex},
	    {"string", BuiltinTypeKind::string},
	};

	if (const auto it = builtin_types.find(type_name); it != builtin_types.end()) {
		return it->second;
	}

	return SymbolPath(type_name);
}

/*bool mint::match_type(const Program& program, const TypeAnnotation& target, const TypeAnnotation& source) {
	return std::visit(mint::Overloaded {
	                      [](std::monostate) -> bool {
		                      return true;
	                      },
	                      [&](const PrimitiveType& primitive) -> bool {
		                      return primitive.format == source;
	                      },
	                      [&](const BuiltinType& builtin) -> bool {
		                      return builtin.metatype == source;
	                      },
	                      [&](const UserType& user_type) -> bool {
		                      return program.global_data().locate(user_type.path).is_derived_from(source);
	                      },
	                      [&](const UnionType& union_type) -> bool {
		                      return std::ranges::any_of(union_type.elements, [&](const auto& source) {
			                      return match_type(program, target, source);
		                      });
	                      },
	                  },
	    target.definition);
}*/
