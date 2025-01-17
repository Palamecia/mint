#ifndef ITERATOR_HPP
#define ITERATOR_HPP

#include "mint/memory/builtin/iterator.h"

namespace mint::internal {

class IteratorData {
public:
	IteratorData() = default;
	IteratorData(IteratorData &&) = delete;
	IteratorData(const IteratorData &) = default;
	virtual ~IteratorData() = default;

	IteratorData &operator=(IteratorData &&) = delete;
	IteratorData &operator=(const IteratorData &) = default;

	[[nodiscard]] virtual IteratorData *copy() = 0;
	virtual void mark() = 0;

	[[nodiscard]] virtual mint::Iterator::Context::Type get_type() const = 0;
	[[nodiscard]] virtual mint::Iterator::Context::value_type &value() = 0;
	[[nodiscard]] virtual mint::Iterator::Context::value_type &last() = 0;
	[[nodiscard]] virtual size_t size() const = 0;
	[[nodiscard]] virtual bool empty() const = 0;

	[[nodiscard]] virtual size_t capacity() const = 0;
	virtual void reserve(size_t capacity) = 0;

	virtual void yield(mint::Iterator::Context::value_type &&value) = 0;
	virtual void next() = 0;

	virtual void finalize() = 0;
	virtual void clear() = 0;
};

}

#endif // ITERATOR_HPP
