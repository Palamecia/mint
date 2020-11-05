#include "definition.h"

#include <algorithm>
#include <memory>

using namespace std;

Definition::Definition(Type type, const string &name) :
	type(type),
	flags(0),
	name(name) {

}

Definition::~Definition() {

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
