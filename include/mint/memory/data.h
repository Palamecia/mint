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

#ifndef MINT_DATA_H
#define MINT_DATA_H

#include "mint/config.h"

#include <cstddef>

namespace mint {

struct MemoryInfos {
	bool reachable = true;
	bool collected = false;
	size_t refcount = 0;
};

struct MINT_EXPORT Data {
	enum Format {
		FMT_NONE,
		FMT_NULL,
		FMT_NUMBER,
		FMT_BOOLEAN,
		FMT_OBJECT,
		FMT_PACKAGE,
		FMT_FUNCTION
	};
	const Format format;

	virtual void mark();

protected:
	friend class GarbageCollector;

	explicit Data(Format fmt);
	Data(const Data &other) = delete;
	virtual ~Data() = default;

	Data &operator =(const Data &other) = delete;

	bool marked_bit() const;

private:
	MemoryInfos infos;
	Data *prev = nullptr;
	Data *next = nullptr;
};

struct MINT_EXPORT None : public Data {
protected:
	friend class GlobalData;
	None();
};

struct MINT_EXPORT Null : public Data {
protected:
	friend class GlobalData;
	Null();
};

}

#endif // MINT_DATA_H
