#include "debugprinter.h"

#include <memory/builtin/library.h>
#include <memory/builtin/string.h>
#include <memory/builtin/regex.h>
#include <memory/memorytool.h>
#include <memory/casttool.h>
#include <system/plugin.h>

#include <stdio.h>
#include <stdarg.h>

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
						   member.first.c_str(),
						   type_name(SharedReference::unsafe(&member.second->value)).c_str(),
						   reference_value(SharedReference::unsafe(object->data + member.second->offset)).c_str());
				}
			}
			else {
				for (auto member : object->metadata->members()) {
					printf("\t%s : (%s) %s\n",
						   member.first.c_str(),
						   type_name(SharedReference::unsafe(&member.second->value)).c_str(),
						   reference_value(SharedReference::unsafe(&member.second->value)).c_str());
				}
			}

			printf("}\n");
		}
		break;

	case package:
		printf("package: %s\n", static_cast<Package *>(value)->data->fullName().c_str());
		break;

	case function:
		printf("function\n");
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
		return "function";
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
		join += reference_value(*it);
	}

	return "[" + join + "]";
}

string hash_value(Hash *hash) {

	string join;

	for (auto it = hash->values.begin(); it != hash->values.end(); ++it) {
		if (it != hash->values.begin()) {
			join += ", ";
		}
		join += reference_value(it->first);
		join += " : ";
		join += reference_value(it->second);
	}

	return "{" + join + "}";
}

void print_debug_trace(const char *format, ...) {

	va_list va_args;

	va_start(va_args, format);
	printf("\t");
	vprintf(format, va_args);
	printf("\n");
	va_end(va_args);
}

