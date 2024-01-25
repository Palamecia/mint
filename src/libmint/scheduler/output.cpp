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

#include "mint/scheduler/output.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/reference.h"
#include "mint/memory/casttool.h"
#include "mint/memory/class.h"
#include "mint/system/terminal.h"
#include "mint/system/plugin.h"

using namespace mint;
using namespace std;

static string reference_value(const Reference &reference);

static string function_value(Function *function) {

	string join;

	for (auto it = function->mapping.begin(); it != function->mapping.end(); ++it) {
		if (it != function->mapping.begin()) {
			join += ", ";
		}
		Module *module = AbstractSyntaxTree::instance()->get_module(it->second.handle->module);
		DebugInfo *infos = AbstractSyntaxTree::instance()->get_debug_info(it->second.handle->module);
		join += to_string(it->first);
		join += "@";
		join += AbstractSyntaxTree::instance()->get_module_name(module);
		join += "(line ";
		join += to_string(infos->line_number(it->second.handle->offset));
		join += ")";
	}

	return join;
}

static string array_value(Array *array) {

	string join;

	for (auto it = array->values.begin(); it != array->values.end(); ++it) {
		if (it != array->values.begin()) {
			join += ", ";
		}
		join += reference_value(*it);
	}

	return join;
}

static string hash_value(Hash *hash) {

	string join;

	for (auto it = hash->values.begin(); it != hash->values.end(); ++it) {
		if (it != hash->values.begin()) {
			join += ", ";
		}
		join += reference_value(it->first);
		join +=  ": ";
		join += reference_value(it->second);
	}

	return join;
}

string reference_value(const Reference &reference) {
	switch (reference.data()->format) {
	case Data::fmt_none:
		return "\033[2mnone\033[0m";
	case Data::fmt_null:
		return "\033[2mnull\033[0m";
	case Data::fmt_package:
		return "\033[35mpackage:\033[0m " + reference.data<Package>()->data->full_name() + "\033[0m";
	case Data::fmt_function:
		return "\033[35mfunction:\033[0m " + function_value(reference.data<Function>()) + "\033[0m";
	case Data::fmt_object:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::object:
			if (mint::is_class(reference.data<Object>())) {
				return "\033[35mclass:\033[0m " + reference.data<Object>()->metadata->full_name() + "\033[0m";
			}
			return "\033[35mobject:\033[0m " + reference.data<Object>()->metadata->full_name() + " \033[2m(" + to_string(reference.data()) + ")\033[0m";
		case Class::string:
			return "\033[32m'" + to_string(reference) + "'\033[0m";
		case Class::regex:
			return "\033[31m" + to_string(reference) + "\033[0m";
		case Class::array:
			return "[ " + array_value(reference.data<Array>()) + " ]";
		case Class::hash:
			return "{ " + hash_value(reference.data<Hash>()) + " }";
		case Class::iterator:
			if (optional<WeakReference> &&item = iterator_get(reference.data<Iterator>())) {
				return "\033[35miterator:\033[0m " + reference_value(item.value()) + "\033[0m";
			}
			return "\033[35miterator:\033[2m empty\033[0m";
		case Class::library:
			return "\033[35mlibrary:\033[0m " + reference.data<Library>()->plugin->get_path() + "\033[0m";
		case Class::libobject:
			return "\033[35mlibobject:\033[0m " + to_string(reference.data()) + "\033[0m";
		}
		break;
	default:
		return "\033[33m" + to_string(reference) + "\033[0m";
	}
	return {};
}

Output::Output() {

}

Output::~Output() {
	Terminal::print(stdout, "\n");
}

Output &Output::instance() {

	static Output g_instance;

	return g_instance;
}

void Output::print(Reference &reference) {
	Terminal::printf(stdout, "%s\n", reference_value(reference).c_str());
}

bool Output::global() const {
	return true;
}
