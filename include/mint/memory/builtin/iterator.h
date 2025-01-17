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

#include <cstddef>
#include <optional>
#include <iterator>
#include <memory>

namespace mint {

namespace internal {
class IteratorData;
class IteratorDataIterator;
}

class Cursor;

class MINT_EXPORT IteratorClass : public Class {
	friend class GlobalData;
public:
	static IteratorClass *instance();

private:
	IteratorClass();
};

struct MINT_EXPORT Iterator : public Object {
	friend class GarbageCollector;
public:
	Iterator();
	explicit Iterator(size_t capacity);
	explicit Iterator(Reference &ref);
	explicit Iterator(Reference &&ref);
	explicit Iterator(mint::internal::IteratorData *data);
	Iterator(const Iterator &other);
	Iterator(Iterator &&other) noexcept;
	~Iterator() override = default;

	Iterator &operator=(const Iterator &other);
	Iterator &operator=(Iterator &&other) noexcept;

	static Iterator *from_generator(size_t stack_size);
	static Iterator *from_inclusive_range(double begin, double end);
	static Iterator *from_exclusive_range(double begin, double end);

	void mark() override;

	class MINT_EXPORT Context {
	public:
		enum Type : std::uint8_t {
			ITEMS,
			RANGE,
			GENERATOR
		};

		using value_type = Reference;

		class MINT_EXPORT iterator {
		public:
			explicit iterator(Context *context);
			iterator(iterator &&other) noexcept = default;
			iterator(const iterator &other) = default;
			~iterator() = default;

			iterator &operator=(iterator &&other) noexcept = default;
			iterator &operator=(const iterator &other) = default;

			bool operator==(const iterator &other) const;
			bool operator!=(const iterator &other) const;

			value_type &operator*() const;
			value_type *operator->() const;

			iterator operator++(int);
			iterator &operator++();

		private:
			Context *m_context;
		};

		explicit Context(mint::internal::IteratorData *data);
		Context(const Context &other);
		Context(Context &&other) noexcept;
		~Context();

		Context &operator=(Context &&other) noexcept;
		Context &operator=(const Context &other);

		iterator begin();
		iterator end();
		void mark();

		[[nodiscard]] Type get_type() const;
		[[nodiscard]] value_type &value();
		[[nodiscard]] value_type &last();
		[[nodiscard]] size_t size() const;
		[[nodiscard]] bool empty() const;

		[[nodiscard]] size_t capacity() const;
		void reserve(size_t capacity);

		void yield(value_type &&value);
		void next();

		void finalize();
		void clear();

	private:
		std::unique_ptr<mint::internal::IteratorData> m_data;
	};

	Context ctx;

private:
	static LocalPool<Iterator> g_pool;
};

MINT_EXPORT void iterator_new(Cursor *cursor, size_t length);
MINT_EXPORT Iterator *iterator_init(Reference &ref);
MINT_EXPORT Iterator *iterator_init(Reference &&ref);
MINT_EXPORT void iterator_yield(Iterator *iterator, Reference &&item);
MINT_EXPORT std::optional<WeakReference> iterator_get(Iterator *iterator);
MINT_EXPORT std::optional<WeakReference> iterator_next(Iterator *iterator);

}

template<>
struct std::iterator_traits<mint::Iterator::Context::iterator> {
	using iterator_category = std::input_iterator_tag;
	using difference_type = size_t;
	using container_type = mint::Iterator::Context;
	using value_type = container_type::value_type;
	using pointer = value_type *;
	using reference = value_type &;
};

#endif // MINT_BUILTIN_ITERATOR_H
