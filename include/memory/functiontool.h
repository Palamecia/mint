#ifndef FUNCTION_TOOL_H
#define FUNCTION_TOOL_H

#include "memory/memorytool.h"
#include "memory/builtin/libobject.h"

#include <initializer_list>
#include <config.h>

#ifdef OS_WINDOWS
#include <BaseTsd.h>
using ssize_t = SSIZE_T;
#endif

#define MINT_FUNCTION(__name, __argc, __cursor) \
	extern "C" DECL_EXPORT void __name##_##__argc(Cursor *__cursor)
#define VARIADIC -

namespace mint {

class FunctionHelper;

class MINT_EXPORT ReferenceHelper {
public:
	ReferenceHelper operator [](const std::string &symbol) const;
	ReferenceHelper member(const std::string &symbol) const;

	operator SharedReference &();
	operator SharedReference &&();
	Reference &operator *() const;
	Reference *operator ->() const;
	Reference *get() const;

protected:
	ReferenceHelper(const FunctionHelper *function, SharedReference &&reference);
	friend class FunctionHelper;

private:
	const FunctionHelper *m_function;
	SharedReference m_reference;
};

class MINT_EXPORT FunctionHelper {
public:
	FunctionHelper(Cursor *cursor, size_t argc);
	~FunctionHelper();

	SharedReference &popParameter();

	ReferenceHelper reference(const std::string &symbol) const;
	ReferenceHelper member(const SharedReference &object, const std::string &symbol) const;

	void returnValue(Reference *value);
	void returnValue(SharedReference &&value);

private:
	Cursor *m_cursor;
	ssize_t m_top;
	ssize_t m_base;
	bool m_valueReturned;
};

MINT_EXPORT SharedReference create_number(double value);
MINT_EXPORT SharedReference create_boolean(bool value);
MINT_EXPORT SharedReference create_string(const std::string &value);
MINT_EXPORT SharedReference create_array(Array::values_type &&values);
MINT_EXPORT SharedReference create_array(std::initializer_list<SharedReference> items);
MINT_EXPORT SharedReference create_hash(mint::Hash::values_type &&values);
MINT_EXPORT SharedReference create_hash(std::initializer_list<std::pair<SharedReference, SharedReference>> items);

template<class Type>
SharedReference create_object(Type *object) {

	Reference *ref = StrongReference::create<LibObject<Type>>();
	ref->data<LibObject<Type>>()->construct();
	ref->data<LibObject<Type>>()->impl = object;
	return SharedReference::unique(ref);
}

// ...

}

#endif // FUNCTION_TOOL_H
