#include "debugprinter.h"
#include "highlighter.h"

#include <memory/builtin/library.h>
#include <memory/builtin/string.h>
#include <memory/builtin/regex.h>
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

bool DebugPrinter::print(DataType type, void *value) {
	switch (type) {
	case none:
		printf("none\n");
		break;

	case null:
		printf("null\n");
		break;

	case regex:
		printf("%s\n", static_cast<Regex *>(value)->initializer.c_str());
		break;

	case array:
		printf("%s\n", array_value(static_cast<Array *>(value)).c_str());
		break;

	case hash:
		printf("%s\n", hash_value(static_cast<Hash *>(value)).c_str());
		break;

	case iterator:
		printf("%s\n", iterator_value(static_cast<Iterator *>(value)).c_str());
		break;

	case object:
		if (Object *object = static_cast<Object *>(value)) {

			printf("(%s) {\n", object->metadata->name().c_str());

			if (mint::is_object(object)) {
				for (auto member : object->metadata->members()) {
					printf("\t%s : (%s) %s\n",
						   member.first.str().c_str(),
						   type_name(SharedReference::weak(member.second->value)).c_str(),
						   reference_value(SharedReference::weak(object->data[member.second->offset])).c_str());
				}
			}
			else {
				for (auto member : object->metadata->members()) {
					printf("\t%s : (%s) %s\n",
						   member.first.str().c_str(),
						   type_name(SharedReference::weak(member.second->value)).c_str(),
						   reference_value(SharedReference::weak(member.second->value)).c_str());
				}
			}

			printf("}\n");
		}
		break;

	case package:
		printf("package: %s\n", static_cast<Package *>(value)->data->fullName().c_str());
		break;

	case function:
		printf("%s\n", function_value(static_cast<Function *>(value)).c_str());
		break;
	}

	return true;
}

void DebugPrinter::print(const char *value) {
	printf("\"%s\"\n", value);
}

void DebugPrinter::print(double value) {
	printf("%g\n", value);
}

void DebugPrinter::print(bool value) {
	printf("%s\n", value ? "true" : "false");
}

string reference_value(const SharedReference &reference) {

	char address[2 * sizeof(void *) + 3];

	switch (reference->data()->format) {
	case Data::fmt_none:
		return "none";

	case Data::fmt_null:
		return "null";

	case Data::fmt_number:
	case Data::fmt_boolean:
		return to_string(reference);

	case Data::fmt_object:
		switch (reference->data<Object>()->metadata->metatype()) {
		case Class::string:
			return "\"" + reference->data<String>()->str + "\"";

		case Class::regex:
			return reference->data<Regex>()->initializer;

		case Class::array:
			return array_value(reference->data<Array>());

		case Class::hash:
			return hash_value(reference->data<Hash>());

		case Class::iterator:
			return iterator_value(reference->data<Iterator>());

		case Class::library:
			return reference->data<Library>()->plugin->getPath();

		case Class::object:
		case Class::libobject:
			sprintf(address, "0x%p", static_cast<void *>(reference->data()));
		}
		break;

	case Data::fmt_package:
		return reference->data<Package>()->data->fullName();

	case Data::fmt_function:
		return function_value(reference->data<Function>());
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
		Module *module = AbstractSyntaxTree::instance().getModule(it->second.handle->module);
		DebugInfos *infos = AbstractSyntaxTree::instance().getDebugInfos(it->second.handle->module);
		join += to_string(it->first);
		join += "@";
		join += AbstractSyntaxTree::instance().getModuleName(module);
		join += "(line ";
		join += to_string(infos->lineNumber(it->second.handle->offset));
		join += ")";
	}

	return "function: " + join;
}

void print_script_context(size_t line_number, int digits, bool current, const string &line) {

	if (current) {
		term_cprint(stdout, "\033[1;31m");
		printf("% *zd >| ", digits, line_number);
		term_cprint(stdout, "\033[0m");
	}
	else {
		term_cprint(stdout, "\033[1;30m");
		printf("% *zd  | ", digits, line_number);
		term_cprint(stdout, "\033[0m");
	}

	print_highlighted(line);
}

void print_debug_trace(const char *format, ...) {

	va_list va_args;

	va_start(va_args, format);
	printf("\t");
	vprintf(format, va_args);
	printf("\n");
	va_end(va_args);
}

