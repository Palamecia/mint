#include "memory/object.h"
#include "memory/class.h"
#include "memory/operatortool.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"

#include <functional>

using namespace std;
using namespace mint;

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
	delete [] data;
}

void Object::construct() {

	data = new Reference [metadata->size()];
	for (auto member : metadata->members()) {
		data[member.second->offset].clone(member.second->value);
	}
}

void Object::construct(const Object &other) {

	if (other.data) {
		data = new Reference [metadata->size()];
		for (auto member : metadata->members()) {
			data[member.second->offset].clone(other.data[member.second->offset]);
		}
	}
}

Function::Function()
{ format = fmt_function; }

Function::Handler::Handler(int module, int offset) :
	module(module),
	offset(offset),
	capture(nullptr) {}
