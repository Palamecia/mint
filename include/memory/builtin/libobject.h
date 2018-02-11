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
struct LibObject : public Object {
	LibObject();
	typedef Type impl_type;
	impl_type *impl;
};

template<typename Type>
LibObject<Type>::LibObject() :
	Object(LibObjectClass::instance()) {
	impl = nullptr;
}

}

#endif // LIB_OBJECT_H
