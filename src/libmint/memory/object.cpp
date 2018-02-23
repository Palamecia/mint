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

Number::Number() : Data(fmt_number) {

}

Boolean::Boolean() : Data(fmt_boolean) {

}

Object::Object(Class *type) :
	Data(fmt_object),
	metadata(type),
	data(nullptr) {

}

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

Function::Function() : Data(fmt_function) {

}

Function::Handler::Handler(int module, int offset) :
	module(module),
	offset(offset),
	capture(nullptr) {}
