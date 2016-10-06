#include "object.h"
#include "class.h"
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
	/// \todo call destructor
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
