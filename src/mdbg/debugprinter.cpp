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

using namespace mint;

DebugPrinter::DebugPrinter() {}

DebugPrinter::~DebugPrinter() {}

void DebugPrinter::print(Reference &reference) {
	switch (reference.data()->format) {
	case Data::FMT_NONE:
		Terminal::print(stdout, "none\n");
		break;

	case Data::FMT_NULL:
		Terminal::print(stdout, "null\n");
		break;

	case Data::FMT_NUMBER:
		Terminal::printf(stdout, "%g\n", reference.data<Number>()->value);
		break;

	case Data::FMT_BOOLEAN:
		Terminal::printf(stdout, "%s\n", reference.data<Boolean>()->value ? "true" : "false");
		break;

	case Data::FMT_OBJECT:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::OBJECT:
			if (Object *object = reference.data<Object>()) {

				std::string type = object->metadata->full_name();
				Terminal::printf(stdout, "(%s) {\n", type.c_str());

				if (mint::is_object(object)) {
					for (auto member : object->metadata->members()) {
						std::string member_str = member.first.str();
						std::string type = type_name(member.second->value);
						std::string value = reference_value(Class::MemberInfo::get(member.second, object));
						Terminal::printf(stdout, "\t%s : (%s) %s\n", member_str.c_str(), type.c_str(), value.c_str());
					}
				}
				else {
					for (auto member : object->metadata->members()) {
						std::string member_str = member.first.str();
						std::string type = type_name(member.second->value);
						std::string value = reference_value(member.second->value);
						Terminal::printf(stdout, "\t%s : (%s) %s\n", member_str.c_str(), type.c_str(), value.c_str());
					}
				}

				Terminal::printf(stdout, "}\n");
			}
			break;

		case Class::STRING:
			Terminal::printf(stdout, "\"%s\"\n", reference.data<String>()->str.c_str());
			break;

		case Class::REGEX:
			Terminal::printf(stdout, "%s\n", reference.data<Regex>()->initializer.c_str());
			break;

		case Class::ARRAY:
			{
				std::string value = array_value(reference.data<Array>());
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::HASH:
			{
				std::string value = hash_value(reference.data<Hash>());
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::ITERATOR:
			{
				std::string value = iterator_value(reference.data<Iterator>());
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::LIBRARY:
		case Class::LIBOBJECT:
			{
				std::string value = reference_value(reference);
				Terminal::printf(stdout, "%s\n", value.c_str());
			}
			break;
		}
		break;

	case Data::FMT_PACKAGE:
		{
			std::string value = reference.data<Package>()->data->full_name();
			Terminal::printf(stdout, "package: %s\n", value.c_str());
		}
		break;

	case Data::FMT_FUNCTION:
		{
			std::string value = function_value(reference.data<Function>());
			Terminal::printf(stdout, "%s\n", value.c_str());
		}
		break;
	}
}

std::string reference_value(const Reference &reference) {

	char address[2 * sizeof(void *) + 3];

	switch (reference.data()->format) {
	case Data::FMT_NONE:
		return "none";

	case Data::FMT_NULL:
		return "null";

	case Data::FMT_NUMBER:
	case Data::FMT_BOOLEAN:
		return to_string(reference);

	case Data::FMT_OBJECT:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			return "\"" + reference.data<String>()->str + "\"";

		case Class::REGEX:
			return reference.data<Regex>()->initializer;

		case Class::ARRAY:
			return array_value(reference.data<Array>());

		case Class::HASH:
			return hash_value(reference.data<Hash>());

		case Class::ITERATOR:
			return iterator_value(reference.data<Iterator>());

		case Class::LIBRARY:
			return reference.data<Library>()->plugin->get_path();

		case Class::OBJECT:
		case Class::LIBOBJECT:
			sprintf(address, "0x%p", static_cast<void *>(reference.data()));
			return address;
		}
		break;

	case Data::FMT_PACKAGE:
		return reference.data<Package>()->data->full_name();

	case Data::FMT_FUNCTION:
		return function_value(reference.data<Function>());
	}

	return "unknown";
}

std::string iterator_value(Iterator *iterator) {
	return "("
		   + mint::join(iterator->ctx, ", ",
						[](auto it) {
							return reference_value(*it);
						})
		   + ")";
}

std::string array_value(Array *array) {
	return "["
		   + mint::join(array->values, ", ",
						[](auto it) {
							return reference_value(array_get_item(it));
						})
		   + "]";
}

std::string hash_value(Hash *hash) {
	return "{"
		   + mint::join(hash->values, ", ",
						[](auto it) {
							return reference_value(hash_get_key(it)) + " : " + reference_value(hash_get_value(it));
						})
		   + "}";
}

std::string function_value(Function *function) {
	return "function: " + mint::join(function->mapping, ", ", [ast = AbstractSyntaxTree::instance()](auto it) {
			   Module *module = ast->get_module(it->second.handle->module);
			   DebugInfo *infos = ast->get_debug_info(it->second.handle->module);
			   return std::to_string(it->first) + "@" + ast->get_module_name(module) + "(line "
					  + std::to_string(infos->line_number(it->second.handle->offset)) + ")";
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
