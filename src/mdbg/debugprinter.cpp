#include "debugprinter.h"
#include "highlighter.h"

#include <memory/builtin/library.h>
#include <memory/builtin/string.h>
#include <memory/builtin/regex.h>
#include <memory/globaldata.h>
#include <memory/memorytool.h>
#include <memory/casttool.h>
#include <ast/abstractsyntaxtree.h>
#include <system/terminal.h>
#include <system/plugin.h>

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
		term_print(stdout, "none\n");
		break;

	case Data::fmt_null:
		term_print(stdout, "null\n");
		break;

	case Data::fmt_number:
		term_printf(stdout, "%g\n", reference.data<Number>()->value);
		break;

	case Data::fmt_boolean:
		term_printf(stdout, "%s\n", reference.data<Boolean>()->value ? "true" : "false");
		break;

	case Data::fmt_object:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::object:
			if (Object *object = reference.data<Object>()) {

				string type = object->metadata->name();
				term_printf(stdout, "(%s) {\n", type.c_str());

				if (mint::is_object(object)) {
					for (auto member : object->metadata->members()) {
						string member_str = member.first.str();
						string type = type_name(member.second->value);
						string value = reference_value(object->data[member.second->offset]);
						term_printf(stdout, "\t%s : (%s) %s\n", member_str.c_str(), type.c_str(), value.c_str());
					}
				}
				else {
					for (auto member : object->metadata->members()) {
						string member_str = member.first.str();
						string type = type_name(member.second->value);
						string value = reference_value(member.second->value);
						term_printf(stdout, "\t%s : (%s) %s\n", member_str.c_str(), type.c_str(), value.c_str());
					}
				}

				term_printf(stdout, "}\n");
			}
			break;

		case Class::string:
			term_printf(stdout, "\"%s\"\n", reference.data<String>()->str.c_str());
			break;

		case Class::regex:
			term_printf(stdout, "%s\n", reference.data<Regex>()->initializer.c_str());
			break;

		case Class::array:
			{
				string value = array_value(reference.data<Array>());
				term_printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::hash:
			{
				string value = hash_value(reference.data<Hash>());
				term_printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::iterator:
			{
				string value = iterator_value(reference.data<Iterator>());
				term_printf(stdout, "%s\n", value.c_str());
			}
			break;

		case Class::library:
		case Class::libobject:
			{
				string value = reference_value(reference);
				term_printf(stdout, "%s\n", value.c_str());
			}
			break;
		}
		break;

	case Data::fmt_package:
		{
			string value = reference.data<Package>()->data->fullName();
			term_printf(stdout, "package: %s\n", value.c_str());
		}
		break;

	case Data::fmt_function:
		{
			string value = function_value(reference.data<Function>());
			term_printf(stdout, "%s\n", value.c_str());
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
			return reference.data<Library>()->plugin->getPath();

		case Class::object:
		case Class::libobject:
			sprintf(address, "0x%p", static_cast<void *>(reference.data()));
			return address;
		}
		break;

	case Data::fmt_package:
		return reference.data<Package>()->data->fullName();

	case Data::fmt_function:
		return function_value(reference.data<Function>());
	}

	return "unknown";
}

string iterator_value(Iterator *iterator) {

	string join;

	for (auto it = iterator->ctx.begin(); it != iterator->ctx.end(); ++it) {
		if (it != iterator->ctx.begin()) {
			join += ", ";
		}
		join += reference_value(*it);
	}

	return "(" + join + ")";
}

string array_value(Array *array) {

	string join;

	for (auto it = array->values.begin(); it != array->values.end(); ++it) {
		if (it != array->values.begin()) {
			join += ", ";
		}
		join += reference_value(array_get_item(it));
	}

	return "[" + join + "]";
}

string hash_value(Hash *hash) {

	string join;

	for (auto it = hash->values.begin(); it != hash->values.end(); ++it) {
		if (it != hash->values.begin()) {
			join += ", ";
		}
		join += reference_value(hash_get_key(it));
		join += " : ";
		join += reference_value(hash_get_value(it));
	}

	return "{" + join + "}";
}

string function_value(Function *function) {

	string join;

	for (auto it = function->mapping.begin(); it != function->mapping.end(); ++it) {
		if (it != function->mapping.begin()) {
			join += ", ";
		}
		Module *module = AbstractSyntaxTree::instance()->getModule(it->second.handle->module);
		DebugInfos *infos = AbstractSyntaxTree::instance()->getDebugInfos(it->second.handle->module);
		join += to_string(it->first);
		join += "@";
		join += AbstractSyntaxTree::instance()->getModuleName(module);
		join += "(line ";
		join += to_string(infos->lineNumber(it->second.handle->offset));
		join += ")";
	}

	return "function: " + join;
}

void print_debug_trace(const char *format, ...) {

	va_list va_args;

	term_print(stdout, "\t");
	va_start(va_args, format);
	term_vprintf(stdout, format, va_args);
	va_end(va_args);
	term_print(stdout, "\n");
}

