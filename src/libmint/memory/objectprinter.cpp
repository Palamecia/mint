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

void ObjectPrinter::print(SpecialValue value) {

	switch (value) {
	case none:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
		break;

	case null:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(SharedReference::unique(Reference::create<Null>()));
		break;

	case package:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(create_string("(package)"));
		break;

	case function:
		m_cursor->stack().push_back(&m_object);
		m_cursor->stack().push_back(create_string("(function)"));
		break;
	}

	if (!call_overload(m_cursor, "write", 1)) {
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
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

void ObjectPrinter::print(void *value) {

	Reference::Flags flags = Reference::const_address | Reference::const_value;
	Reference *object = new Reference(flags, reinterpret_cast<Object *>(const_cast<void *>(value)));

	m_cursor->stack().push_back(&m_object);
	m_cursor->stack().push_back(SharedReference::unique(object));

	if (!call_overload(m_cursor, "write", 1)) {
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}
