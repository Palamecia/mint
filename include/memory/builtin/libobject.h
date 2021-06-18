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
	using impl_type = Type;
	impl_type *impl;

private:
	friend class Reference;
	static SystemPool<LibObject<Type>> g_pool;
};

template<typename Type>
LibObject<Type>::LibObject() :
	Object(LibObjectClass::instance()) {
	impl = nullptr;
}

template<typename Type>
SystemPool<LibObject<Type>> LibObject<Type>::g_pool;

}

#endif // LIB_OBJECT_H
