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

#include "mint/memory/casttool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/system/string.h"
#include "mint/system/utf8.h"
#include "mint/system/error.h"
#include "mint/ast/cursor.h"

#include <optional>
#include <string>
#include <cmath>

using namespace std;
using namespace mint;

static string number_to_char(intmax_t number) {

	string result;

	while (number) {
		result.insert(result.begin(), static_cast<char>(number % (1 << 8)));
		number = number / (1 << 8);
	}

	return result;
}

double mint::to_unsigned_number(const string &str, bool *error) {

	const char *value = str.c_str();
	intmax_t intpart = 0;

	if (value[0] == '0') {
		switch (value[1]) {
		case 'b':
		case 'B':
			for (const char *cptr = value + 2; *cptr != '\0'; ++cptr) {
				switch (*cptr) {
				case '0':
					intpart = intpart << 1;
					break;
				case '1':
					intpart = (intpart << 1) + 1;
					break;
				default:
					if (error) {
						*error = true;
					}
					return 0;
				}
			}
			if (error) {
				*error = false;
			}
			return intpart;
		case 'o':
		case 'O':
			for (const char *cptr = value + 2; *cptr != '\0'; ++cptr) {
				if ('0' <= *cptr && *cptr < '8') {
					intpart = intpart * 8 + (*cptr - '0');
				}
				else {
					if (error) {
						*error = true;
					}
					return 0;
				}
			}
			if (error) {
				*error = false;
			}
			return intpart;

		case 'x':
		case 'X':
			for (const char *cptr = value + 2; *cptr != '\0'; ++cptr) {
				if (*cptr >= 'A') {
					const int digit = ((*cptr - 'A') & (~('a' ^ 'A'))) + 10;
					if (digit < 16) {
						intpart = intpart * 16 + digit;
					}
					else {
						if (error) {
							*error = true;
						}
						return 0;
					}
				}
				else if (isdigit(*cptr)) {
					intpart = intpart * 16 + (*cptr - '0');
				}
				else {
					if (error) {
						*error = true;
					}
					return 0;
				}
			}
			if (error) {
				*error = false;
			}
			return intpart;

		default:
			break;
		}
	}

	bool decimals = false;
	bool exponant = false;
	double fracpart = 0.;
	intmax_t fracexp = 0;
	intmax_t exppart = 0;
	intmax_t expsign = 0;

	for (const char *cptr = value; *cptr != '\0'; ++cptr) {
		switch (*cptr) {
		case '.':
			if (decimals || exponant) {
				if (error) {
					*error = true;
				}
				return 0;
			}
			decimals = true;
			break;
		case 'e':
		case 'E':
			if (exponant) {
				if (error) {
					*error = true;
				}
				return 0;
			}
			exponant = true;
			switch (cptr[1]) {
			case '+':
				expsign = +1;
				++cptr;
				break;
			case '-':
				expsign = -1;
				++cptr;
				break;
			default:
				break;
			}
			break;
		default:
			if (isdigit(*cptr)) {
				if (exponant) {
					exppart = exppart * 10 + (*cptr - '0');
				}
				else if (decimals) {
					fracpart = fracpart * 10. + (*cptr - '0');
					--fracexp;
				}
				else {
					intpart = intpart * 10 + (*cptr - '0');
				}
			}
			else {
				if (error) {
					*error = true;
				}
				return 0;
			}
		}
	}

	if (error) {
		*error = false;
	}

	if (exponant) {
		return (fracpart * pow(10, fracexp) + intpart) * pow(10, copysign(exppart, expsign));
	}

	if (decimals) {
		return fracpart * pow(10, fracexp) + intpart;
	}

	return intpart;
}

double mint::to_signed_number(const string &str, bool *error) {
	const char *data = str.data();
	return *data == '-' ? -to_unsigned_number(data + 1, error) : to_unsigned_number(str, error);
}

intmax_t mint::to_integer(double value) {
		return static_cast<intmax_t>(value);
}

intmax_t mint::to_integer(Cursor *cursor, Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(std::forward<Reference>(ref));
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
			error("invalid conversion from '%s' to 'number'", type_name(ref).c_str());
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
		cursor->raise(std::forward<Reference>(ref));
		break;
	case Data::fmt_number:
		return ref.data<Number>()->value;
	case Data::fmt_boolean:
		return ref.data<Boolean>()->value;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			return to_signed_number(ref.data<String>()->str, nullptr);
		case Class::iterator:
			if (optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_number(cursor, *item);
			}
			return to_number(cursor, WeakReference::create<None>());
		default:
			error("invalid conversion from '%s' to 'number'", type_name(ref).c_str());
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
		return {};
	case Data::fmt_number:
		return number_to_char(to_integer(ref.data<Number>()->value));
	case Data::fmt_boolean:
		return ref.data<Boolean>()->value ? "y" : "n";
	case Data::fmt_object:
		if (ref.data<Object>()->metadata->metatype() == Class::string) {
			return *const_utf8iterator(ref.data<String>()->str.begin());
		}
		else {
			error("invalid conversion from '%s' to 'character'", type_name(ref).c_str());
		}
	case Data::fmt_package:
		error("invalid conversion from 'package' to 'character'");
	case Data::fmt_function:
		error("invalid conversion from 'function' to 'character'");
	}

	return {};
}

string mint::to_string(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		return {};
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
			return "[" + mint::join(ref.data<Array>()->values, ", ", [](auto it) {
					   return to_string(array_get_item(it));
				   }) + "]";
		case Class::hash:
			return "{" + mint::join(ref.data<Hash>()->values, ", ", [](auto it) {
					   return to_string(hash_get_key(it)) + " : " + to_string(hash_get_value(it));
				   }) + "}";
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

	return {};
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
		[[fallthrough]];
	default:
		break;
	}

	try {
		return regex(to_string(ref));
	}
	catch (const regex_error &) {
		error("regular expression '/%s/' is not valid", to_string(ref).c_str());
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
		[[fallthrough]];
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
		[[fallthrough]];
	default:
		result.emplace(hash_key(ref), WeakReference());
	}

	return result;
}
