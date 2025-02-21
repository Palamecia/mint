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

#include "mint/memory/casttool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/system/string.h"
#include "mint/system/utf8.h"
#include "mint/system/error.h"
#include "mint/ast/cursor.h"

#include <cstddef>
#include <optional>
#include <string>
#include <cmath>

using namespace mint;

namespace {

std::string number_to_char(intmax_t number) {

	std::string result;

	while (number) {
		result.insert(result.begin(), static_cast<char>(number % (1 << 8)));
		number = number / (1 << 8);
	}

	return result;
}

}

double mint::to_unsigned_number(const std::string &str, bool *error) {

	const char *value = str.c_str();
	double intpart = 0;

	if (value[0] == '0') {
		switch (value[1]) {
		case 'b':
		case 'B':
			for (const char *cptr = value + 2; *cptr != '\0'; ++cptr) {
				switch (*cptr) {
				case '0':
					intpart = intpart * 2;
					break;
				case '1':
					intpart = (intpart * 2) + 1;
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
					intpart = (intpart * 8) + (*cptr - '0');
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
						intpart = (intpart * 16) + digit;
					}
					else {
						if (error) {
							*error = true;
						}
						return 0;
					}
				}
				else if (isdigit(*cptr)) {
					intpart = (intpart * 16) + (*cptr - '0');
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
	bool exponent = false;
	double fracpart = 0.;
	intmax_t fracexp = 0;
	intmax_t exppart = 0;
	intmax_t expsign = 0;

	for (const char *cptr = value; *cptr != '\0'; ++cptr) {
		switch (*cptr) {
		case '.':
			if (decimals || exponent) {
				if (error) {
					*error = true;
				}
				return 0;
			}
			decimals = true;
			break;
		case 'e':
		case 'E':
			if (exponent) {
				if (error) {
					*error = true;
				}
				return 0;
			}
			exponent = true;
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
				if (exponent) {
					exppart = (exppart * 10) + (*cptr - '0');
				}
				else if (decimals) {
					fracpart = (fracpart * 10) + (*cptr - '0');
					--fracexp;
				}
				else {
					intpart = (intpart * 10) + (*cptr - '0');
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

	if (exponent) {
		return (fracpart * pow(10, fracexp) + intpart) * pow(10, copysign(exppart, expsign));
	}

	if (decimals) {
		return fracpart * pow(10, fracexp) + intpart;
	}

	return intpart;
}

double mint::to_signed_number(const std::string &str, bool *error) {
	const char *data = str.data();
	return *data == '-' ? -to_unsigned_number(data + 1, error) : +to_unsigned_number(str, error);
}

uintmax_t mint::to_unsigned_integer(const std::string &str, bool *error) {

	const char *value = str.c_str();
	uintmax_t intpart = 0;

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
					intpart = (intpart * 8) + (*cptr - '0');
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
						intpart = (intpart * 16) + digit;
					}
					else {
						if (error) {
							*error = true;
						}
						return 0;
					}
				}
				else if (isdigit(*cptr)) {
					intpart = (intpart * 16) + (*cptr - '0');
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

	for (const char *cptr = value; *cptr != '\0'; ++cptr) {
		if ('0' <= *cptr && *cptr <= '9') {
			intpart = (intpart * 10) + (*cptr - '0');
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
}

intmax_t mint::to_signed_integer(const std::string &str, bool *error) {
	const char *data = str.data();
	return *data == '-' ? -static_cast<intmax_t>(to_unsigned_integer(data + 1, error))
						: +static_cast<intmax_t>(to_unsigned_integer(str, error));
}

intmax_t mint::to_integer(double value) {
	return static_cast<intmax_t>(value);
}

intmax_t mint::to_integer(Cursor *cursor, Reference &ref) {

	switch (ref.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(std::forward<Reference>(ref));
		break;
	case Data::FMT_NUMBER:
		return to_integer(ref.data<Number>()->value);
	case Data::FMT_BOOLEAN:
		return ref.data<Boolean>()->value;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			return to_signed_integer(ref.data<String>()->str);
		case Class::ITERATOR:
			if (std::optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_integer(cursor, *item);
			}
			return to_integer(cursor, WeakReference::create<None>());
		default:
			error("invalid conversion from '%s' to 'number'", type_name(ref).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid conversion from 'package' to 'number'");
	case Data::FMT_FUNCTION:
		error("invalid conversion from 'function' to 'number'");
	}

	return 0;
}

intmax_t mint::to_integer(Cursor *cursor, Reference &&ref) {

	switch (ref.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(std::move(ref));
		break;
	case Data::FMT_NUMBER:
		return to_integer(ref.data<Number>()->value);
	case Data::FMT_BOOLEAN:
		return ref.data<Boolean>()->value;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			return to_signed_integer(ref.data<String>()->str);
		case Class::ITERATOR:
			if (std::optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_integer(cursor, *item);
			}
			return to_integer(cursor, WeakReference::create<None>());
		default:
			error("invalid conversion from '%s' to 'number'", type_name(ref).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid conversion from 'package' to 'number'");
	case Data::FMT_FUNCTION:
		error("invalid conversion from 'function' to 'number'");
	}

	return 0;
}

double mint::to_number(Cursor *cursor, Reference &ref) {

	switch (ref.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(std::forward<Reference>(ref));
		break;
	case Data::FMT_NUMBER:
		return ref.data<Number>()->value;
	case Data::FMT_BOOLEAN:
		return ref.data<Boolean>()->value;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			return to_signed_number(ref.data<String>()->str);
		case Class::ITERATOR:
			if (std::optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_number(cursor, *item);
			}
			return to_number(cursor, WeakReference::create<None>());
		default:
			error("invalid conversion from '%s' to 'number'", type_name(ref).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid conversion from 'package' to 'number'");
	case Data::FMT_FUNCTION:
		error("invalid conversion from 'function' to 'number'");
	}

	return 0;
}

double mint::to_number(Cursor *cursor, Reference &&ref) {

	switch (ref.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(std::move(ref));
		break;
	case Data::FMT_NUMBER:
		return ref.data<Number>()->value;
	case Data::FMT_BOOLEAN:
		return ref.data<Boolean>()->value;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			return to_signed_number(ref.data<String>()->str);
		case Class::ITERATOR:
			if (std::optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_number(cursor, *item);
			}
			return to_number(cursor, WeakReference::create<None>());
		default:
			error("invalid conversion from '%s' to 'number'", type_name(ref).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid conversion from 'package' to 'number'");
	case Data::FMT_FUNCTION:
		error("invalid conversion from 'function' to 'number'");
	}

	return 0;
}

bool mint::to_boolean(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		return false;
	case Data::FMT_NUMBER:
		return ref.data<Number>()->value != 0.;
	case Data::FMT_BOOLEAN:
		return ref.data<Boolean>()->value;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::ITERATOR:
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

std::string mint::to_char(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		return {};
	case Data::FMT_NUMBER:
		return number_to_char(to_integer(ref.data<Number>()->value));
	case Data::FMT_BOOLEAN:
		return ref.data<Boolean>()->value ? "y" : "n";
	case Data::FMT_OBJECT:
		if (ref.data<Object>()->metadata->metatype() == Class::STRING) {
			return *const_utf8iterator(ref.data<String>()->str.begin());
		}
		else {
			error("invalid conversion from '%s' to 'character'", type_name(ref).c_str());
		}
	case Data::FMT_PACKAGE:
		error("invalid conversion from 'package' to 'character'");
	case Data::FMT_FUNCTION:
		error("invalid conversion from 'function' to 'character'");
	}

	return {};
}

std::string mint::to_string(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::FMT_NONE:
		return {};
	case Data::FMT_NULL:
		return "(null)";
	case Data::FMT_NUMBER:
		{
			double fracpart, intpart;
			if ((fracpart = modf(ref.data<Number>()->value, &intpart)) != 0.) {
				return mint::to_string(intpart + fracpart);
			}
			return mint::to_string(to_integer(intpart));
		}
	case Data::FMT_BOOLEAN:
		return ref.data<Boolean>()->value ? "true" : "false";
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			return ref.data<String>()->str;
		case Class::REGEX:
			return ref.data<Regex>()->initializer;
		case Class::ARRAY:
			return "["
				   + mint::join(ref.data<Array>()->values, ", ",
								[](auto it) {
									return to_string(array_get_item(it));
								})
				   + "]";
		case Class::HASH:
			return "{"
				   + mint::join(ref.data<Hash>()->values, ", ",
								[](auto it) {
									return to_string(hash_get_key(it)) + " : " + to_string(hash_get_value(it));
								})
				   + "}";
		case Class::ITERATOR:
			if (std::optional<WeakReference> &&item = iterator_get(ref.data<Iterator>())) {
				return to_string(*item);
			}
			return to_string(WeakReference::create<None>());
		case Class::OBJECT:
			return is_object(ref.data<Object>()) ? "(object)" : "(class)";
		case Class::LIBRARY:
			return "(library)";
		case Class::LIBOBJECT:
			return "(libobject)";
		}
	case Data::FMT_PACKAGE:
		return "(package)";
	case Data::FMT_FUNCTION:
		return "(function)";
	}

	return {};
}

std::regex mint::to_regex(Reference &ref) {

	switch (ref.data()->format) {
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::REGEX:
			return ref.data<Regex>()->expr;
		default:
			break;
		}
		[[fallthrough]];
	default:
		break;
	}

	try {
		return std::regex(to_string(ref));
	}
	catch (const std::regex_error &) {
		error("regular expression '/%s/' is not valid", to_string(ref).c_str());
	}
}

Array::values_type mint::to_array(Reference &ref) {

	Array::values_type result;

	switch (ref.data()->format) {
	case Data::FMT_NONE:
		return result;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::ARRAY:
			result.reserve(ref.data<Array>()->values.size());
			std::transform(ref.data<Array>()->values.begin(), ref.data<Array>()->values.end(),
						   std::back_inserter(result), [](auto &item) {
							   return array_get_item(item);
						   });
			return result;
		case Class::HASH:
			result.reserve(ref.data<Hash>()->values.size());
			std::transform(ref.data<Hash>()->values.begin(), ref.data<Hash>()->values.end(), std::back_inserter(result),
						   [](const auto &item) {
							   return hash_get_key(item);
						   });
			return result;
		case Class::ITERATOR:
			result.reserve(ref.data<Iterator>()->ctx.size());
			std::transform(ref.data<Iterator>()->ctx.begin(), ref.data<Iterator>()->ctx.end(),
						   std::back_inserter(result), [](const Reference &item) {
							   return array_item(item);
						   });
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

Hash::values_type mint::to_hash(Reference &ref) {

	Hash::values_type result;

	switch (ref.data()->format) {
	case Data::FMT_NONE:
		return result;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::ARRAY:
			for (size_t i = 0; i < ref.data<Array>()->values.size(); ++i) {
				result.emplace(WeakReference::create<Number>(static_cast<double>(i)),
							   array_get_item(ref.data<Array>()->values.at(i)));
			}
			return result;
		case Class::HASH:
			for (auto &item : ref.data<Hash>()->values) {
				result.emplace(hash_get_key(item), hash_get_value(item));
			}
			return result;
		case Class::ITERATOR:
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
