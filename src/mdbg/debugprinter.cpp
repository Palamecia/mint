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

#include "debugprinter.h"

#include <mint/memory/builtin/iterator.h>
#include <mint/memory/builtin/library.h>
#include <mint/memory/builtin/string.h>
#include <mint/memory/builtin/regex.h>
#include <mint/memory/globaldata.h>
#include <mint/memory/memorytool.h>
#include <mint/memory/casttool.h>
#include <mint/ast/abstractsyntaxtree.h>
#include <mint/system/string.h>
#include <mint/system/terminal.h>
#include <mint/system/plugin.h>

#include <cstdio>
#include <cstdarg>

using namespace std;
using namespace mint;

DebugPrinter::DebugPrinter() {

}

DebugPrinter::~DebugPrinter() {

}

void DebugPrinter::print(Reference &reference) {
	switch (reference.data()->format) {
	case Data::fmt_none:
		Terminal::print(stdout, "none\n");
		break;

	case Data::fmt_null:
		Terminal::print(stdout, "null\n");
		break;

	case Data::fmt_number:
		Terminal::printf(stdout, "%g\n", reference.data<Number>()->value);
		break;

	case Data::fmt_boolean:
		Terminal::printf(stdout, "%s\n", reference.data<Boolean>()->value ? "true" : "false");
		break;

	case Data::fmt_object:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::object:
			if (Object *object = reference.data<Object>()) {
				
				string type = object->metadata->full_name();
				Terminal::printf(stdout, "(%s) {\n", type.c_str());

				if (mint::is_object(object)) {
					for (auto member : object->metadata->members()) {
						string member_str = member.first.str();
						string type = type_name(member.second->value);
						string value = reference_value(Class::MemberInfo::get(member.second, object));
						Terminal::printf(stdout, "\t%s : (%s) %s\n", member_str.c_str(), type.c_str(), value.c_str());
					}
				}
				else {
					for (auto member : object->metadata->members()) {
						string member_str = member.first.str();
						string type = type_name(member.second->value);
						string value = reference_value(member.second->value);
						Terminal::printf(stdout, "\t%s : (%s) %s\n", member_str.c_str(), type.c_str(), value.c_str());
					}
				}

				Terminal::printf(stdout, "}\n");
			}
			break;

		case Class::string:
			Terminal::printf(stdout, "\"%s\"\n", reference.data<String>()->str.c_str());
			break;

		case Class::regex:
			Terminal::printf(stdout, "%s\n", reference.data<Regex>()->initializer.c_str());
			break;

		case Class::array:
			{
				string value = array_value(reference.data<Array>());
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::hash:
			{
				string value = hash_value(reference.data<Hash>());
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::iterator:
			{
				string value = iterator_value(reference.data<Iterator>());
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::library:
		case Class::libobject:
			{
				string value = reference_value(reference);
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;
		}
		break;

	case Data::fmt_package:
		{
		string value = reference.data<Package>()->data->full_name();
			Terminal::printf(stdout, "package: %s\n", value.c_str());
		}
		break;

	case Data::fmt_function:
		{
			string value = function_value(reference.data<Function>());
			Terminal::printf(stdout, "%s\n", value.c_str());
		}
		break;
	}
}

string reference_value(const Reference &reference) {

	char address[2 * sizeof(void *) + 3];

	switch (reference.data()->format) {
	case Data::fmt_none:
		return "none";

	case Data::fmt_null:
		return "null";

	case Data::fmt_number:
	case Data::fmt_boolean:
		return to_string(reference);

	case Data::fmt_object:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::string:
			return "\"" + reference.data<String>()->str + "\"";

		case Class::regex:
			return reference.data<Regex>()->initializer;

		case Class::array:
			return array_value(reference.data<Array>());

		case Class::hash:
			return hash_value(reference.data<Hash>());

		case Class::iterator:
			return iterator_value(reference.data<Iterator>());

		case Class::library:
			return reference.data<Library>()->plugin->get_path();

		case Class::object:
		case Class::libobject:
			sprintf(address, "0x%p", static_cast<void *>(reference.data()));
			return address;
		}
		break;

	case Data::fmt_package:
		return reference.data<Package>()->data->full_name();

	case Data::fmt_function:
		return function_value(reference.data<Function>());
	}

	return "unknown";
}

string iterator_value(Iterator *iterator) {
	return "(" + mint::join(iterator->ctx, ", ", [](auto it) {
			   return reference_value(*it);
		   }) + ")";
}

string array_value(Array *array) {
	return "[" + mint::join(array->values, ", ", [](auto it) {
			   return reference_value(array_get_item(it));
		   }) + "]";
}

string hash_value(Hash *hash) {
	return "{" + mint::join(hash->values, ", ", [](auto it) {
			   return reference_value(hash_get_key(it)) + " : " + reference_value(hash_get_value(it));
		   }) + "}";
}

string function_value(Function *function) {
	return "function: " + mint::join(function->mapping, ", ", [ast = AbstractSyntaxTree::instance()](auto it) {
			   Module *module = ast->get_module(it->second.handle->module);
			   DebugInfo *infos = ast->get_debug_info(it->second.handle->module);
			   return to_string(it->first)
					  + "@" + ast->get_module_name(module)
					  + "(line " + to_string(infos->line_number(it->second.handle->offset)) + ")";
		   });
}

void print_debug_trace(const char *format, ...) {
	va_list va_args;
	mint::print(stdout, "\t");
	va_start(va_args, format);
	mint::vprintf(stdout, format, va_args);
	va_end(va_args);
	mint::print(stdout, "\n");
}

