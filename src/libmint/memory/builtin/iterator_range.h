#ifndef ITERATOR_RANGE_H
#define ITERATOR_RANGE_H

#include "iterator_p.h"

namespace mint::internal {

struct RangeFunctions {
	double (*inc)(double current);
	size_t (*size)(double begin, double end);
};

class RangeIteratorData : public IteratorData {
public:
	RangeIteratorData(double begin, double end);
	RangeIteratorData(RangeIteratorData &&) = delete;
	RangeIteratorData(const RangeIteratorData &other);
	~RangeIteratorData() override = default;

	RangeIteratorData &operator=(RangeIteratorData &&) = delete;
	RangeIteratorData &operator=(const RangeIteratorData &) = delete;

	[[nodiscard]] IteratorData *copy() override;
	void mark() override;

	[[nodiscard]] mint::Iterator::Context::Type get_type() const override;
	[[nodiscard]] mint::Iterator::Context::value_type &value() override;
	[[nodiscard]] mint::Iterator::Context::value_type &last() override;
	[[nodiscard]] size_t size() const override;
	[[nodiscard]] bool empty() const override;

	[[nodiscard]] size_t capacity() const override;
	void reserve(size_t capacity) override;

	void yield(mint::Iterator::Context::value_type &&value) override;
	void next() override;

	void finalize() override;
	void clear() override;

private:
	mint::WeakReference m_head;
	mint::WeakReference m_tail;
	const RangeFunctions *m_func;
};

}

#endif // ITERATOR_RANGE_HPP
