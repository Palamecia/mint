/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#ifndef MINT_BUILTIN_LIBOBJECT_H
#define MINT_BUILTIN_LIBOBJECT_H

#include "mint/memory/class.h"
#include "mint/memory/object.h"

namespace mint {

class MINT_EXPORT LibObjectClass : public Class {
	friend class GlobalData;
public:
	static LibObjectClass *instance();

private:
	LibObjectClass();
};

template<typename Type>
struct LibObject : public Object {
	friend class GarbageCollector;
public:
	LibObject();
	using impl_type = Type;
	impl_type *impl = nullptr;

private:
	static SystemPool<LibObject<Type>> g_pool;
};

template<typename Type>
LibObject<Type>::LibObject() :
	Object(LibObjectClass::instance()) {}

template<typename Type>
SystemPool<LibObject<Type>> LibObject<Type>::g_pool;

}

#endif // MINT_BUILTIN_LIBOBJECT_H
