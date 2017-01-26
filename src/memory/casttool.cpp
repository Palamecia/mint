#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/builtin/string.h"
#include "ast/abstractsyntaxtree.h"
#include "system/utf8iterator.h"
#include "system/error.h"

#include <string>

using namespace std;

string number_to_char(long number) {

	string result;

	while (number) {
		result.insert(result.begin(), number % (1 << 8));
		number = number / (1 << 8);
	}

	return result;
}

double to_number(AbstractSynatxTree *ast, const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise((Reference *)&ref);
		break;
	case Data::fmt_number:
		return ((Number *)ref.data())->value;
	case Data::fmt_boolean:
		return ((Boolean *)ref.data())->value;
	case Data::fmt_object:
		switch (((Object *)ref.data())->metadata->metatype()) {
		case Class::string:
			if (const char *value = ((String *)ref.data())->str.c_str()) {

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

				return strtod(value, nullptr);
			}
			break;
		case Class::iterator:
			for (SharedReference item; iterator_next((Iterator *)ref.data(), item);) {
				return to_number(ast, *item);
			}
			break;
		default:
			error("invalid conversion from '%s' to 'number'", ((Object *)ref.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid conversion from 'function' to 'number'");
		break;
	}

	return 0;
}

bool to_boolean(AbstractSynatxTree *ast, const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		return false;
	case Data::fmt_number:
		return ((Number *)ref.data())->value;
	case Data::fmt_boolean:
		return ((Boolean *)ref.data())->value;
	default:
		break;
	}

	return true;
}

string to_char(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		return string();
	case Data::fmt_number:
		return number_to_char(((Number *)ref.data())->value);
	case Data::fmt_boolean:
		return ((Boolean *)ref.data())->value ? "y" : "n";
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata->metatype() == Class::string) {
			return *utf8iterator(((String *)ref.data())->str.begin());
		}
		error("invalid conversion from '%s' to 'character'", ((Object *)ref.data())->metadata->name().c_str());
		break;
	case Data::fmt_function:
		error("invalid conversion from 'function' to 'character'");
		break;
	}

	return string();
}

string to_string(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
		return "(none)";
	case Data::fmt_null:
		return "(null)";
	case Data::fmt_number:
		return to_string(((Number*)ref.data())->value);
	case Data::fmt_boolean:
		return ((Boolean *)ref.data())->value ? "true" : "false";
	case Data::fmt_object:
		switch (((Object *)ref.data())->metadata->metatype()) {
		case Class::string:
			return ((String *)ref.data())->str;
		case Class::array:
			return "[" + [] (const Array::values_type &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += to_string(**it);
				}
				return join;
			} (((Array *)ref.data())->values) + "]";
		case Class::hash:
			return "{" + [] (const Hash::values_type &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += to_string(*it->first);
					join += " : ";
					join += to_string(*it->second);
				}
				return join;
			} (((Hash *)ref.data())->values) + "}";
		case Class::iterator:
			for (SharedReference item; iterator_next((Iterator *)ref.data(), item);) {
				return to_string(*item);
			}
			break;
		default:
			char buffer[(sizeof(void *) * 2) + 3];
			sprintf(buffer, "%p", ref.data());
			return buffer;
		}
		break;
	case Data::fmt_function:
		return "(function)";
	}

	return string();
}

Array::values_type to_array(const Reference &ref) {

	Array::values_type result;

	switch (ref.data()->format) {

	case Data::fmt_object:
		switch (((Object *)ref.data())->metadata->metatype()) {
		case Class::array:
			for (size_t i = 0; i < ((Array *)ref.data())->values.size(); ++i) {
				result.push_back(array_get_item((Array *)ref.data(), i));
			}
			return result;
		case Class::hash:
			for (auto &item : ((Hash *)ref.data())->values) {
				result.push_back(hash_get_key(item));
			}
			return result;
		case Class::iterator:
			for (SharedReference item; iterator_next((Iterator *)ref.data(), item);) {
				result.push_back(SharedReference::unique(new Reference(*item)));
			}
			return result;
		default:
			break;
		}
	default:
		result.push_back(SharedReference::unique(new Reference(ref)));
	}

	return result;
}

Hash::values_type to_hash(const Reference &ref) {

	Hash::values_type result;

	switch (ref.data()->format) {
	case Data::fmt_object:
		switch (((Object *)ref.data())->metadata->metatype()) {
		case Class::array:
			for (size_t i = 0; i < ((Array *)ref.data())->values.size(); ++i) {
				Reference *index = Reference::create<Number>();
				((Number *)index->data())->value = i;
				result.insert({SharedReference::unique(index), array_get_item((Array *)ref.data(), i)});
			}
			return result;
		case Class::hash:
			for (auto &item : ((Hash *)ref.data())->values) {
				result.insert({hash_get_key(item), hash_get_value(item)});
			}
			return result;
		case Class::iterator:
			for (SharedReference item; iterator_next((Iterator *)ref.data(), item);) {
				result.insert({SharedReference::unique(new Reference(*item)), SharedReference()});
			}
			return result;
		default:
			break;
		}
	default:
		result.insert({SharedReference::unique(new Reference(ref)), SharedReference()});
	}

	return result;
}
