#include "Memory/casttool.h"
#include "Memory/object.h"

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
}
