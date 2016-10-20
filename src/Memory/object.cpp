#include "Memory/object.h"
#include "Memory/class.h"
#include "Memory/operatortool.h"
#include "Scheduler/processor.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

#include <functional>

Null::Null()
{ format = fmt_null; }

None::None()
{ format = fmt_none; }

Number::Number()
{ format = fmt_number; }

Object::Object(Class *type) : metadata(type), data(nullptr)
{ format = fmt_object; }

Object::~Object() {

	auto destructor = metadata->members().find("delete");
	if (destructor != metadata->members().end()) {

		AbstractSynatxTree ast;
		ast.call(0, 0);

		ast.stack().push_back(SharedReference::unique(new Reference(Reference::standard, this)));
		ast.waitingCalls().push(&data[destructor->second->offset]);

		AbstractSynatxTree::CallHandler handler = ast.getCallHandler();
		call_member_operator(&ast, 0);
		while (ast.callInProgress(handler)) {
			run_step(&ast);
		}
	}

	delete [] data;
}

void Object::construct() {

	data = new Reference [metadata->size()];
	for (auto member : metadata->members()) {
		data[member.second->offset].clone(member.second->value);
	}
}

void Object::construct(const Object &other) {

	data = new Reference [metadata->size()];
	for (auto member : metadata->members()) {
		data[member.second->offset].clone(other.data[member.second->offset]);
	}
}

Function::Function()
{ format = fmt_function; }

String::String() : Object(StringClass::instance()) {}

Array::Array() : Object(ArrayClass::instance()) {}

Hash::Hash() : Object(HashClass::instance()) {}

Iterator::Iterator() : Object(IteratorClass::instance()) {}

Library::Library() : Object(LibraryClass::instance())
{ plugin = nullptr; }

Library::~Library()
{ delete plugin; }
