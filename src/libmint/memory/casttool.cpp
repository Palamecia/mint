#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "memory/builtin/string.h"
#include "memory/builtin/regex.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"
#include "system/error.h"

#include <string>
#include <cmath>

using namespace std;
using namespace mint;

string number_to_char(long number) {

	string result;

	while (number) {
		result.insert(result.begin(), static_cast<char>(number % (1 << 8)));
		number = number / (1 << 8);
	}

	return result;
}

intmax_t mint::to_integer(double value) {
	return static_cast<intmax_t>(value);
}

intmax_t mint::to_integer(Cursor *cursor, Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(forward<Reference>(ref));
		break;
	case Data::fmt_number:
		return to_integer(ref.data<Number>()->value);
	case Data::fmt_boolean:
		return ref.data<Boolean>()->value;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			if (const char *value = ref.data<String>()->str.c_str()) {

				if (value[0] == '0') {
					switch (value[1]) {
					case 'b':
					case 'B':
						return strtol(value + 2, nullptr, 2);

					case 'o':
					case 'O':
						return strtol(value + 2, nullptr, 8);

					case 'x':
					case 'X':
						return strtol(value + 2, nullptr, 16);

					default:
						break;
					}
				}

				return strtol(value, nullptr, 10);
			}
			break;
		case Class::iterator:
			if (optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_integer(cursor, *item);
			}
			return to_integer(cursor, WeakReference::create<None>());
		default:
			string type = type_name(ref);
			error("invalid conversion from '%s' to 'number'", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid conversion from 'package' to 'number'");
	case Data::fmt_function:
		error("invalid conversion from 'function' to 'number'");
	}

	return 0;
}

intmax_t mint::to_integer(Cursor *cursor, Reference &&ref) {
	return to_integer(cursor, static_cast<Reference &>(ref));
}

double mint::to_number(Cursor *cursor, Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(forward<Reference>(ref));
		break;
	case Data::fmt_number:
		return ref.data<Number>()->value;
	case Data::fmt_boolean:
		return ref.data<Boolean>()->value;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			if (const char *value = ref.data<String>()->str.c_str()) {

				if (value[0] == '0') {
					switch (value[1]) {
					case 'b':
					case 'B':
						return static_cast<double>(strtol(value + 2, nullptr, 2));

					case 'o':
					case 'O':
						return static_cast<double>(strtol(value + 2, nullptr, 8));

					case 'x':
					case 'X':
						return static_cast<double>(strtol(value + 2, nullptr, 16));

					default:
						break;
					}
				}

				return strtod(value, nullptr);
			}
			break;
		case Class::iterator:
			if (optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_number(cursor, *item);
			}
			return to_number(cursor, WeakReference::create<None>());
		default:
			string type = type_name(ref);
			error("invalid conversion from '%s' to 'number'", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid conversion from 'package' to 'number'");
	case Data::fmt_function:
		error("invalid conversion from 'function' to 'number'");
	}

	return 0;
}

double mint::to_number(Cursor *cursor, Reference &&ref) {
	return to_number(cursor, static_cast<Reference &>(ref));
}

bool mint::to_boolean(Cursor *cursor, Reference &ref) {

	((void)cursor);

	switch (ref.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		return false;
	case Data::fmt_number:
		return ref.data<Number>()->value != 0.;
	case Data::fmt_boolean:
		return ref.data<Boolean>()->value;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::iterator:
			return !ref.data<Iterator>()->ctx.empty();
		default:
			break;
		}
		break;
	default:
		break;
	}

	return true;
}

bool mint::to_boolean(Cursor *cursor, Reference &&ref) {
	return to_boolean(cursor, static_cast<Reference &>(ref));
}

string mint::to_char(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		return string();
	case Data::fmt_number:
		return number_to_char(to_integer(ref.data<Number>()->value));
	case Data::fmt_boolean:
		return ref.data<Boolean>()->value ? "y" : "n";
	case Data::fmt_object:
		if (ref.data<Object>()->metadata->metatype() == Class::string) {
			return *const_utf8iterator(ref.data<String>()->str.begin());
		}
		else {
			string type = type_name(ref);
			error("invalid conversion from '%s' to 'character'", type.c_str());
		}
	case Data::fmt_package:
		error("invalid conversion from 'package' to 'character'");
	case Data::fmt_function:
		error("invalid conversion from 'function' to 'character'");
	}

	return string();
}

string mint::to_string(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		return string();
	case Data::fmt_null:
		return "(null)";
	case Data::fmt_number:
	{
		double fracpart, intpart;
		if ((fracpart = modf(ref.data<Number>()->value, &intpart)) != 0.) {
			return mint::to_string(intpart + fracpart);
		}
		return mint::to_string(to_integer(intpart));
	}
	case Data::fmt_boolean:
		return ref.data<Boolean>()->value ? "true" : "false";
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			return ref.data<String>()->str;
		case Class::regex:
			return ref.data<Regex>()->initializer;
		case Class::array:
			return "[" + [] (Array::values_type &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += to_string(array_get_item(it));
				}
				return join;
			} (ref.data<Array>()->values) + "]";
		case Class::hash:
			return "{" + [] (Hash::values_type &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += to_string(hash_get_key(it));
					join += " : ";
					join += to_string(hash_get_value(it));
				}
				return join;
			} (ref.data<Hash>()->values) + "}";
		case Class::iterator:
			if (optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_string(*item);
			}
			return to_string(WeakReference::create<None>());
		default:
			return mint::to_string(ref.data());
		}
	case Data::fmt_package:
		return "(package)";
	case Data::fmt_function:
		return "(function)";
	}

	return string();
}

regex mint::to_regex(Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::regex:
			return ref.data<Regex>()->expr;
		default:
			break;
		}
		fall_through;
	default:
		break;
	}

	try {
		return regex(to_string(ref));
	}
	catch (const regex_error &) {
		string re_str = to_string(ref);
		error("regular expression '/%s/' is not valid", re_str.c_str());
	}
}

Array::values_type mint::to_array(Reference &ref) {

	Array::values_type result;

	switch (ref.data()->format) {
	case Data::fmt_none:
		return result;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::array:
			for (auto &item : ref.data<Array>()->values) {
				result.emplace_back(array_get_item(item));
			}
			return result;
		case Class::hash:
			for (auto &item : ref.data<Hash>()->values) {
				result.emplace_back(hash_get_key(item));
			}
			return result;
		case Class::iterator:
			for (const Reference &item : ref.data<Iterator>()->ctx) {
				result.emplace_back(array_item(item));
			}
			return result;
		default:
			break;
		}
		fall_through;
	default:
		result.emplace_back(array_item(ref));
	}

	return result;
}

Hash::values_type mint::to_hash(Cursor *cursor, Reference &ref) {

	((void)cursor);

	Hash::values_type result;

	switch (ref.data()->format) {
	case Data::fmt_none:
		return result;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::array:
			for (size_t i = 0; i < ref.data<Array>()->values.size(); ++i) {
				result.emplace(WeakReference::create<Number>(static_cast<double>(i)), array_get_item(ref.data<Array>()->values.at(i)));
			}
			return result;
		case Class::hash:
			for (auto &item : ref.data<Hash>()->values) {
				result.emplace(hash_get_key(item), hash_get_value(item));
			}
			return result;
		case Class::iterator:
			for (const Reference &item : ref.data<Iterator>()->ctx) {
				result.emplace(hash_key(item), WeakReference());
			}
			return result;
		default:
			break;
		}
		fall_through;
	default:
		result.emplace(hash_key(ref), WeakReference());
	}

	return result;
}
