/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#ifndef MINT_MEMORY_BUILTIN_LIBOBJECT_H
#define MINT_MEMORY_BUILTIN_LIBOBJECT_H

#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/memorypool.hpp"
#include "mint/memory/object.h"

namespace mint {

class AbstractSyntaxTree;
class GarbageCollector;

class MINT_EXPORT LibObjectClass : public Class {
public:
	LibObjectClass(AbstractSyntaxTree& ast);
	static LibObjectClass& instance(AbstractSyntaxTree& ast);
};

template<typename Type>
struct LibObject : public Object {
	friend class GarbageCollector;
public:
	using object_type = Type;

	explicit LibObject(AbstractSyntaxTree& ast);
	LibObject(AbstractSyntaxTree& ast, object_type* ptr);

	object_type* ptr = nullptr;

private:
	static SystemPool<LibObject<Type>> g_pool;
};

template<typename Type>
LibObject<Type>::LibObject(AbstractSyntaxTree& ast) :
    Object(LibObjectClass::instance(ast)) {}

template<typename Type>
LibObject<Type>::LibObject(AbstractSyntaxTree& ast, object_type* ptr) :
    Object(LibObjectClass::instance(ast)),
    ptr(ptr) {}

template<typename Type>
SystemPool<LibObject<Type>> LibObject<Type>::g_pool;

}

#endif // MINT_MEMORY_BUILTIN_LIBOBJECT_H
