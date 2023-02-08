/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_FUNCTIONTOOL_H
#define MINT_FUNCTIONTOOL_H

#include "mint/memory/memorytool.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/config.h"

#include <initializer_list>

#ifdef OS_WINDOWS
#include <BaseTsd.h>
using HANDLE = void *;
using ssize_t = SSIZE_T;
#endif

#define MINT_FUNCTION(__name, __argc, __cursor) \
	extern "C" DECL_EXPORT void __name##_##__argc(mint::Cursor *__cursor)
#define VARIADIC ~

namespace mint {

class FunctionHelper;

class MINT_EXPORT ReferenceHelper {
public:
	ReferenceHelper operator [](const Symbol &symbol) const;
	ReferenceHelper member(const Symbol &symbol) const;

	operator Reference &();
	operator Reference &&();

	const Reference &operator *() const;
	const Reference *operator ->() const;
	const Reference *get() const;

protected:
	ReferenceHelper(const FunctionHelper *function, Reference &&reference);
	friend class FunctionHelper;

private:
	const FunctionHelper *m_function;
	WeakReference m_reference;
};

class MINT_EXPORT FunctionHelper {
public:
	FunctionHelper(Cursor *cursor, size_t argc);
	~FunctionHelper();

	Reference &pop_parameter();

	ReferenceHelper reference(const Symbol &symbol) const;
	ReferenceHelper member(const Reference &object, const Symbol &symbol) const;

	void return_value(Reference &&value);

private:
	Cursor *m_cursor;
	ssize_t m_top;
	ssize_t m_base;
	bool m_value_returned;
};

MINT_EXPORT WeakReference create_number(double value);
MINT_EXPORT WeakReference create_boolean(bool value);
MINT_EXPORT WeakReference create_string(const std::string &value);
MINT_EXPORT WeakReference create_array(Array::values_type &&values);
MINT_EXPORT WeakReference create_array(std::initializer_list<WeakReference> items);
MINT_EXPORT WeakReference create_hash(mint::Hash::values_type &&values);
MINT_EXPORT WeakReference create_hash(std::initializer_list<std::pair<WeakReference, WeakReference>> items);

MINT_EXPORT WeakReference create_array();
MINT_EXPORT WeakReference create_hash();
MINT_EXPORT WeakReference create_iterator();

template<class Type>
WeakReference create_object(Type *object) {

	WeakReference ref = WeakReference::create<LibObject<Type>>();
	ref.data<LibObject<Type>>()->construct();
	ref.data<LibObject<Type>>()->impl = object;
	return ref;
}

MINT_EXPORT WeakReference get_member_ignore_visibility(Object *object, const Symbol &member);

#ifdef OS_WINDOWS
using handle_t = HANDLE;
MINT_EXPORT WeakReference create_handle(handle_t handle);
MINT_EXPORT handle_t to_handle(const Reference &reference);
MINT_EXPORT handle_t *to_handle_ptr(const Reference &reference);
#else
using handle_t = int;
MINT_EXPORT WeakReference create_handle(handle_t handle);
MINT_EXPORT handle_t to_handle(const Reference &reference);
MINT_EXPORT handle_t *to_handle_ptr(const Reference &reference);
#endif

// ...

}

#endif // MINT_FUNCTIONTOOL_H
