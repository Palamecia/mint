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

#include "mint/memory/builtin/iterator.h"
#include "mint/memory/algorithm.hpp"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/error.h"

#include "iterator_items.h"
#include "iterator_range.h"
#include "iterator_generator.h"

using namespace mint;
using namespace std;

IteratorClass *IteratorClass::instance() {
	return GlobalData::instance()->builtin<IteratorClass>(Class::iterator);
}

Iterator::Iterator() : Object(IteratorClass::instance()),
	ctx(new _mint_iterator::items_data) {

}

Iterator::Iterator(Reference &ref) : Object(IteratorClass::instance()),
	ctx(new _mint_iterator::items_data(ref)) {

}

Iterator::Iterator(const Iterator &other) : Object(IteratorClass::instance()),
	ctx(other.ctx) {

}

Iterator::Iterator(double begin, double end) : Object(IteratorClass::instance()),
	ctx(new _mint_iterator::range_data(begin, end)) {

}

Iterator::Iterator(size_t stack_size) : Object((IteratorClass::instance())),
	ctx(new _mint_iterator::generator_data(stack_size)) {

}

void Iterator::mark() {
	if (!marked_bit()) {
		Object::mark();
		ctx.mark();
	}
}

WeakReference Iterator::fromInclusiveRange(double begin, double end) {
	WeakReference iterator = WeakReference::create(GarbageCollector::instance().alloc<Iterator>(begin, begin <= end ? end + 1 : end - 1));
	iterator.data<Iterator>()->construct();
	return iterator;
}

WeakReference Iterator::fromExclusiveRange(double begin, double end) {
	WeakReference iterator = WeakReference::create(GarbageCollector::instance().alloc<Iterator>(begin, end));
	iterator.data<Iterator>()->construct();
	return iterator;
}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();
	
	create_builtin_member(copy_operator, ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							Iterator::ctx_type::iterator it = self.data<Iterator>()->ctx.begin();
							const Iterator::ctx_type::iterator end = self.data<Iterator>()->ctx.end();

							for_each_if(other, [&it, &end] (const Reference &item) -> bool {
								if (it != end) {
									if (UNLIKELY((it->flags() & Reference::const_address) && (it->data()->format != Data::fmt_none))) {
										error("invalid modification of constant reference");
									}

									it->move(item);
									++it;
									return true;
								}

								return false;
							});

							cursor->stack().pop_back();
						}));
	
	create_builtin_member("next", ast->create_builtin_methode(this, 1, [] (Cursor *cursor) {

							WeakReference self = std::move(cursor->stack().back());

							if (!self.data<Iterator>()->ctx.empty()) {
								cursor->stack().back() = std::move(self.data<Iterator>()->ctx.next());
								// The next call can iterrupt the current context,
								// so the value must be pushed first
								self.data<Iterator>()->ctx.pop();
							}
							else {
								cursor->stack().back() = WeakReference::create<None>();
							}
						}));
	
	create_builtin_member("value", ast->create_builtin_methode(this, 1, [] (Cursor *cursor) {
							if (optional<WeakReference> &&result = iterator_get(cursor->stack().back().data<Iterator>())) {
								cursor->stack().back() = std::move(*result);
							}
							else {
								cursor->stack().back() = WeakReference::create<None>();
							}
						}));
	
	create_builtin_member("isEmpty", ast->create_builtin_methode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Boolean>(cursor->stack().back().data<Iterator>()->ctx.empty());
						}));
	
	create_builtin_member("each", ast->create_builtin_methode(this, 2, R"""(
						def (const self, const func) {
							for item in self {
								func(item)
							}
						})"""));

	/// \todo register operator overloads
}

Iterator::ctx_type::iterator::iterator(_mint_iterator::data_iterator *data) :
	m_data(data) {

}

Iterator::ctx_type::iterator::iterator(const iterator &other) :
	m_data(other.m_data->copy()) {

}

Iterator::ctx_type::iterator::iterator(iterator &&other) :
	m_data(std::forward<unique_ptr<_mint_iterator::data_iterator>>(other.m_data)) {

}

Iterator::ctx_type::iterator::~iterator() {

}

Iterator::ctx_type::iterator &Iterator::ctx_type::iterator::operator =(const iterator &other) {
	m_data.reset(other.m_data->copy());
	return *this;
}

Iterator::ctx_type::iterator &Iterator::ctx_type::iterator::operator =(iterator &&other) {
	swap(m_data, other.m_data);
	return *this;
}

void Iterator::ctx_type::mark() {
	m_data->mark();
}

Iterator::ctx_type::type Iterator::ctx_type::getType() const {
	return m_data->getType();
}

bool Iterator::ctx_type::iterator::operator ==(const iterator &other) const {
	return !m_data->compare(other.m_data.get());
}

bool Iterator::ctx_type::iterator::operator !=(const iterator &other) const {
	return m_data->compare(other.m_data.get());
}

Iterator::ctx_type::value_type &Iterator::ctx_type::iterator::operator *() const {
	return m_data->get();
}

Iterator::ctx_type::value_type *Iterator::ctx_type::iterator::operator ->() const {
	return &m_data->get();
}

Iterator::ctx_type::iterator Iterator::ctx_type::iterator::operator ++(int) {
	iterator tmp(m_data->copy());
	m_data->next();
	return tmp;
}

Iterator::ctx_type::iterator &Iterator::ctx_type::iterator::operator ++() {
	m_data->next();
	return *this;
}

Iterator::ctx_type::ctx_type(_mint_iterator::data *data) :
	m_data(data) {

}

Iterator::ctx_type::ctx_type(const ctx_type &other) :
	m_data(other.m_data->copy()) {

}

Iterator::ctx_type::~ctx_type() {

}

Iterator::ctx_type &Iterator::ctx_type::operator =(const ctx_type &other) {
	m_data.reset(other.m_data->copy());
	return *this;
}

Iterator::ctx_type::iterator Iterator::ctx_type::begin() const {
	return iterator(m_data->begin());
}

Iterator::ctx_type::iterator Iterator::ctx_type::end() const {
	return iterator(m_data->end());
}

Iterator::ctx_type::value_type &Iterator::ctx_type::next() {
	return m_data->next();
}

Iterator::ctx_type::value_type &Iterator::ctx_type::back() {
	return m_data->back();
}

void Iterator::ctx_type::emplace(value_type &&value) {
	m_data->emplace(std::forward<Reference>(value));
}

void Iterator::ctx_type::pop() {
	m_data->pop();
}

void Iterator::ctx_type::finalize() {
	m_data->finalize();
}

void Iterator::ctx_type::clear() {
	m_data->clear();
}

size_t Iterator::ctx_type::size() const {
	return m_data->size();
}

bool Iterator::ctx_type::empty() const {
	return m_data->empty();
}

void mint::iterator_new(Cursor *cursor, size_t length) {

	auto &stack = cursor->stack();

	Cursor::Call call = std::move(cursor->waiting_calls().top());
	cursor->waiting_calls().pop();

	Iterator *self = call.function().data<Iterator>();
	self->construct();

	const auto from = std::prev(stack.end(), length + call.extra_argument_count());
	const auto to = stack.end();
	for (auto it = from; it != to; ++it) {
		iterator_insert(self, std::move(*it));
	}

	stack.erase(from, to);
	stack.emplace_back(std::move(call.function()));
}

Iterator *mint::iterator_init(Reference &ref) {

	if (ref.data()->format == Data::fmt_object && ref.data<Object>()->metadata->metatype() == Class::iterator) {
		return ref.data<Iterator>();
	}

	Iterator *iterator = new Iterator(ref);
	iterator->construct();
	return iterator;
}

Iterator *mint::iterator_init(Reference &&ref) {
	return iterator_init(static_cast<Reference &>(ref));
}

void mint::iterator_insert(Iterator *iterator, Reference &&item) {
	iterator->ctx.emplace(std::forward<Reference>(item));
}

optional<WeakReference> mint::iterator_get(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		return optional<WeakReference>(WeakReference::share(iterator->ctx.next()));
	}

	return nullopt;
}

optional<WeakReference> mint::iterator_next(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		optional<WeakReference> item(WeakReference::share(iterator->ctx.next()));
		iterator->ctx.pop();
		return item;
	}

	return nullopt;
}
