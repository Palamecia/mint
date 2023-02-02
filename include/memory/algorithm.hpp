#ifndef MINT_ALGORITHM_HPP
#define MINT_ALGORITHM_HPP

#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "memory/builtin/string.h"
#include "memory/builtin/iterator.h"
#include "system/utf8iterator.h"

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
				String *substr = Reference::alloc<String>();
				substr->construct();
				substr->str = *i;
				function(WeakReference(Reference::const_address | Reference::const_value, substr));
			}
			break;
		case Class::array:
			for (Array::values_type::value_type &item : ref.data<Array>()->values) {
				function(std::move(item));
			}
			break;
		case Class::hash:
			for (Hash::values_type::value_type &item : ref.data<Hash>()->values) {
				Iterator *element = Reference::alloc<Iterator>();
				element->construct();
				iterator_insert(element, hash_get_key(item));
				iterator_insert(element, hash_get_value(item));
				function(WeakReference(Reference::const_address | Reference::const_value, element));
			}
			break;
		case Class::iterator:
			while (!ref.data<Iterator>()->ctx.empty()) {
				function(ref.data<Iterator>()->ctx.next());
				ref.data<Iterator>()->ctx.pop_next();
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
				String *substr = Reference::alloc<String>();
				substr->construct();
				substr->str = *i;
				if (UNLIKELY(!function(WeakReference(Reference::const_address | Reference::const_value, substr)))) {
					return false;
				}
			}
			break;
		case Class::array:
			for (Array::values_type::value_type &item : ref.data<Array>()->values) {
				if (!function(std::move(item))) {
					return false;
				}
			}
			break;
		case Class::hash:
			for (Hash::values_type::value_type &item : ref.data<Hash>()->values) {
				Iterator *element = Reference::alloc<Iterator>();
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
				ref.data<Iterator>()->ctx.pop_next();
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
