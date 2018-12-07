#include "memory/objectprinter.h"
#include "memory/operatortool.h"
#include "memory/functiontool.h"
#include "memory/reference.h"
#include "memory/object.h"
#include "system/error.h"
#include "ast/cursor.h"

using namespace mint;

ObjectPrinter::ObjectPrinter(Cursor *cursor, Reference::Flags flags, Object *object) :
	m_object(flags, object),
	m_cursor(cursor) {

}

bool ObjectPrinter::print(DataType type, void *value) {

	Reference::Flags flags = Reference::const_address | Reference::const_value;

	switch (type) {
	case none:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
		break;

	case null:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(SharedReference::unique(Reference::create<Null>()));
		break;

	case regex:
	case array:
	case hash:
	case iterator:
	case object:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(SharedReference::unique(new Reference(flags, static_cast<Object *>(value))));
		break;

	case package:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(SharedReference::unique(new Reference(flags, static_cast<Package *>(value))));
		break;

	case function:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(SharedReference::unique(new Reference(flags, static_cast<Function *>(value))));
		break;
	}

	if (!call_overload(m_cursor, "write", 1)) {
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}

	return true;
}

void ObjectPrinter::print(const char *value) {

	m_cursor->stack().push_back(&m_object);
	m_cursor->stack().push_back(create_string(value));

	if (!call_overload(m_cursor, "write", 1)) {
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}

void ObjectPrinter::print(double value) {

	m_cursor->stack().push_back(&m_object);
	m_cursor->stack().push_back(create_number(value));

	if (!call_overload(m_cursor, "write", 1)) {
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}

void ObjectPrinter::print(bool value) {

	m_cursor->stack().push_back(&m_object);
	m_cursor->stack().push_back(create_boolean(value));

	if (!call_overload(m_cursor, "write", 1)) {
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}
