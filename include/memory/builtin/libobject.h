#ifndef LIB_OBJECT_H
#define LIB_OBJECT_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT LibObjectClass : public Class {
public:
	static LibObjectClass *instance();

private:
	LibObjectClass();
};

template<typename Type>
struct MINT_EXPORT LibObject : public Object {
	LibObject() : Object(LibObjectClass::instance()) {
		impl = nullptr;
	}
	Type *impl;
};

}

#endif // LIB_OBJECT_H
