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

Function::Function()
{ format = fmt_function; }

Hash::Hash()
{ format = fmt_hash; }

bool Hash::compare::operator ()(const Reference &a, const Reference &b) const {

	/// \todo use ast

	switch (a.data()->format) {
	case fmt_number:
		return ((Number *)a.data())->value < ((Number *)b.data())->value;
	case fmt_object:
		if (((Object *)a.data())->metadata == StringClass::instance()) {
			return ((String *)a.data())->str < ((String *)b.data())->str;
		}
	}

	return false;
}

Array::Array()
{ format = fmt_array; }

Array::~Array() {
	for (auto item : values) {
		delete item;
	}
}

String::String() : Object(StringClass::instance()) {}

Iterator::Iterator() : Object(IteratorClass::instance()) {}
