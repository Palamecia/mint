#ifndef ITERATOR_HPP
#define ITERATOR_HPP

#include <memory/builtin/iterator.h>

namespace _mint_iterator {

class data_iterator {
public:
	data_iterator() = default;
	virtual ~data_iterator() = default;

	virtual mint::Iterator::ctx_type::value_type &get() = 0;
	virtual bool compare(data_iterator *other) const = 0;
	virtual data_iterator *copy() = 0;
	virtual void next() = 0;
};

class data {
public:
	data() = default;
	virtual ~data() = default;

	virtual void mark() = 0;

	virtual mint::Iterator::ctx_type::type getType() = 0;
	virtual data *copy() const = 0;

	virtual data_iterator *begin() = 0;
	virtual data_iterator *end() = 0;

	virtual mint::Iterator::ctx_type::value_type &front() = 0;
	virtual mint::Iterator::ctx_type::value_type &back() = 0;

	virtual void emplace_front(mint::Iterator::ctx_type::value_type &&value) = 0;
	virtual void emplace_back(mint::Iterator::ctx_type::value_type &&value) = 0;

	virtual void pop_front() = 0;
	virtual void pop_back() = 0;

	virtual void finalize() = 0;
	virtual void clear() = 0;

	virtual size_t size() const = 0;
	virtual bool empty() const = 0;
};

}

#endif // ITERATOR_HPP
