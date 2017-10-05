#ifndef FUNCTION_TOOL_H
#define FUNCTION_TOOL_H

#include "memory/memorytool.h"
#include "memory/builtin/libobject.h"

class FunctionHelper {
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

SharedReference create_number(double value);
SharedReference create_boolean(bool value);
SharedReference create_string(const std::string &value);

template<class Type>
SharedReference create_object(Type *object) {

	Reference *ref = Reference::create<LibObject<Type>>();
	((LibObject<Type> *)ref->data())->construct();
	((LibObject<Type> *)ref->data())->impl = object;
	return SharedReference::unique(ref);
}

// ...

#endif // FUNCTION_TOOL_H
