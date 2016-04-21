#include "object.h"
#include "class.h"

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

Array::Array()
{ format = fmt_array; }

String::String() : Object(StringClass::instance()) {}
