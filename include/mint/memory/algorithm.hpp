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

#ifndef MINT_ALGORITHM_HPP
#define MINT_ALGORITHM_HPP

#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/system/utf8.h"

namespace mint {

template<class Function>
void for_each(Reference &ref, Function function) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		break;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			for (utf8iterator i = ref.data<String>()->str.begin(); i != ref.data<String>()->str.end(); ++i) {
				String *substr = GarbageCollector::instance().alloc<String>(*i);
				substr->construct();
				function(WeakReference(Reference::const_address | Reference::const_value, substr));
			}
			break;
		case Class::array:
			for (Array::values_type::value_type &item : ref.data<Array>()->values) {
				function(std::forward<Reference>(item));
			}
			break;
		case Class::hash:
			for (Hash::values_type::value_type &item : ref.data<Hash>()->values) {
				Iterator *element = GarbageCollector::instance().alloc<Iterator>();
				element->construct();
				iterator_insert(element, hash_get_key(item));
				iterator_insert(element, hash_get_value(item));
				function(WeakReference(Reference::const_address | Reference::const_value, element));
			}
			break;
		case Class::iterator:
			while (!ref.data<Iterator>()->ctx.empty()) {
				function(ref.data<Iterator>()->ctx.next());
				ref.data<Iterator>()->ctx.pop();
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
bool for_each_if(Reference &ref, Function function) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		break;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			for (utf8iterator i = ref.data<String>()->str.begin(); i != ref.data<String>()->str.end(); ++i) {
				String *substr = GarbageCollector::instance().alloc<String>(*i);
				substr->construct();
				if (UNLIKELY(!function(WeakReference(Reference::const_address | Reference::const_value, substr)))) {
					return false;
				}
			}
			break;
		case Class::array:
			for (Array::values_type::value_type &item : ref.data<Array>()->values) {
				if (!function(std::forward<Reference>(item))) {
					return false;
				}
			}
			break;
		case Class::hash:
			for (Hash::values_type::value_type &item : ref.data<Hash>()->values) {
				Iterator *element = GarbageCollector::instance().alloc<Iterator>();
				element->construct();
				iterator_insert(element, hash_get_key(item));
				iterator_insert(element, hash_get_value(item));
				if (UNLIKELY(!function(WeakReference(Reference::const_address | Reference::const_value, element)))) {
					return false;
				}
			}
			break;
		case Class::iterator:
			while (!ref.data<Iterator>()->ctx.empty()) {
				if (UNLIKELY(!function(ref.data<Iterator>()->ctx.next()))) {
					return false;
				}
				ref.data<Iterator>()->ctx.pop();
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

#endif // MINT_ALGORITHM_HPP
