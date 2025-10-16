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

#ifndef MINT_MEMORY_DATA_H
#define MINT_MEMORY_DATA_H

#include "mint/config.h"

#include <cstddef>
#include <cstdint>

namespace mint {

struct MemoryInfo {
	bool reachable = true;
	bool collected = false;
	std::size_t refcount = 0;
};

class MINT_EXPORT Data {
	friend class GarbageCollector;
public:
	enum Format : std::uint8_t {
		none_format,
		null_format,
		number_format,
		boolean_format,
		object_format,
		package_format,
		function_format
	};

	Data();
	Data(Data&& other) noexcept;
	Data(const Data& other);
	virtual ~Data() = default;

	Data& operator=(Data&&) = delete;
	Data& operator=(const Data&) = delete;

	[[nodiscard]] virtual Format format() const = 0;
	virtual void mark();

protected:
	[[nodiscard]] bool marked_bit() const;

private:
	MemoryInfo _info;
	Data* _prev = nullptr;
	Data* _next = nullptr;
};

class MINT_EXPORT None : public Data {
	friend class GlobalData;
public:
	[[nodiscard]] Format format() const override {
		return none_format;
	}
};

class MINT_EXPORT Null : public Data {
	friend class GlobalData;
public:
	[[nodiscard]] Format format() const override {
		return null_format;
	}
};

}

#endif // MINT_MEMORY_DATA_H
