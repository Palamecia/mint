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

#ifndef LIBMINT_MEMORY_BUILTIN_ITERATOR_P_H
#define LIBMINT_MEMORY_BUILTIN_ITERATOR_P_H

#include "mint/ast/cursor.h"
#include "mint/memory/builtin/iterator.h"
#include <cstddef>
#include <memory>

namespace mint::internal {

class IteratorViewData {
public:
	IteratorViewData() = default;
	IteratorViewData(IteratorViewData&&) = delete;
	IteratorViewData(const IteratorViewData&) = delete;
	virtual ~IteratorViewData() = default;

	IteratorViewData& operator=(IteratorViewData&&) = delete;
	IteratorViewData& operator=(const IteratorViewData&) = delete;

	[[nodiscard]] virtual mint::Iterator::Context::reference front() = 0;
	[[nodiscard]] virtual mint::Iterator::Context::reference back() = 0;
	[[nodiscard]] virtual mint::Iterator::Context::reference get() = 0;
	[[nodiscard]] virtual bool empty() const = 0;

	virtual void prev() = 0;
	virtual void next() = 0;
};

class IteratorData {
public:
	IteratorData() = default;
	IteratorData(IteratorData&&) = default;
	IteratorData(const IteratorData&) = default;
	virtual ~IteratorData() = default;

	IteratorData& operator=(IteratorData&&) = default;
	IteratorData& operator=(const IteratorData&) = default;

	[[nodiscard]] virtual std::unique_ptr<IteratorViewData> view() = 0;
	[[nodiscard]] virtual std::unique_ptr<IteratorData> copy() = 0;
	virtual void mark() = 0;

	[[nodiscard]] virtual mint::Iterator::Context::Type get_type() const = 0;
	[[nodiscard]] virtual mint::Iterator::Context::reference get() = 0;
	[[nodiscard]] virtual std::size_t size() const = 0;
	[[nodiscard]] virtual bool empty() const = 0;

	[[nodiscard]] virtual std::size_t capacity() const = 0;
	virtual void reserve(std::size_t capacity) = 0;

	virtual void yield(mint::Cursor& cursor, mint::Iterator::Context::value_type&& value,
	    Iterator::ResumeKind resume_kind) = 0;
	virtual void next(mint::Cursor& cursor) = 0;

	virtual void finalize(mint::Cursor& cursor) = 0;
	virtual void clear() = 0;
};

}

#endif // LIBMINT_MEMORY_BUILTIN_ITERATOR_P_H
