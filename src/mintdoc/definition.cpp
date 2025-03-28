/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#include "definition.h"

#include <algorithm>
#include <memory>

namespace {

std::string::size_type find_symbol_separator(const std::string &name) {
	if (auto pos = name.rfind('.'); pos != std::string::npos) {
		while (pos && name[pos - 1] == '.') {
			--pos;
		}
		if (pos) {
			return pos;
		}
	}
	return std::string::npos;
}

}

Definition::Definition(Type type, std::string name) :
	type(type),
	flags(0),
	name(std::move(name)) {}

Definition::~Definition() {}

std::string Definition::context() const {
	return name.substr(0, find_symbol_separator(name));
}

std::string Definition::symbol() const {
	return name.substr(find_symbol_separator(name) + 1);
}

Package::Package(const std::string &name) :
	Definition(PACKAGE_DEFINITION, name) {}

Enum::Enum(const std::string &name) :
	Definition(ENUM_DEFINITION, name) {}

Class::Class(const std::string &name) :
	Definition(CLASS_DEFINITION, name) {}

Constant::Constant(const std::string &name) :
	Definition(CONSTANT_DEFINITION, name) {}

Function::Function(const std::string &name) :
	Definition(FUNCTION_DEFINITION, name) {}

Function::~Function() {
	std::for_each(signatures.begin(), signatures.end(), std::default_delete<Signature>());
}
