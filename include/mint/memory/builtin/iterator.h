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

#ifndef MINT_MEMORY_BUILTIN_ITERATOR_H
#define MINT_MEMORY_BUILTIN_ITERATOR_H

#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"

#include <cstddef>
#include <optional>
#include <iterator>
#include <memory>

namespace mint {

namespace internal {

class IteratorData;
class IteratorDataIterator;
class IteratorViewData;
class IteratorViewDataIterator;

}

class AbstractSyntaxTree;
class GarbageCollector;
class Cursor;

class MINT_EXPORT IteratorClass : public Class {
public:
	IteratorClass(AbstractSyntaxTree& ast);
	static IteratorClass& instance(AbstractSyntaxTree& ast);
};

struct FromGenerator {};

struct FromInclusiveRange {};

struct FromExclusiveRange {};

inline constexpr FromGenerator from_generator;
inline constexpr FromInclusiveRange from_inclusive_range;
inline constexpr FromExclusiveRange from_exclusive_range;

class MINT_EXPORT Iterator : public Object {
	friend class GarbageCollector;
public:
	explicit Iterator(AbstractSyntaxTree& ast);
	Iterator(AbstractSyntaxTree& ast, std::size_t capacity);
	Iterator(AbstractSyntaxTree& ast, const Reference& ref);
	Iterator(AbstractSyntaxTree& ast, Reference&& ref);
	Iterator(AbstractSyntaxTree& ast, std::unique_ptr<mint::internal::IteratorData>&& data);
	Iterator(FromGenerator /*from_generator*/, AbstractSyntaxTree& ast, std::size_t stack_size);
	Iterator(FromInclusiveRange /*from_inclusive_range*/, AbstractSyntaxTree& ast, double begin, double end);
	Iterator(FromExclusiveRange /*from_exclusive_range*/, AbstractSyntaxTree& ast, double begin, double end);

	Iterator(const Iterator& other);
	Iterator(Iterator&& other) noexcept;
	~Iterator() override = default;

	Iterator& operator=(const Iterator& other);
	Iterator& operator=(Iterator&& other) noexcept;

	void mark() override;

	class MINT_EXPORT View {
		std::unique_ptr<mint::internal::IteratorViewData> _data;
	public:
		using value_type = Reference;
		using reference = value_type&;
		using pointer = value_type*;
		using const_reference = value_type&;
		using const_pointer = value_type*;

		struct sentinel {};

		class MINT_EXPORT const_iterator {
			mint::internal::IteratorViewData* _data = nullptr;
		public:
			explicit const_iterator(mint::internal::IteratorViewData* data);
			const_iterator(const_iterator&& other) noexcept = default;
			const_iterator(const const_iterator& other) = default;
			const_iterator() = default;
			~const_iterator() = default;

			const_iterator& operator=(const_iterator&& other) noexcept = default;
			const_iterator& operator=(const const_iterator& other) = default;

			bool operator==(const const_iterator& other) const;
			bool operator!=(const const_iterator& other) const;

			MINT_EXPORT friend bool operator==(const const_iterator& self, const sentinel& other);
			MINT_EXPORT friend bool operator==(const sentinel& self, const const_iterator& other);
			MINT_EXPORT friend bool operator!=(const const_iterator& self, const sentinel& other);
			MINT_EXPORT friend bool operator!=(const sentinel& self, const const_iterator& other);

			const_reference operator*() const;
			const_pointer operator->() const;

			const_iterator operator++(int);
			const_iterator& operator++();

			const_iterator operator--(int);
			const_iterator& operator--();
		};

		class MINT_EXPORT iterator {
			mint::internal::IteratorViewData* _data = nullptr;
		public:
			explicit iterator(mint::internal::IteratorViewData* data);
			iterator(iterator&& other) noexcept = default;
			iterator(const iterator& other) = default;
			iterator() = default;
			~iterator() = default;

			iterator& operator=(iterator&& other) noexcept = default;
			iterator& operator=(const iterator& other) = default;

			bool operator==(const iterator& other) const;
			bool operator!=(const iterator& other) const;

			MINT_EXPORT friend bool operator==(const iterator& self, const sentinel& other);
			MINT_EXPORT friend bool operator==(const sentinel& self, const iterator& other);
			MINT_EXPORT friend bool operator!=(const iterator& self, const sentinel& other);
			MINT_EXPORT friend bool operator!=(const sentinel& self, const iterator& other);

			reference operator*() const;
			pointer operator->() const;

			iterator operator++(int);
			iterator& operator++();

			iterator operator--(int);
			iterator& operator--();
		};

		explicit View(std::unique_ptr<mint::internal::IteratorViewData>&& data);
		View(const View&) = delete;
		View(View&& other) noexcept;
		~View();

		View& operator=(const View&) = delete;
		View& operator=(View&& other) noexcept;

		[[nodiscard]] const_iterator cbegin() const {
			return const_iterator {_data.get()};
		}

		[[nodiscard]] const_iterator begin() const {
			return const_iterator {_data.get()};
		}

		[[nodiscard]] iterator begin() {
			return iterator {_data.get()};
		}

		[[nodiscard]] sentinel cend() const {
			return sentinel {};
		}

		[[nodiscard]] sentinel end() const {
			return sentinel {};
		}

		[[nodiscard]] sentinel end() {
			return sentinel {};
		}

		[[nodiscard]] reference front();
		[[nodiscard]] reference back();
	};

	class MINT_EXPORT Context {
	public:
		enum Type : std::uint8_t {
			items,
			range,
			generator
		};

		using value_type = Reference;
		using reference = value_type&;
		using pointer = value_type*;

		struct sentinel {};

		class MINT_EXPORT iterator {
			mint::internal::IteratorData* _data = nullptr;
		public:
			explicit iterator(mint::internal::IteratorData* data);
			iterator(iterator&&) = default;
			iterator(const iterator&) = default;
			iterator() = default;
			~iterator() = default;

			iterator& operator=(iterator&&) = default;
			iterator& operator=(const iterator&) = default;

			bool operator==(const iterator& other) const;
			bool operator!=(const iterator& other) const;

			MINT_EXPORT friend bool operator==(const iterator& self, const sentinel& other);
			MINT_EXPORT friend bool operator==(const sentinel& self, const iterator& other);
			MINT_EXPORT friend bool operator!=(const iterator& self, const sentinel& other);
			MINT_EXPORT friend bool operator!=(const sentinel& self, const iterator& other);

			reference operator*() const;
			pointer operator->() const;

			iterator operator++(int);
			iterator& operator++();
		};

		explicit Context(std::unique_ptr<mint::internal::IteratorData>&& data);
		Context(const Context& other);
		Context(Context&& other) noexcept;
		~Context();

		Context& operator=(Context&& other) noexcept;
		Context& operator=(const Context& other);

		[[nodiscard]] iterator cbegin() const;
		[[nodiscard]] iterator begin() const;
		[[nodiscard]] iterator begin();

		[[nodiscard]] sentinel cend() const;
		[[nodiscard]] sentinel end() const;
		[[nodiscard]] sentinel end();

		[[nodiscard]] View view() const;

		void mark();

		[[nodiscard]] Type get_type() const;
		[[nodiscard]] reference get();
		[[nodiscard]] std::size_t size() const;
		[[nodiscard]] bool empty() const;

		[[nodiscard]] std::size_t capacity() const;
		void reserve(std::size_t capacity);

		void yield(value_type&& value);
		void next();

		void finalize();
		void clear();

	private:
		std::unique_ptr<mint::internal::IteratorData> _data;
	};

	Context ctx;

private:
	static LocalPool<Iterator> g_pool;
};

MINT_EXPORT void iterator_new(Cursor& cursor, std::size_t length);
MINT_EXPORT void iterator_yield(Iterator& iterator, Reference&& item);
MINT_EXPORT std::optional<WeakReference> iterator_get(Iterator& iterator);
MINT_EXPORT std::optional<WeakReference> iterator_next(Iterator& iterator);

}

template<>
struct std::iterator_traits<mint::Iterator::View::const_iterator> {
	using iterator_category = std::bidirectional_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using container_type = mint::Iterator::View;
	using value_type = container_type::value_type;
	using pointer = container_type::const_pointer;
	using reference = container_type::const_reference;
};

static_assert(std::bidirectional_iterator<mint::Iterator::View::const_iterator>);

template<>
struct std::iterator_traits<mint::Iterator::View::iterator> {
	using iterator_category = std::bidirectional_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using container_type = mint::Iterator::View;
	using value_type = container_type::value_type;
	using pointer = container_type::pointer;
	using reference = container_type::reference;
};

static_assert(std::bidirectional_iterator<mint::Iterator::View::iterator>);

template<>
struct std::iterator_traits<mint::Iterator::Context::iterator> {
	using iterator_category = std::input_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using container_type = mint::Iterator::Context;
	using value_type = container_type::value_type;
	using pointer = container_type::pointer;
	using reference = container_type::reference;
};

static_assert(std::input_iterator<mint::Iterator::View::iterator>);

#endif // MINT_MEMORY_BUILTIN_ITERATOR_H
