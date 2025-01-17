/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#ifndef DEFINITION_H
#define DEFINITION_H

#include <mint/memory/reference.h>
#include <cstdint>
#include <string>
#include <vector>
#include <set>

struct Definition {
	enum Type : std::uint8_t {
		PACKAGE_DEFINITION,
		ENUM_DEFINITION,
		CLASS_DEFINITION,
		CONSTANT_DEFINITION,
		FUNCTION_DEFINITION
	};

	Definition(Type type, std::string name);
	Definition(const Definition &) = delete;
	Definition(Definition &&) = delete;
	virtual ~Definition();

	Definition &operator=(const Definition &) = delete;
	Definition &operator=(Definition &&) = delete;

	Type type;
	mint::Reference::Flags flags;
	std::string name;

	[[nodiscard]] std::string context() const;
	[[nodiscard]] std::string symbol() const;
};

struct Package : public Definition {
	Package(const std::string &name);

	std::set<std::string> members;
	std::string doc;
};

struct Enum : public Definition {
	Enum(const std::string &name);

	std::set<std::string> members;
	std::string doc;
};

struct Class : public Definition {
	Class(const std::string &name);

	std::vector<std::string> bases;
	std::set<std::string> members;
	std::string doc;
};

struct Constant : public Definition {
	Constant(const std::string &name);

	std::string value;
	std::string doc;
};

struct Function : public Definition {
	struct Signature {
		std::string format;
		std::string doc;
	};

	Function(const std::string &name);
	Function(const Function &) = delete;
	Function(Function &&) = delete;
	~Function();

	Function &operator=(const Function &) = delete;
	Function &operator=(Function &&) = delete;

	std::vector<Signature *> signatures;
};

#endif // DEFINITION_H
