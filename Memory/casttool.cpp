#include "Memory/casttool.h"
#include "Memory/object.h"
#include "Memory/class.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "System/utf8iterator.h"
#include "System/error.h"

#include <string>

using namespace std;

double to_number(AbstractSynatxTree *ast, const Reference &ref) {
	switch (ref.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise((Reference *)&ref);
		break;
	case Data::fmt_number:
		return ((Number*)ref.data())->value;
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == StringClass::instance()) {
			return atof(((String *)ref.data())->str.c_str());
		}
		error("invalid conversion from '%s' to number", ((Object *)ref.data())->metadata->name().c_str());
		break;
	// case Data::fmt_function:
		break;
	}

	return 0;
}

string to_string(const Reference &ref) {
	switch (ref.data()->format) {
	case Data::fmt_none:
		return "(none)";
	case Data::fmt_null:
		return "(null)";
	case Data::fmt_number:
		return to_string(((Number*)ref.data())->value);
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == StringClass::instance()) {
			return ((String *)ref.data())->str;
		}
		else if (((Object *)ref.data())->metadata == ArrayClass::instance()) {
			return "[" + [] (const decltype(Array::values) &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += to_string(**it);
				}
				return join;
			} (((Array *)ref.data())->values) + "]";
		}
		else if (((Object *)ref.data())->metadata == HashClass::instance()) {
			return "{" + [] (const decltype(Hash::values) &values) {
				string join;
				for (auto it = values.begin(); it != values.end(); ++it) {
					if (it != values.begin()) {
						join += ", ";
					}
					join += to_string(it->first);
					join += " : ";
					join += to_string(it->second);
				}
				return join;
			} (((Hash *)ref.data())->values) + "}";
		}
		else {

		}
		break;
	// case Data::fmt_function:
	}

	return string();
}

string number_to_char(long number) {

	string result;

	while (number) {
		result.insert(result.begin(), number % (1 << 8));
		number = number / (1 << 8);
	}

	return result;
}

string to_char(AbstractSynatxTree *ast, const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		/// \todo
		break;
	case Data::fmt_number:
		return number_to_char(((Number *)ref.data())->value);
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == StringClass::instance()) {
			return *utf8iterator(((String *)ref.data())->str.begin());
		}
		/// \todo
		break;
	}

	return string();
}

vector<unique_ptr<Reference>> to_array(const Reference &ref) {

	vector<unique_ptr<Reference>> result;

	switch (ref.data()->format) {

	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == ArrayClass::instance()) {
			return move(((Array *)ref.data())->values);
		}
		else if (((Object *)ref.data())->metadata == HashClass::instance()) {
			Hash *hash = (Hash *)ref.data();
			for (auto item : hash->values) {
				result.push_back(unique_ptr<Reference>(new Reference(item.first)));
			}
			return result;
		}
		else if (((Object *)ref.data())->metadata == IteratorClass::instance()) {
			Iterator *it = (Iterator *)ref.data();
			while (!it->ctx.empty()) {
				result.push_back(unique_ptr<Reference>(new Reference(it->ctx.front().get())));
				it->ctx.pop_front();
			}
			return result;
		}
		else if (((Object *)ref.data())->metadata == StringClass::instance()) {
			String *str = (String *)ref.data();
			for (utf8iterator it = str->str.begin(); it != str->str.end(); ++it) {
				Reference *item = Reference::create<String>();
				((String *)item->data())->construct();
				((String *)item->data())->str = *it;
				result.push_back(unique_ptr<Reference>(item));
			}
			return result;
		}

	default:
		result.push_back(unique_ptr<Reference>(new Reference(ref.flags(), (Data *)ref.data())));
	}

	return result;
}

map<Reference, Reference> to_hash( const Reference &ref) {

	switch (ref.data()->format) {

	}

	return map<Reference, Reference>();
}

void iterator_init(std::deque<SharedReference> &iterator, const Reference &ref) {

	switch (ref.data()->format) {
	// case Data::fmt_none:
	// case Data::fmt_null:
	// case Data::fmt_number:
	// case Data::fmt_function:
	// 	break;
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == StringClass::instance()) {
			string &str = ((String *)ref.data())->str;
			for (utf8iterator it = str.begin(); it != str.end(); ++it) {
				Reference *item = Reference::create<String>();
				((String *)item->data())->construct();
				((String *)item->data())->str = *it;
				iterator.push_back(SharedReference::unique(item));
			}
		}
		else if (((Object *)ref.data())->metadata == ArrayClass::instance()) {
			for (auto &item : ((Array *)ref.data())->values) {
				iterator.push_back(item.get());
			}
		}
		else if (((Object *)ref.data())->metadata == ArrayClass::instance()) {
			for (auto &item : ((Hash *)ref.data())->values) {
				iterator.push_back((Reference *)&item.first);
			}
		}
		else if (((Object *)ref.data())->metadata == IteratorClass::instance()) {
			iterator = ((Iterator *)ref.data())->ctx;
		}
		break;
	}
}
