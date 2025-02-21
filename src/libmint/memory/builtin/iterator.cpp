/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#include <cstddef>
#include <utility>
#include <vector>

#include "mint/memory/builtin/iterator.h"
#include "memory/reference.h"
#include "mint/memory/algorithm.hpp"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/error.h"

#include "iterator_items.h"
#include "iterator_range.h"
#include "iterator_generator.h"

using namespace mint;

IteratorClass *IteratorClass::instance() {
	return GlobalData::instance()->builtin<IteratorClass>(Class::ITERATOR);
}

Iterator::Iterator() :
	Object(IteratorClass::instance()),
	ctx(new mint::internal::ItemsIteratorData) {}

Iterator::Iterator(size_t capacity) :
	Object(IteratorClass::instance()),
	ctx(new mint::internal::ItemsIteratorData(capacity)) {}

Iterator::Iterator(Reference &ref) :
	Object(IteratorClass::instance()),
	ctx(new mint::internal::ItemsIteratorData(ref)) {}

Iterator::Iterator(Reference &&ref) :
	Object(IteratorClass::instance()),
	ctx(new mint::internal::ItemsIteratorData(std::move(ref))) {}

Iterator::Iterator(mint::internal::IteratorData *data) :
	Object(IteratorClass::instance()),
	ctx(data) {}

Iterator::Iterator(const Iterator &other) :
	Object(IteratorClass::instance()),
	ctx(other.ctx) {}

Iterator::Iterator(Iterator &&other) noexcept :
	Object(IteratorClass::instance()),
	ctx(std::move(other.ctx)) {}

Iterator &Iterator::operator=(const Iterator &other) {
	ctx = other.ctx;
	return *this;
}

Iterator &Iterator::operator=(Iterator &&other) noexcept {
	ctx = std::move(other.ctx);
	return *this;
}

Iterator *Iterator::from_generator(size_t stack_size) {
	return GarbageCollector::instance().alloc<Iterator>(new mint::internal::GeneratorData(stack_size));
}

Iterator *Iterator::from_inclusive_range(double begin, double end) {
	return GarbageCollector::instance().alloc<Iterator>(
		new mint::internal::RangeIteratorData(begin, begin <= end ? end + 1 : end - 1));
}

Iterator *Iterator::from_exclusive_range(double begin, double end) {
	return GarbageCollector::instance().alloc<Iterator>(new mint::internal::RangeIteratorData(begin, end));
}

void Iterator::mark() {
	if (!marked_bit()) {
		Object::mark();
		ctx.mark();
	}
}

IteratorClass::IteratorClass() :
	Class("iterator", Class::ITERATOR) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();

	create_builtin_member(COPY_OPERATOR, ast->create_builtin_method(this, 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		Reference &other = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		Iterator::Context::iterator it = self.data<Iterator>()->ctx.begin();
		const Iterator::Context::iterator end = self.data<Iterator>()->ctx.end();

		for_each_if(other, [&it, &end](const Reference &item) -> bool {
			if (it != end) {
				if (UNLIKELY((it->flags() & Reference::CONST_ADDRESS) && (it->data()->format != Data::FMT_NONE))) {
					error("invalid modification of constant reference");
				}

				it->move_data(item);
				++it;
				return true;
			}

			return false;
		});

		cursor->stack().pop_back();
	}));

	create_builtin_member("next", ast->create_builtin_method(this, 1, [](Cursor *cursor) {
		WeakReference self = std::move(cursor->stack().back());

		if (!self.data<Iterator>()->ctx.empty()) {
			cursor->stack().back() = std::move(self.data<Iterator>()->ctx.value());
			// The next call can interrupt the current context,
			// so the value must be pushed first
			self.data<Iterator>()->ctx.next();
		}
		else {
			cursor->stack().back() = WeakReference::create<None>();
		}
	}));

	create_builtin_member("value", ast->create_builtin_method(this, 1, [](Cursor *cursor) {
		if (std::optional<WeakReference> &&result = iterator_get(cursor->stack().back().data<Iterator>())) {
			cursor->stack().back() = std::move(*result);
		}
		else {
			cursor->stack().back() = WeakReference::create<None>();
		}
	}));

	create_builtin_member("isEmpty", ast->create_builtin_method(this, 1, [](Cursor *cursor) {
		cursor->stack().back() = WeakReference::create<Boolean>(cursor->stack().back().data<Iterator>()->ctx.empty());
	}));

	create_builtin_member("each", ast->create_builtin_method(this, 2, R"""(
		def (const self, const func) {
			for item in self {
				func(item)
			}
		})"""));

	/// \todo register operator overloads
}

Iterator::Context::iterator::iterator(Context *context) :
	m_context(context) {}

bool Iterator::Context::iterator::operator==(const iterator &other) const {
	return m_context == other.m_context;
}

bool Iterator::Context::iterator::operator!=(const iterator &other) const {
	return m_context != other.m_context;
}

Iterator::Context::value_type &Iterator::Context::iterator::operator*() const {
	return m_context->value();
}

Iterator::Context::value_type *Iterator::Context::iterator::operator->() const {
	return &m_context->value();
}

Iterator::Context::iterator Iterator::Context::iterator::operator++(int) {
	m_context->next();
	if (m_context->empty()) {
		m_context = nullptr;
	}
	return Iterator::Context::iterator {m_context};
}

Iterator::Context::iterator &Iterator::Context::iterator::operator++() {
	m_context->next();
	if (m_context->empty()) {
		m_context = nullptr;
	}
	return *this;
}

Iterator::Context::Context(mint::internal::IteratorData *data) :
	m_data(data) {}

Iterator::Context::Context(const Context &other) :
	m_data(other.m_data->copy()) {}

Iterator::Context::Context(Context &&other) noexcept :
	m_data(std::move(other.m_data)) {}

Iterator::Context::~Context() {}

Iterator::Context &Iterator::Context::operator=(Context &&other) noexcept {
	m_data = std::move(other.m_data);
	return *this;
}

Iterator::Context &Iterator::Context::operator=(const Context &other) {
	m_data.reset(other.m_data->copy());
	return *this;
}

Iterator::Context::iterator Iterator::Context::begin() {
	return Iterator::Context::iterator {m_data->empty() ? nullptr : this};
}

Iterator::Context::iterator Iterator::Context::end() {
	return Iterator::Context::iterator {nullptr};
}

void Iterator::Context::mark() {
	m_data->mark();
}

Iterator::Context::Type Iterator::Context::get_type() const {
	return m_data->get_type();
}

Iterator::Context::value_type &Iterator::Context::value() {
	return m_data->value();
}

Iterator::Context::value_type &Iterator::Context::last() {
	return m_data->last();
}

size_t Iterator::Context::size() const {
	return m_data->size();
}

bool Iterator::Context::empty() const {
	return m_data->empty();
}

size_t Iterator::Context::capacity() const {
	return m_data->capacity();
}

void Iterator::Context::reserve(size_t capacity) {
	m_data->reserve(capacity);
}

void Iterator::Context::yield(value_type &&value) {
	m_data->yield(std::move(value));
}

void Iterator::Context::next() {
	m_data->next();
}

void Iterator::Context::finalize() {
	m_data->finalize();
}

void Iterator::Context::clear() {
	m_data->clear();
}

void mint::iterator_new(Cursor *cursor, size_t length) {

	auto &stack = cursor->stack();

	Cursor::Call call = std::move(cursor->waiting_calls().top());
	cursor->waiting_calls().pop();

	auto *self = call.function().data<Iterator>();
	self->ctx.reserve(length + call.extra_argument_count());
	self->construct();

	const auto from = std::prev(stack.end(), static_cast<std::vector<WeakReference>::difference_type>(
												 length + call.extra_argument_count()));
	const auto to = stack.end();
	for (auto it = from; it != to; ++it) {
		iterator_yield(self, std::move(*it));
	}

	stack.erase(from, to);
	stack.emplace_back(std::move(call.function()));
}

Iterator *mint::iterator_init(Reference &ref) {

	if (ref.data()->format == Data::FMT_OBJECT && ref.data<Object>()->metadata->metatype() == Class::ITERATOR) {
		return ref.data<Iterator>();
	}

	auto *iterator = new Iterator(ref);
	iterator->construct();
	return iterator;
}

Iterator *mint::iterator_init(Reference &&ref) {
	
	if (ref.data()->format == Data::FMT_OBJECT && ref.data<Object>()->metadata->metatype() == Class::ITERATOR) {
		return ref.data<Iterator>();
	}

	auto *iterator = new Iterator(std::move(ref));
	iterator->construct();
	return iterator;
}

void mint::iterator_yield(Iterator *iterator, Reference &&item) {
	iterator->ctx.yield(std::move(item));
}

std::optional<WeakReference> mint::iterator_get(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		return {WeakReference::share(iterator->ctx.value())};
	}

	return std::nullopt;
}

std::optional<WeakReference> mint::iterator_next(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		std::optional<WeakReference> item(WeakReference::share(iterator->ctx.value()));
		iterator->ctx.next();
		return item;
	}

	return std::nullopt;
}
