#include "Memory/casttool.h"
#include "Memory/object.h"
#include "Memory/class.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
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

vector<Reference *> to_array(const Reference &ref) {

	vector<Reference *> result;

	switch (ref.data()->format) {

	}

	return result;
}

map<Reference, Reference> to_hash( const Reference &ref) {

	switch (ref.data()->format) {

	}

	return map<Reference, Reference>();
}

void iterator_init(std::queue<SharedReference> &iterator, const Reference &ref) {

	switch (ref.data()->format) {
	// case Data::fmt_none:
	// case Data::fmt_null:
	// case Data::fmt_number:
	// case Data::fmt_function:
	// 	break;
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == StringClass::instance()) {
			/// \todo iterator on utf-8 char
		}
		else if (((Object *)ref.data())->metadata == ArrayClass::instance()) {
			for (Reference *item : ((Array *)ref.data())->values) {
				iterator.push(item);
			}
		}
		else if (((Object *)ref.data())->metadata == ArrayClass::instance()) {
			for (auto &item : ((Hash *)ref.data())->values) {
				iterator.push((Reference *)&item.first);
			}
		}
		else if (((Object *)ref.data())->metadata == IteratorClass::instance()) {
			iterator = ((Iterator *)ref.data())->ctx;
		}
		break;
	}
}
