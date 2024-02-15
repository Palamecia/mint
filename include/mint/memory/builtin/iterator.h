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

#ifndef MINT_BUILTIN_ITERATOR_H
#define MINT_BUILTIN_ITERATOR_H

#include "mint/memory/class.h"
#include "mint/memory/object.h"

#include <optional>
#include <iterator>
#include <memory>

namespace _mint_iterator{
class data;
class data_iterator;
}

namespace mint {

class Cursor;

class MINT_EXPORT IteratorClass : public Class {
public:
	static IteratorClass *instance();

private:
	friend class GlobalData;
	IteratorClass();
};

struct MINT_EXPORT Iterator : public Object {
	Iterator();
	Iterator(Reference &ref);
	Iterator(const Iterator &other);
	Iterator(double begin, double end);
	Iterator(size_t stack_size);

	void mark() override;

	static WeakReference fromInclusiveRange(double begin, double end);
	static WeakReference fromExclusiveRange(double begin, double end);

	class MINT_EXPORT ctx_type {
	public:
		enum type { items, range, generator };
		using value_type = Reference;

		class MINT_EXPORT iterator {
		public:
			iterator(_mint_iterator::data_iterator *data);
			iterator(const iterator &other);
			iterator(iterator &&other);
			~iterator();

			iterator &operator =(const iterator &other);
			iterator &operator =(iterator &&other);

			bool operator ==(const iterator &other) const;
			bool operator !=(const iterator &other) const;

			value_type &operator *() const;
			value_type *operator ->() const;

			iterator operator ++(int);
			iterator &operator ++();

		private:
			std::unique_ptr<_mint_iterator::data_iterator> m_data;
		};

		ctx_type(_mint_iterator::data *data);
		ctx_type(const ctx_type &other);
		~ctx_type();

		ctx_type &operator =(const ctx_type &other);

		void mark();

		type getType() const;

		iterator begin() const;
		iterator end() const;

		value_type &next();
		value_type &back();

		void emplace(value_type &&value);
		void pop();

		void finalize();
		void clear();

		size_t size() const;
		bool empty() const;

	private:
		std::unique_ptr<_mint_iterator::data> m_data;
	};

	ctx_type ctx;

private:
	friend class GarbageCollector;
	static LocalPool<Iterator> g_pool;
};

MINT_EXPORT void iterator_new(Cursor *cursor, size_t length);
MINT_EXPORT Iterator *iterator_init(Reference &ref);
MINT_EXPORT Iterator *iterator_init(Reference &&ref);
MINT_EXPORT void iterator_insert(Iterator *iterator, Reference &&item);
MINT_EXPORT std::optional<WeakReference> iterator_get(Iterator *iterator);
MINT_EXPORT std::optional<WeakReference> iterator_next(Iterator *iterator);

}

template<>
struct std::iterator_traits<mint::Iterator::ctx_type::iterator> {
	using iterator_category = std::input_iterator_tag;
	using difference_type = size_t;
	using container_type = mint::Iterator::ctx_type;
	using value_type = container_type::value_type;
	using pointer = value_type *;
	using reference = value_type &;
};

#endif // MINT_BUILTIN_ITERATOR_H
