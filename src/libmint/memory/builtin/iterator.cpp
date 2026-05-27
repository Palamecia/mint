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

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "mint/memory/builtin/iterator.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/algorithm.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/system/error.h"
#include "mint/scheduler/scheduler.h"

#include "iterator_async_generator.h"
#include "iterator_generator.h"
#include "iterator_items.h"
#include "iterator_range.h"

using namespace mint;

IteratorClass& IteratorClass::instance(AbstractSyntaxTree& ast) {
	return ast.global_data().builtin<IteratorClass>(Class::Metatype::iterator);
}

AsyncIteratorClass& AsyncIteratorClass::instance(AbstractSyntaxTree& ast) {
	return ast.global_data().builtin<AsyncIteratorClass>(Class::Metatype::async_iterator);
}

Iterator::Iterator(AbstractSyntaxTree& ast) :
    Object(IteratorClass::instance(ast)),
    ctx(std::make_unique<mint::internal::ItemsIteratorData>()) {}

Iterator::Iterator(Cursor& cursor, const Reference& ref) :
    Object(IteratorClass::instance(cursor.ast())),
    ctx(std::make_unique<mint::internal::ItemsIteratorData>(cursor, ref)) {}

Iterator::Iterator(Cursor& cursor, Reference&& ref) :
    Object(IteratorClass::instance(cursor.ast())),
    ctx(std::make_unique<mint::internal::ItemsIteratorData>(cursor, std::move(ref))) {}

Iterator::Iterator(AbstractSyntaxTree& ast, std::size_t capacity) :
    Object(IteratorClass::instance(ast)),
    ctx(std::make_unique<mint::internal::ItemsIteratorData>(capacity)) {}

Iterator::Iterator(AbstractSyntaxTree& ast, std::unique_ptr<mint::internal::IteratorData>&& data) :
    Object(IteratorClass::instance(ast)),
    ctx(std::move(data)) {}

Iterator::Iterator(const Iterator& other) :
    Object(other.metadata),
    ctx(other.ctx) {}

Iterator::Iterator(Iterator&& other) noexcept :
    Object(other.metadata),
    ctx(std::move(other.ctx)) {}

Iterator::Iterator(FromGenerator /*from_generator*/, AbstractSyntaxTree& ast, std::size_t stack_size) :
    Iterator(ast, std::make_unique<mint::internal::GeneratorData>(stack_size)) {}

Iterator::Iterator(FromAsyncGenerator /*from_async_generator*/, AbstractSyntaxTree& ast, Coroutine& coroutine,
    std::size_t stack_size) :
    Object(AsyncIteratorClass::instance(ast)),
    ctx(std::make_unique<mint::internal::AsyncGeneratorData>(coroutine, stack_size)) {}

Iterator::Iterator(FromInclusiveRange /*from_inclusive_range*/, AbstractSyntaxTree& ast, double begin, double end) :
    Iterator(ast, std::make_unique<mint::internal::RangeIteratorData>(begin, begin <= end ? end + 1 : end - 1)) {}

Iterator::Iterator(FromExclusiveRange /*from_exclusive_range*/, AbstractSyntaxTree& ast, double begin, double end) :
    Iterator(ast, std::make_unique<mint::internal::RangeIteratorData>(begin, end)) {}

Iterator& Iterator::operator=(const Iterator& other) {
	if (this == &other) [[unlikely]] {
		return *this;
	}
	assert(&metadata == &other.metadata);
	ctx = other.ctx;
	return *this;
}

Iterator& Iterator::operator=(Iterator&& other) noexcept {
	assert(&metadata == &other.metadata);
	ctx = std::move(other.ctx);
	return *this;
}

void Iterator::mark() {
	if (marked_bit()) {
		return;
	}
	Object::mark();
	ctx.mark();
}

IteratorClass::IteratorClass(AbstractSyntaxTree& ast) :
    Class(ast.global_data(), "iterator", Class::Metatype::iterator) {

	create_builtin_member(copy_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		auto it = self.data<Iterator>().ctx.begin();
		const auto end = self.data<Iterator>().ctx.end();

		for_each_if(cursor, other, [&it, &end](const Reference& item) -> bool {
			if (it != end) {
				if ((it->flags() & Reference::const_address) && (it->data().format() != Data::Format::none))
				    [[unlikely]] {
					error("invalid modification of constant reference");
				}

				it->move_data(item);
				++it;
				return true;
			}

			return false;
		});

		cursor.stack().pop_back();
	}));

	create_builtin_member("next", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		const auto self = std::move(cursor.stack().back());

		if (!self.data<Iterator>().ctx.empty()) {
			cursor.stack().back() = Reference(create_from, self.data<Iterator>().ctx.get());
			// The next call can interrupt the current context,
			// so the value must be pushed first
			self.data<Iterator>().ctx.next(cursor);
		}
		else {
			cursor.stack().back() = create_none();
		}
	}));

	create_builtin_member("value", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		if (std::optional<Reference>&& result = iterator_get(cursor.stack().back().data<Iterator>())) {
			cursor.stack().back() = std::move(*result);
		}
		else {
			cursor.stack().back() = create_none();
		}
	}));

	create_builtin_member("isEmpty", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		cursor.stack().back() = create_boolean(cursor.stack().back().data<Iterator>().ctx.empty());
	}));

	create_builtin_member("each", ast.create_builtin_method(*this, 2, R"""(
		def (self, const func) {
			for let item in self {
				func(item)
			}
		})"""));

	/// \todo register operator overloads
}

AsyncIteratorClass::AsyncIteratorClass(AbstractSyntaxTree& ast) :
    Class(ast.global_data(), "async_iterator", Class::Metatype::async_iterator) {

	create_builtin_member(copy_operator, ast.create_builtin_async_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		auto it = self.data<Iterator>().ctx.begin();
		const auto end = self.data<Iterator>().ctx.end();

		for_each_if(cursor, other, [&it, &end](const Reference& item) -> bool {
			if (it != end) {
				if ((it->flags() & Reference::const_address) && (it->data().format() != Data::Format::none))
				    [[unlikely]] {
					error("invalid modification of constant reference");
				}

				it->move_data(item);
				++it;
				return true;
			}

			return false;
		});

		cursor.stack().pop_back();
	}));

	create_builtin_member("next", ast.create_builtin_async_method(*this, 1, [](Cursor& cursor) {
		const auto self = std::move(cursor.stack().back());

		if (!self.data<Iterator>().ctx.empty()) {
			cursor.stack().back() = Reference(create_from, self.data<Iterator>().ctx.get());
			// The next call can interrupt the current context,
			// so the value must be pushed first
			self.data<Iterator>().ctx.next(cursor);
		}
		else {
			cursor.stack().back() = create_none();
		}
	}));

	create_builtin_member("value", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		if (std::optional<Reference>&& result = iterator_get(cursor.stack().back().data<Iterator>())) {
			cursor.stack().back() = std::move(*result);
		}
		else {
			cursor.stack().back() = create_none();
		}
	}));

	create_builtin_member("isEmpty", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		cursor.stack().back() = create_boolean(cursor.stack().back().data<Iterator>().ctx.empty());
	}));

	create_builtin_member("each", ast.create_builtin_method(*this, 2, R"""(
		async def (self, const func) {
			for let item in await self {
				func(item)
			}
		})"""));

	/// \todo register operator overloads
}

Iterator::View::const_iterator::const_iterator(mint::internal::IteratorViewData* data) :
    _data(data) {}

bool Iterator::View::const_iterator::operator==(const const_iterator& other) const {
	return _data == other._data;
}

bool Iterator::View::const_iterator::operator!=(const const_iterator& other) const {
	return _data != other._data;
}

bool mint::operator==(const Iterator::View::const_iterator& self, const Iterator::View::sentinel& /*other*/) {
	return self._data->empty();
}

bool mint::operator==(const Iterator::View::sentinel& /*self*/, const Iterator::View::const_iterator& other) {
	return other._data->empty();
}

bool mint::operator!=(const Iterator::View::const_iterator& self, const Iterator::View::sentinel& /*other*/) {
	return !self._data->empty();
}

bool mint::operator!=(const Iterator::View::sentinel& /*self*/, const Iterator::View::const_iterator& other) {
	return !other._data->empty();
}

Iterator::View::reference Iterator::View::const_iterator::operator*() const {
	return _data->get();
}

Iterator::View::pointer Iterator::View::const_iterator::operator->() const {
	return &_data->get();
}

Iterator::View::const_iterator Iterator::View::const_iterator::operator++(int) {
	_data->next();
	return Iterator::View::const_iterator {_data};
}

Iterator::View::const_iterator& Iterator::View::const_iterator::operator++() {
	_data->next();
	return *this;
}

Iterator::View::const_iterator Iterator::View::const_iterator::operator--(int) {
	_data->prev();
	return Iterator::View::const_iterator {_data};
}

Iterator::View::const_iterator& Iterator::View::const_iterator::operator--() {
	_data->prev();
	return *this;
}

Iterator::View::View(std::unique_ptr<mint::internal::IteratorViewData>&& data) :
    _data(std::move(data)) {}

Iterator::View::View(View&& other) noexcept = default;

Iterator::View::~View() {}

Iterator::View& Iterator::View::operator=(View&& other) noexcept = default;

Iterator::View::reference Iterator::View::front() {
	return _data->front();
}

Iterator::View::reference Iterator::View::back() {
	return _data->back();
}

Iterator::Context::const_iterator::const_iterator(mint::internal::IteratorData* data) :
    _data(data) {}

bool Iterator::Context::const_iterator::operator==(const const_iterator& other) const {
	return _data == other._data;
}

bool Iterator::Context::const_iterator::operator!=(const const_iterator& other) const {
	return _data != other._data;
}

bool mint::operator==(const Iterator::Context::const_iterator& self, const Iterator::Context::sentinel& /*other*/) {
	return self._data->empty();
}

bool mint::operator==(const Iterator::Context::sentinel& /*self*/, const Iterator::Context::const_iterator& other) {
	return other._data->empty();
}

bool mint::operator!=(const Iterator::Context::const_iterator& self, const Iterator::Context::sentinel& /*other*/) {
	return !self._data->empty();
}

bool mint::operator!=(const Iterator::Context::sentinel& /*self*/, const Iterator::Context::const_iterator& other) {
	return !other._data->empty();
}

Iterator::Context::reference Iterator::Context::const_iterator::operator*() const {
	return _data->get();
}

Iterator::Context::pointer Iterator::Context::const_iterator::operator->() const {
	return &_data->get();
}

Iterator::Context::const_iterator Iterator::Context::const_iterator::operator++(int) {
	_data->next(Scheduler::current_process()->cursor());
	return Iterator::Context::const_iterator {_data};
}

Iterator::Context::const_iterator& Iterator::Context::const_iterator::operator++() {
	_data->next(Scheduler::current_process()->cursor());
	return *this;
}

Iterator::Context::Context(std::unique_ptr<mint::internal::IteratorData>&& data) :
    _data(std::move(data)) {}

Iterator::Context::Context(const Context& other) :
    _data(other._data->copy()) {}

Iterator::Context::Context(Context&& other) noexcept :
    _data(std::move(other._data)) {}

Iterator::Context::~Context() {}

Iterator::Context& Iterator::Context::operator=(Context&& other) noexcept {
	_data = std::move(other._data);
	return *this;
}

Iterator::Context& Iterator::Context::operator=(const Context& other) {
	if (this == &other) [[unlikely]] {
		return *this;
	}
	_data = other._data->copy();
	return *this;
}

Iterator::Context::const_iterator Iterator::Context::cbegin() const {
	return Iterator::Context::const_iterator {_data.get()};
}

Iterator::Context::const_iterator Iterator::Context::begin() const {
	return Iterator::Context::const_iterator {_data.get()};
}

Iterator::Context::sentinel Iterator::Context::cend() const {
	return Iterator::Context::sentinel {};
}

Iterator::Context::sentinel Iterator::Context::end() const {
	return Iterator::Context::sentinel {};
}

Iterator::View Iterator::Context::view() const {
	return Iterator::View {_data->view()};
}

void Iterator::Context::mark() {
	_data->mark();
}

Iterator::Context::Type Iterator::Context::get_type() const {
	return _data->get_type();
}

Iterator::Context::reference Iterator::Context::get() {
	return _data->get();
}

std::size_t Iterator::Context::size() const {
	return _data->size();
}

bool Iterator::Context::empty() const {
	return _data->empty();
}

std::size_t Iterator::Context::capacity() const {
	return _data->capacity();
}

void Iterator::Context::reserve(std::size_t capacity) {
	_data->reserve(capacity);
}

void Iterator::Context::yield(Cursor& cursor, value_type&& value, Iterator::ResumeKind resume_kind) {
	_data->yield(cursor, std::move(value), resume_kind);
}

void Iterator::Context::next(Cursor& cursor) {
	_data->next(cursor);
}

void Iterator::Context::finalize(Cursor& cursor) {
	_data->finalize(cursor);
}

void Iterator::Context::clear() {
	_data->clear();
}

bool mint::is_iterator(const Reference& ref) {
	return ref.data().format() == Data::Format::object
	       && (ref.data<Object>().metadata.metatype() == Class::Metatype::iterator
	           || ref.data<Object>().metadata.metatype() == Class::Metatype::async_iterator);
}

void mint::iterator_new(Cursor& cursor, std::size_t length) {

	auto& stack = cursor.stack();

	Cursor::Call call = std::move(cursor.waiting_calls().top());
	cursor.waiting_calls().pop();

	auto& self = call.function().data<Iterator>();
	self.ctx.reserve(length + call.extra_argument_count());
	self.construct();

	const auto from = std::prev(stack.end(),
	    static_cast<std::vector<Reference>::difference_type>(length + call.extra_argument_count()));
	const auto to = stack.end();
	for (auto it = from; it != to; ++it) {
		iterator_yield(cursor, self, std::move(*it));
	}

	stack.erase(from, to);
	stack.emplace_back(std::move(call.function()));
}

void mint::iterator_yield(Cursor& cursor, Iterator& iterator, Reference&& item) {
	iterator.ctx.yield(cursor, std::move(item), Iterator::ResumeKind::next);
}

void mint::iterator_return(Cursor& cursor, Iterator& iterator, Reference&& item) {
	iterator.ctx.yield(cursor, std::move(item), Iterator::ResumeKind::close);
	cursor.exit_call();
}

void mint::iterator_resume(Cursor& cursor, Iterator& iterator, Reference&& item) {

	assert(cursor.is_in_coroutine());

	const mint::Reference coroutine = cursor.coroutine();
	iterator.ctx.yield(cursor, std::move(item), Iterator::ResumeKind::close);
	coroutine.data<Coroutine>().exit(cursor);
}

std::optional<Reference> mint::iterator_get(Iterator& iterator) {
	if (!iterator.ctx.empty()) {
		return iterator.ctx.get();
	}
	return std::nullopt;
}

std::optional<Reference> mint::iterator_next(Cursor& cursor, Iterator& iterator) {
	if (!iterator.ctx.empty()) {
		std::optional<Reference> item(iterator.ctx.get());
		iterator.ctx.next(cursor);
		return item;
	}
	return std::nullopt;
}
