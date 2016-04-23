#include "Memory/casttool.h"
#include "Memory/object.h"
#include "Memory/class.h"

#include <string>

using namespace std;

double to_number(const Reference &ref) {
	switch (ref.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		return ((Number*)ref.data())->data;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}

	return 0;
}

string to_string(const Reference &ref) {
	switch (ref.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		return to_string(((Number*)ref.data())->data);
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}

	return string();
}

void iterator_init(std::queue<SharedReference> &iterator, const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
	case Data::fmt_number:
	case Data::fmt_function:
		break;
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == StringClass::instance()) {
			/// \todo iterator on utf-8 char
		}
		else if (((Object *)ref.data())->metadata == IteratorClass::instance()) {
			iterator = ((Iterator *)ref.data())->ctx;
		}
		break;
	case Data::fmt_hash:
		for (auto &item : ((Hash *)ref.data())->values) {
			iterator.push((Reference *)&item.first);
		}
		break;
	case Data::fmt_array:
		for (Reference &item : ((Array *)ref.data())->values) {
			iterator.push(&item);
		}
		break;
	}
}
