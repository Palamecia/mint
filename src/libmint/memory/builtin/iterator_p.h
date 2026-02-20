#ifndef ITERATOR_HPP
#define ITERATOR_HPP

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

#endif // ITERATOR_HPP
