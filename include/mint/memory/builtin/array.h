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

#ifndef MINT_BUILTIN_ARRAY_H
#define MINT_BUILTIN_ARRAY_H

#include "mint/memory/class.h"
#include "mint/memory/object.h"

namespace mint {

class Cursor;

class MINT_EXPORT ArrayClass : public Class {
public:
	static ArrayClass *instance();

private:
	friend class GlobalData;
	ArrayClass();
};

struct MINT_EXPORT Array : public Object {
	Array();
	Array(const Array &other);

	Array &operator =(const Array &other) = delete;

	void mark() override;

	using values_type = std::vector<WeakReference>;
	values_type values;

private:
	friend class GarbageCollector;
	static LocalPool<Array> g_pool;
};

MINT_EXPORT void array_new(Cursor *cursor, size_t length);
MINT_EXPORT void array_append(Array *array, Reference &item);
MINT_EXPORT void array_append(Array *array, Reference &&item);
MINT_EXPORT WeakReference array_insert(Array *array, intmax_t index, Reference &item);
MINT_EXPORT WeakReference array_insert(Array *array, intmax_t index, Reference &&item);
MINT_EXPORT WeakReference array_get_item(Array *array, intmax_t index);
MINT_EXPORT WeakReference array_get_item(Array::values_type::iterator &it);
MINT_EXPORT WeakReference array_get_item(Array::values_type::value_type &value);
MINT_EXPORT size_t array_index(const Array *array, intmax_t index);
MINT_EXPORT WeakReference array_item(const Reference &item);

}

#endif // MINT_BUILTIN_ARRAY_H
