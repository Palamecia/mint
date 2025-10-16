#ifndef LIBMINT_MEMORY_BUILTIN_ITERATOR_RANGE_H
#define LIBMINT_MEMORY_BUILTIN_ITERATOR_RANGE_H

#include "iterator_p.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <functional>
#include <memory>

namespace mint::internal {

struct RangeFunctions {
	double (*inc)(double current);
	double (*dec)(double current);
	std::size_t (*size)(double begin, double end);
};

class RangeIteratorViewData : public IteratorViewData {
	std::reference_wrapper<const RangeFunctions> _func;
	mint::WeakReference _head;
	mint::WeakReference _tail;
	mint::WeakReference _cur;
public:
	RangeIteratorViewData(const RangeFunctions& func, mint::WeakReference& head, mint::WeakReference& tail);
	RangeIteratorViewData(RangeIteratorViewData&&) = delete;
	RangeIteratorViewData(const RangeIteratorViewData&) = delete;
	virtual ~RangeIteratorViewData() = default;

	RangeIteratorViewData& operator=(RangeIteratorViewData&&) = delete;
	RangeIteratorViewData& operator=(const RangeIteratorViewData&) = delete;

	[[nodiscard]] mint::Iterator::Context::reference front() override;
	[[nodiscard]] mint::Iterator::Context::reference back() override;
	[[nodiscard]] mint::Iterator::Context::reference get() override;
	[[nodiscard]] bool empty() const override;

	void prev() override;
	void next() override;
};

class RangeIteratorData : public IteratorData {
	std::reference_wrapper<const RangeFunctions> _func;
	mint::WeakReference _head;
	mint::WeakReference _tail;
public:
	RangeIteratorData(double begin, double end);
	RangeIteratorData(RangeIteratorData&&) = delete;
	RangeIteratorData(const RangeIteratorData& other);
	~RangeIteratorData() override = default;

	RangeIteratorData& operator=(RangeIteratorData&&) = delete;
	RangeIteratorData& operator=(const RangeIteratorData&) = delete;

	[[nodiscard]] std::unique_ptr<IteratorViewData> view() override;
	[[nodiscard]] std::unique_ptr<IteratorData> copy() override;
	void mark() override;

	[[nodiscard]] mint::Iterator::Context::Type get_type() const override;
	[[nodiscard]] mint::Iterator::Context::value_type& get() override;
	[[nodiscard]] std::size_t size() const override;
	[[nodiscard]] bool empty() const override;

	[[nodiscard]] std::size_t capacity() const override;
	void reserve(std::size_t capacity) override;

	void yield(mint::Iterator::Context::value_type&& value) override;
	void next() override;

	void finalize() override;
	void clear() override;
};

}

#endif // LIBMINT_MEMORY_BUILTIN_ITERATOR_RANGE_H
