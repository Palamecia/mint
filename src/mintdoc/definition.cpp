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
