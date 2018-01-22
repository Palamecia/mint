#ifndef FUNCTION_TOOL_H
#define FUNCTION_TOOL_H

#include "memory/memorytool.h"
#include "memory/builtin/libobject.h"

#define MINT_FUNCTION(__name, __argc, __cursor) \
	extern "C" void __name##_##__argc(Cursor *__cursor)

namespace mint {

class MINT_EXPORT FunctionHelper {
public:
	FunctionHelper(Cursor *cursor, size_t argc);
	~FunctionHelper();

	SharedReference &popParameter();

	void returnValue(const SharedReference &value);
	void returnValue(Reference *value);

private:
	Cursor *m_cursor;
	size_t m_top;
	size_t m_base;
	bool m_valueReturned;
};

MINT_EXPORT SharedReference create_number(double value);
MINT_EXPORT SharedReference create_boolean(bool value);
MINT_EXPORT SharedReference create_string(const std::string &value);

template<class Type>
SharedReference create_object(Type *object) {

	Reference *ref = Reference::create<LibObject<Type>>();
	ref->data<LibObject<Type>>()->construct();
	ref->data<LibObject<Type>>()->impl = object;
	return SharedReference::unique(ref);
}

// ...

}

#endif // FUNCTION_TOOL_H
