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
#include <string>
#include <vector>
#include <set>

struct Definition {
	enum Type {
		package_definition,
		enum_definition,
		class_definition,
		constant_definition,
		function_definition
	};

	Definition(Type type, const std::string &name);
	virtual ~Definition();

	Type type;
	mint::Reference::Flags flags;
	std::string name;

	std::string context() const;
	std::string symbol() const;
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
	~Function();

	std::vector<Signature *> signatures;
};

#endif // DEFINITION_H
