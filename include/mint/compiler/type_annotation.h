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

#ifndef MINT_COMPILER_TYPE_ANNOTATION_H
#define MINT_COMPILER_TYPE_ANNOTATION_H

#include "mint/config.h"
#include "mint/program/symbol.h"
#include <string>
#include <variant>
#include <vector>

namespace mint {

class Program;
struct TypeAnnotation;

enum class PrimitiveTypeKind {
	boolean,
	coroutine,
	function,
	none,
	null,
	number,
	package,
};

struct PrimitiveType {
	PrimitiveTypeKind kind;
};

enum class BuiltinTypeKind {
	array,
	hash,
	iterator,
	async_iterator,
	libobject,
	library,
	object,
	regex,
	string,
};

struct BuiltinType {
	BuiltinTypeKind kind;
};

struct UserType {
	SymbolPath path;
};

struct UnionType {
	std::vector<TypeAnnotation> elements;
};

struct MINT_EXPORT TypeAnnotation {
	TypeAnnotation();
	explicit(false) TypeAnnotation(PrimitiveTypeKind kind);
	explicit(false) TypeAnnotation(BuiltinTypeKind kind);
	explicit(false) TypeAnnotation(SymbolPath path);
	explicit(false) TypeAnnotation(std::vector<TypeAnnotation> elements);

	std::variant<PrimitiveType, BuiltinType, UserType, UnionType, std::monostate> definition;

	[[nodiscard]] bool is_primitive() const {
		return std::holds_alternative<PrimitiveType>(definition);
	}

	[[nodiscard]] bool is_built_in() const {
		return std::holds_alternative<BuiltinType>(definition);
	}

	[[nodiscard]] bool is_user() const {
		return std::holds_alternative<UserType>(definition);
	}

	[[nodiscard]] bool is_union() const {
		return std::holds_alternative<UnionType>(definition);
	}

	void add_type(TypeAnnotation&& other);
	void add_type(const TypeAnnotation& other);
	void add_type(const std::string& path);
};

MINT_EXPORT TypeAnnotation make_type_annotation(const std::string& type_name);
MINT_EXPORT bool match_type(const Program& program, const TypeAnnotation& target, const TypeAnnotation& source);

}

#endif // MINT_COMPILER_TYPE_ANNOTATION_H
