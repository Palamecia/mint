#include "object.h"
#include "class.h"
#include <functional>

Null::Null()
{ format = fmt_null; }

None::None()
{ format = fmt_none; }

Number::Number()
{ format = fmt_number; }

Object::Object(Class *type) : metadata(type)
{ format = fmt_object;
	data = new Reference [metadata->size()];
	for (auto itmember : metadata->members()) {
		data[itmember.second.offset].clone(itmember.second.value);
	}
	/// \todo call constructor
}

Object::~Object() {
	/// \todo call destructor
	delete [] data;
}

Function::Function()
{ format = fmt_function; }

Hash::Hash()
{ format = fmt_hash; }

bool Hash::compare::operator ()(const Reference &a, const Reference &b) const {

	/// \todo use ast

	switch (a.data()->format) {
	case fmt_number:
		return ((Number *)a.data())->data < ((Number *)b.data())->data;
	case fmt_object:
		if (((Object *)a.data())->metadata == StringClass::instance()) {
			return ((String *)a.data())->str < ((String *)b.data())->str;
		}
	}

	return false;
}

Array::Array()
{ format = fmt_array; }

String::String() : Object(StringClass::instance()) {}

Iterator::Iterator() : Object(IteratorClass::instance()) {}
