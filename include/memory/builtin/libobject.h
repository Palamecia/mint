#ifndef LIB_OBJECT_H
#define LIB_OBJECT_H

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

#endif // LIB_OBJECT_H
