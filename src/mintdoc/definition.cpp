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

#include "definition.h"

#include <algorithm>
#include <memory>

using namespace std;

static string::size_type find_symbol_separator(const string &name) {
	auto pos = name.rfind('.');
	if (pos != string::npos) {
		while (pos && name[pos - 1] == '.') {
			--pos;
		}
	}
	return pos;
}

Definition::Definition(Type type, const string &name) :
	type(type),
	flags(0),
	name(name) {

}

Definition::~Definition() {

}

std::string Definition::context() const {
	return name.substr(0, find_symbol_separator(name));
}

std::string Definition::symbol() const {
	return name.substr(find_symbol_separator(name) + 1);
}

Package::Package(const string &name) :
	Definition(package_definition, name) {

}

Enum::Enum(const string &name) :
	Definition(enum_definition, name) {

}

Class::Class(const string &name) :
	Definition(class_definition, name) {

}

Constant::Constant(const string &name) :
	Definition(constant_definition, name) {

}

Function::Function(const string &name) :
	Definition(function_definition, name) {

}

Function::~Function() {
	for_each(signatures.begin(), signatures.end(), default_delete<Signature>());
}
