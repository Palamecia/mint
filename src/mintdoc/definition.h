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

#ifndef MINTDOC_DEFINITION_H
#define MINTDOC_DEFINITION_H

#include "docnode.h"

#include <cassert>
#include "mint/memory/reference.h"

#include <concepts>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <set>

struct Definition {
	enum Type : std::uint8_t {
		package_definition,
		enum_definition,
		class_definition,
		constant_definition,
		function_definition
	};

	Definition(Type type, std::string name);
	Definition(const Definition&) = delete;
	Definition(Definition&&) = delete;
	virtual ~Definition();

	Definition& operator=(const Definition&) = delete;
	Definition& operator=(Definition&&) = delete;

	Type type;
	mint::Reference::Flags flags;
	std::string name;

	[[nodiscard]] std::string context() const;
	[[nodiscard]] std::string symbol() const;

	template<std::derived_from<Definition> T>
	inline const T& as() const;
};

struct Package : public Definition {
	Package(const std::string& name);

	std::set<std::string> members;
	std::unique_ptr<DocNode> doc;
};

template<>
inline const Package& Definition::as<Package>() const {
	assert(type == package_definition);
	return static_cast<const Package&>(*this);
}

struct Enum : public Definition {
	Enum(const std::string& name);

	std::set<std::string> members;
	std::unique_ptr<DocNode> doc;
};

template<>
inline const Enum& Definition::as<Enum>() const {
	assert(type == enum_definition);
	return static_cast<const Enum&>(*this);
}

struct Class : public Definition {
	Class(const std::string& name);

	std::vector<std::string> bases;
	std::set<std::string> members;
	std::unique_ptr<DocNode> doc;
};

template<>
inline const Class& Definition::as<Class>() const {
	assert(type == class_definition);
	return static_cast<const Class&>(*this);
}

struct Constant : public Definition {
	Constant(const std::string& name);

	std::string value;
	std::unique_ptr<DocNode> doc;
};

template<>
inline const Constant& Definition::as<Constant>() const {
	assert(type == constant_definition);
	return static_cast<const Constant&>(*this);
}

struct Function : public Definition {
	struct Signature {
		std::string format;
		std::unique_ptr<DocNode> doc;
	};

	Function(const std::string& name);

	std::vector<std::shared_ptr<Signature>> signatures;
};

template<>
inline const Function& Definition::as<Function>() const {
	assert(type == function_definition);
	return static_cast<const Function&>(*this);
}

template<class R, class Visitor>
R visit(Visitor&& visitor, const Definition& definition) {
	switch (definition.type) {
	case Definition::package_definition:
		return std::invoke(std::forward<Visitor>(visitor), definition.as<Package>());
	case Definition::enum_definition:
		return std::invoke(std::forward<Visitor>(visitor), definition.as<Enum>());
	case Definition::class_definition:
		return std::invoke(std::forward<Visitor>(visitor), definition.as<Class>());
	case Definition::constant_definition:
		return std::invoke(std::forward<Visitor>(visitor), definition.as<Constant>());
	case Definition::function_definition:
		return std::invoke(std::forward<Visitor>(visitor), definition.as<Function>());
	}
	return {};
}

#endif // MINTDOC_DEFINITION_H
