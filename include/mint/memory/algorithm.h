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

#ifndef MINT_MEMORY_ALGORITHM_H
#define MINT_MEMORY_ALGORITHM_H

#include "mint/ast/cursor.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/symbol_table.h"
#include "mint/system/utf8.h"
#include <functional>
#include <utility>

namespace mint {

template<class... Ts>
struct Overloaded : Ts... {
	using Ts::operator()...;
};

template<class R, class Visitor>
R visit(Visitor&& visitor, const Reference& reference) {
	switch (reference.data().format()) {
	case Data::Format::none:
		return std::invoke(std::forward<Visitor>(visitor), reference.data<None>());
	case Data::Format::null:
		return std::invoke(std::forward<Visitor>(visitor), reference.data<Null>());
	case Data::Format::number:
		return std::invoke(std::forward<Visitor>(visitor), reference.data<Number>());
	case Data::Format::boolean:
		return std::invoke(std::forward<Visitor>(visitor), reference.data<Boolean>());
	case Data::Format::object:
		switch (reference.data<Object>().metadata.metatype()) {
		case Class::Metatype::object:
			return std::invoke(std::forward<Visitor>(visitor), reference.data<Object>());
		case Class::Metatype::string:
			return std::invoke(std::forward<Visitor>(visitor), reference.data<String>());
		case Class::Metatype::regex:
			return std::invoke(std::forward<Visitor>(visitor), reference.data<Regex>());
		case Class::Metatype::array:
			return std::invoke(std::forward<Visitor>(visitor), reference.data<Array>());
		case Class::Metatype::hash:
			return std::invoke(std::forward<Visitor>(visitor), reference.data<Hash>());
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			return std::invoke(std::forward<Visitor>(visitor), reference.data<Iterator>());
		case Class::Metatype::library:
			return std::invoke(std::forward<Visitor>(visitor), reference.data<Library>());
		case Class::Metatype::libobject:
			return std::invoke(std::forward<Visitor>(visitor), reference.data());
		}
		return std::invoke(std::forward<Visitor>(visitor), reference.data<Object>());
	case Data::Format::package:
		return std::invoke(std::forward<Visitor>(visitor), reference.data<Package>());
	case Data::Format::function:
		return std::invoke(std::forward<Visitor>(visitor), reference.data<Function>());
	case Data::Format::coroutine:
		return std::invoke(std::forward<Visitor>(visitor), reference.data<Coroutine>());
	}
	if constexpr (!std::is_same_v<void, R>) {
		return {};
	}
}

template<class Function>
void for_each(Cursor& cursor, const Reference& ref, Function function) {
	switch (ref.data().format()) {
	case Data::Format::none:
		break;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			for (const auto& item : views::utf8(ref.data<String>().str)) {
				auto substr = make_reference<String>(Reference::const_address | Reference::const_value, cursor.ast(),
				    item);
				substr.data<String>().construct();
				function(std::move(substr));
			}
			break;
		case Class::Metatype::array:
			for (auto& item : ref.data<Array>().values) {
				function(std::forward<Reference>(item));
			}
			break;
		case Class::Metatype::hash:
			for (auto& item : ref.data<Hash>().values) {
				auto element = make_reference<Iterator>(Reference::const_address | Reference::const_value, cursor.ast());
				iterator_yield(cursor, element.data<Iterator>(), hash_get_key(item));
				iterator_yield(cursor, element.data<Iterator>(), hash_get_value(item));
				element.data<Iterator>().construct();
				function(std::move(element));
			}
			break;
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			while (!ref.data<Iterator>().ctx.empty()) {
				function(ref.data<Iterator>().ctx.get());
				ref.data<Iterator>().ctx.next(cursor);
			}
			break;
		default:
			function(ref);
		}
		break;

	default:
		function(ref);
	}
}

template<class Function>
bool for_each_if(Cursor& cursor, const Reference& ref, Function function) {
	switch (ref.data().format()) {
	case Data::Format::none:
		break;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			for (const auto& item : views::utf8(ref.data<String>().str)) {
				auto substr = make_reference<String>(Reference::const_address | Reference::const_value, cursor.ast(),
				    item);
				substr.data<String>().construct();
				if (!function(std::move(substr))) [[unlikely]] {
					return false;
				}
			}
			break;
		case Class::Metatype::array:
			for (auto& item : ref.data<Array>().values) {
				if (!function(std::forward<Reference>(item))) [[unlikely]] {
					return false;
				}
			}
			break;
		case Class::Metatype::hash:
			for (auto& item : ref.data<Hash>().values) {
				auto element = make_reference<Iterator>(Reference::const_address | Reference::const_value, cursor.ast());
				iterator_yield(cursor, element.data<Iterator>(), hash_get_key(item));
				iterator_yield(cursor, element.data<Iterator>(), hash_get_value(item));
				element.data<Iterator>().construct();
				if (!function(std::move(element))) [[unlikely]] {
					return false;
				}
			}
			break;
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			while (!ref.data<Iterator>().ctx.empty()) {
				if (!function(ref.data<Iterator>().ctx.get())) [[unlikely]] {
					return false;
				}
				ref.data<Iterator>().ctx.next(cursor);
			}
			break;
		default:
			return function(ref);
		}
		break;

	default:
		return function(ref);
	}
	return true;
}

}

#endif // MINT_MEMORY_ALGORITHM_H
