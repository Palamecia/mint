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

#include "mint/memory/object.h"
#include "mint/ast/module.h"
#include "mint/ast/savedstate.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/reference.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/ast/cursor.h"
#include "mint/memory/symboltable.h"
#include "mint/system/error.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace mint;

std::allocator<Reference> Object::g_allocator;

Number::Number(double value) :
    value(value) {}

Number::Number(std::intmax_t value) :
    value(static_cast<double>(value)) {}

Number::Number(std::uintmax_t value) :
    value(static_cast<double>(value)) {}

bool mint::is_integer(const Reference& ref) {
	return ref.data().format() == Data::Format::number
	       && std::floor(ref.data<Number>().value) == ref.data<Number>().value;
}

Boolean::Boolean(bool value) :
    value(value) {}

Object::Object(Class& type) :
    metadata(type) {}

Object::~Object() {
	destroy();
}

void Object::construct() {
	data = g_allocator.allocate(std::max(1uz, metadata.size()));
	for (const Class::MemberInfo& member : metadata.slots()) {
		std::construct_at(std::next(data, static_cast<std::ptrdiff_t>(member.offset)), copy_from, member.value);
	}
}

void Object::construct(const Object& other) {
	std::unordered_map<const Data*, Data*> memory_map;
	memory_map.emplace(&other, this);
	construct(other, memory_map);
}

void Object::destroy() {
	if (data) {
		for (std::size_t offset = 0; offset < metadata.size(); ++offset) {
			std::destroy_at(std::next(data, static_cast<std::ptrdiff_t>(offset)));
		}
		g_allocator.deallocate(data, std::max(1uz, metadata.size()));
	}
}

void Object::construct(const Object& other, std::unordered_map<const Data*, Data*>& memory_map) {

	if (other.data) {

		if (!metadata.is_trivially_copyable()) [[unlikely]] {
			// TODO: try using a clone method if available before giving up
			error("type '{}' is not copyable", metadata.full_name());
		}

		data = g_allocator.allocate(std::max(1uz, metadata.size()));

		for (const auto& member : metadata.slots()) {

			const auto& target_ref = other.data[member.get().offset];
			Reference* member_ref = data + member.get().offset;
			auto i = memory_map.find(&target_ref.data());

			if (i == memory_map.end()) {
				if ((target_ref.flags() & (Reference::const_address | Reference::const_value))
				    != (Reference::const_address | Reference::const_value)) {
					switch (target_ref.data().format()) {
					case Data::Format::object:
						switch (target_ref.data<Object>().metadata.metatype()) {
						case Class::Metatype::object:
							member_ref = std::construct_at(member_ref, target_ref.flags(), std::in_place_type<Object>,
							    target_ref.data<Object>().metadata);
							break;
						case Class::Metatype::string:
							member_ref = std::construct_at(member_ref, target_ref.flags(), std::in_place_type<String>,
							    target_ref.data<String>());
							break;
						case Class::Metatype::regex:
							member_ref = std::construct_at(member_ref, target_ref.flags(), std::in_place_type<Regex>,
							    target_ref.data<Regex>());
							break;
						case Class::Metatype::array:
							member_ref = std::construct_at(member_ref, target_ref.flags(), std::in_place_type<Array>,
							    target_ref.data<Array>());
							break;
						case Class::Metatype::hash:
							member_ref = std::construct_at(member_ref, target_ref.flags(), std::in_place_type<Hash>,
							    target_ref.data<Hash>());
							break;
						case Class::Metatype::iterator:
						case Class::Metatype::async_iterator:
							member_ref = std::construct_at(member_ref, target_ref.flags(), std::in_place_type<Iterator>,
							    target_ref.data<Iterator>());
							break;
						case Class::Metatype::library:
							member_ref = std::construct_at(member_ref, target_ref.flags(), std::in_place_type<Library>,
							    target_ref.data<Library>());
							break;
						case Class::Metatype::libobject:
							member_ref = std::construct_at(member_ref, Reference(copy_from, target_ref));
							memory_map.emplace(&target_ref.data(), &member_ref->data());
							continue;
						}

						memory_map.emplace(&target_ref.data(), &member_ref->data());
						member_ref->data<Object>().construct(target_ref.data<Object>(), memory_map);
						break;

					default:
						member_ref = std::construct_at(member_ref, copy_from, target_ref);
						memory_map.emplace(&target_ref.data(), &member_ref->data());
						break;
					}
				}
				else {
					member_ref = std::construct_at(member_ref, target_ref);
					memory_map.emplace(&target_ref.data(), &member_ref->data());
				}
			}
			else {
				std::construct_at(member_ref, target_ref.flags(), *i->second);
			}
		}
	}
}

void Object::mark() {
	if (!marked_bit()) {
		Data::mark();
		if (data) {
			for (auto& slot : std::span(data, metadata.size())) {
				slot.data().mark();
			}
		}
	}
}

Package::Package(PackageData& package) :
    data(package) {}

Function::Function() = default;

Function::Function(Mapping mapping) :
    mapping(std::move(mapping)) {}

Function::Function(int signature, Signature&& handle) :
    mapping(signature, std::move(handle)) {}

Function::Function(const std::pair<int, Signature>& mapping) :
    mapping(mapping) {}

mint::Function::Function(const std::pair<int, Module::Handle&>& mapping) :
    mapping(mapping) {}

void Function::Stateless::call(int signature, Class* metadata, Cursor& cursor) {
	cursor.call(handle(), signature, metadata);
}

void Function::Stateful::call(int signature, Class* metadata, Cursor& cursor) {

	cursor.call(handle(), signature, metadata);

	for (SymbolTable& symbols = handle().async ? cursor.stack().back().data<Coroutine>().symbols() : cursor.symbols();
	    auto& item : _capture) {
		symbols.emplace(item.first, item.second);
	}
}

Function::Mapping::Mapping(int signature, Function::Signature&& handle) :
    _signatures({{signature, std::move(handle)}}) {}

Function::Mapping::Mapping(const std::pair<int, Signature>& mapping) :
    _signatures({mapping}) {}

Function::Mapping::Mapping(const std::pair<int, Module::Handle&>& mapping) :
    _signatures({{mapping.first, std::make_unique<Stateless>(mapping.second)}}) {}

bool Function::Mapping::operator==(const Mapping& other) const {

	if (_signatures.size() != other._signatures.size()) {
		return false;
	}

	for (auto it1 = _signatures.begin(), it2 = other._signatures.begin();
	    it1 != _signatures.end() && it2 != other._signatures.end(); ++it1, ++it2) {
		if (it1->first != it2->first || &it1->second.handle() != &it2->second.handle()) {
			return false;
		}
	}

	return true;
}

bool Function::Mapping::operator!=(const Mapping& other) const {

	if (_signatures.size() != other._signatures.size()) {
		return true;
	}

	for (auto it1 = _signatures.begin(), it2 = other._signatures.begin();
	    it1 != _signatures.end() && it2 != other._signatures.end(); ++it1, ++it2) {
		if (it1->first != it2->first || &it1->second.handle() != &it2->second.handle()) {
			return true;
		}
	}

	return false;
}

std::pair<Function::Mapping::iterator, bool> Function::Mapping::emplace(int signature, Signature&& handle) {
	return _signatures.emplace(signature, std::move(handle));
}

std::pair<Function::Mapping::iterator, bool> Function::Mapping::insert(const std::pair<int, Signature>& signature) {
	return _signatures.insert(signature);
}

std::pair<Function::Mapping::iterator, bool> Function::Mapping::insert(
    const std::pair<int, Module::Handle&>& signature) {
	return _signatures.insert({signature.first, Signature(std::make_unique<Stateless>(signature.second))});
}

Function::Mapping::const_iterator Function::Mapping::lower_bound(int signature) const {
	return _signatures.lower_bound(signature);
}

Function::Mapping::const_iterator Function::Mapping::find(int signature) const {
	return _signatures.find(signature);
}

Function::Mapping::const_iterator Function::Mapping::cbegin() const {
	return _signatures.cbegin();
}

Function::Mapping::const_iterator Function::Mapping::begin() const {
	return _signatures.begin();
}

Function::Mapping::iterator Function::Mapping::begin() {
	return _signatures.begin();
}

Function::Mapping::const_iterator Function::Mapping::cend() const {
	return _signatures.cend();
}

Function::Mapping::const_iterator Function::Mapping::end() const {
	return _signatures.end();
}

Function::Mapping::iterator Function::Mapping::end() {
	return _signatures.end();
}

bool Function::Mapping::empty() const {
	return _signatures.empty();
}

void Function::mark() {
	if (!marked_bit()) {
		Data::mark();
		for (auto& signature : mapping) {
			signature.second.mark();
		}
	}
}

bool mint::is_stateful_function(const Reference& object) {
	if (object.data().format() != Data::Format::function) {
		return false;
	}
	return std::ranges::any_of(std::views::values(object.data<Function>().mapping), [](auto& signature) {
		return signature.template context<Function::Stateful>() != nullptr;
	});
}

Coroutine::Coroutine(std::unique_ptr<SavedState>&& state, std::size_t stack_size) :
    _saved_state(std::move(state)),
    _stored_stack(std::from_range,
        std::views::drop(_saved_state->cursor.get().stack(), static_cast<std::ptrdiff_t>(stack_size))) {
	_saved_state->cursor.get().stack().resize(stack_size);
}

void Coroutine::call(Cursor& cursor, Reference&& self) {

	if (_state != State::ready) {
		error("cannot reuse already awaited coroutine");
	}

	_context = std::make_shared<Context>(Context {
	    .stack_size = cursor.stack().size(),
	});

	cursor.stack().emplace_back(std::move(self));
	cursor.stack().append_range(std::move(_stored_stack));
	_stored_stack.clear();

	cursor.restore(std::move(_saved_state));

	_state = State::running;
}

void Coroutine::await(Cursor& cursor, Reference&& self) {

	if (_state != State::ready) {
		error("cannot reuse already awaited coroutine");
	}

	const auto stack_size = cursor.stack().size();
	cursor.stack().emplace_back(std::move(self));
	cursor.stack().append_range(std::move(_stored_stack));
	_stored_stack.clear();

	_parent_saved_state = cursor.suspend(std::move(_saved_state));

	assert(_parent_saved_state->stack_frame->coroutine);
	_context = _parent_saved_state->stack_frame->coroutine->data<Coroutine>()._context;
	_stack_offset = stack_size - _context->stack_size;

	_state = State::running;
}

std::unique_ptr<mint::SavedState> mint::Coroutine::yield(Cursor& cursor) {

	assert(_state == State::running);

	if (_parent_saved_state) {
		return cursor.suspend(std::move(_parent_saved_state), _context->stack_size);
	}

	_state = State::waiting;
	return cursor.interrupt(_context->stack_size);
}

void Coroutine::resume(Cursor& cursor, std::unique_ptr<mint::SavedState>&& state) {

	assert(_state == State::running || _state == State::waiting);

	switch (_state) {
	case State::running:
		_stack_offset = cursor.stack().size() - _context->stack_size;
		_parent_saved_state = cursor.suspend(std::move(state), _context->stack_size);
		break;
	case State::waiting:
		_context->stack_size = cursor.stack().size() - _stack_offset;
		cursor.restore(std::move(state), _context->stack_size);
		_state = State::running;
		break;
	default:
		break;
	}
}

void Coroutine::resume(Cursor& cursor, Reference&& value) {

	assert(_state == State::running);

	if (_parent_saved_state) {
		cursor.stack().resize(_context->stack_size + _stack_offset);
		cursor.exit_call();
		cursor.restore(std::move(_parent_saved_state), _context->stack_size);
	}
	else {
		cursor.stack().resize(_context->stack_size + _stack_offset);
		cursor.exit_call();
	}
	cursor.stack().emplace_back(std::move(value));
	_state = State::completed;
}

void Coroutine::resume(Cursor& cursor) {

	assert(_state == State::waiting);

	_context->stack_size = cursor.stack().size();
	cursor.stack().append_range(std::move(_stored_stack));
	_stored_stack.clear();
	cursor.restore(std::move(_saved_state), _context->stack_size);

	_state = State::running;
}

void Coroutine::suspend(Cursor& cursor) {

	assert(_state == State::running);

	_stored_stack.append_range(std::views::drop(cursor.stack(), static_cast<std::ptrdiff_t>(_context->stack_size)));
	cursor.stack().resize(_context->stack_size);
	_saved_state = cursor.interrupt(_context->stack_size);

	_state = State::waiting;
}

void Coroutine::raise(Cursor& cursor) {
	cursor.exit_call();
	if (_parent_saved_state) {
		cursor.restore(std::move(_parent_saved_state), _context->stack_size);
	}
	_state = State::failed;
}

void Coroutine::exit(Cursor& cursor) {

	assert(_state == State::running);

	if (_parent_saved_state) {
		cursor.stack().resize(_context->stack_size + _stack_offset);
		cursor.exit_call();
		cursor.restore(std::move(_parent_saved_state), _context->stack_size);
	}
	else {
		cursor.stack().resize(_context->stack_size + _stack_offset);
		cursor.exit_call();
	}
}

void Coroutine::mark() {
	if (!marked_bit()) {
		Data::mark();
		if (_saved_state) {
			_saved_state->stack_frame->mark();
		}
		for (const auto& item : _stored_stack) {
			item.data().mark();
		}
		if (_parent_saved_state) {
			_parent_saved_state->stack_frame->mark();
		}
	}
}
