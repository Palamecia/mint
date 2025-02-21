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

#ifndef MINT_DATA_H
#define MINT_DATA_H

#include "mint/config.h"

#include <cstddef>
#include <cstdint>

namespace mint {

struct MemoryInfos {
	bool reachable = true;
	bool collected = false;
	size_t refcount = 0;
};

struct MINT_EXPORT Data {
	friend class GarbageCollector;
public:
	enum Format : std::uint8_t {
		FMT_NONE,
		FMT_NULL,
		FMT_NUMBER,
		FMT_BOOLEAN,
		FMT_OBJECT,
		FMT_PACKAGE,
		FMT_FUNCTION
	};

	Data(Data &&other) = delete;
	Data(const Data &other) = delete;

	Data &operator=(Data &&other) = delete;
	Data &operator=(const Data &other) = delete;

	const Format format;

	virtual void mark();

protected:
	explicit Data(Format fmt);
	virtual ~Data() = default;

	[[nodiscard]] bool marked_bit() const;

private:
	MemoryInfos infos;
	Data *prev = nullptr;
	Data *next = nullptr;
};

struct MINT_EXPORT None : public Data {
	friend class GlobalData;
protected:
	None();
};

struct MINT_EXPORT Null : public Data {
	friend class GlobalData;
protected:
	Null();
};

}

#endif // MINT_DATA_H
