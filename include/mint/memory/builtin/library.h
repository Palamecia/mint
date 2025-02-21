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

#ifndef MINT_BUILTIN_LIBRARY_H
#define MINT_BUILTIN_LIBRARY_H

#include "mint/memory/class.h"
#include "mint/memory/object.h"

namespace mint {

class Plugin;

class MINT_EXPORT LibraryClass : public Class {
	friend class GlobalData;
public:
	static LibraryClass *instance();

private:
	LibraryClass();
};

struct MINT_EXPORT Library : public Object {
	friend class GarbageCollector;
public:
	Library();
	Library(Library &&other) noexcept;
	Library(const Library &other);
	~Library() override;

	Library &operator=(Library &&other) noexcept;
	Library &operator=(const Library &other);

	Plugin *plugin;

private:
	static LocalPool<Library> g_pool;
};
}

#endif // MINT_BUILTIN_LIBRARY_H
