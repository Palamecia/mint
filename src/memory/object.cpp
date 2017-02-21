#include "memory/object.h"
#include "memory/class.h"
#include "memory/operatortool.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"

#include <functional>

Null::Null()
{ format = fmt_null; }

None::None()
{ format = fmt_none; }

Number::Number()
{ format = fmt_number; }

Boolean::Boolean()
{ format = fmt_boolean; }

Object::Object(Class *type) : metadata(type), data(nullptr)
{ format = fmt_object; }

Object::~Object() {

	if (data) {

		auto destructor = metadata->members().find("delete");
		if (destructor != metadata->members().end()) {

			AbstractSyntaxTree ast;

			ast.stack().push_back(SharedReference::unique(new Reference(Reference::standard, this)));
			ast.waitingCalls().push(&data[destructor->second->offset]);

			AbstractSyntaxTree::CallHandler handler = ast.getCallHandler();
			call_member_operator(&ast, 0);
			while (ast.callInProgress(handler)) {
				run_step(&ast);
			}
		}

		delete [] data;
	}
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
