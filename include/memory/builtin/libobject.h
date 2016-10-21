#ifndef LIB_OBJECT_HPP
#define LIB_OBJECT_HPP

#include "memory/class.h"
#include "memory/object.h"

class LibObjectClass : public Class {
public:
	static LibObjectClass *instance();

private:
	LibObjectClass();
};

template<typename Type>
struct LibObject : public Object {
	LibObject() : Object(LibObjectClass::instance()) {
		impl = nullptr;
	}
	Type *impl;
};

#endif // LIB_OBJECT_HPP
