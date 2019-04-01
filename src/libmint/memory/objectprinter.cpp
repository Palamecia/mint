#include "memory/objectprinter.h"
#include "memory/operatortool.h"
#include "memory/functiontool.h"
#include "memory/reference.h"
#include "memory/object.h"
#include "system/error.h"
#include "ast/module.h"
#include "ast/cursor.h"

using namespace mint;

class ResultHandler : public Module {
public:
	ResultHandler() {

		Node node;

		node.command = Node::unload_reference;
		pushNode(node);

		node.command = Node::module_end;
		pushNode(node);
	}

	static ResultHandler &instance() {
		static ResultHandler g_instance;
		return g_instance;
	}
};

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

	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (!call_overload(m_cursor, "write", 1)) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}

	return true;
}

void ObjectPrinter::print(const char *value) {

	m_cursor->stack().push_back(&m_object);
	m_cursor->stack().push_back(create_string(value));
	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (!call_overload(m_cursor, "write", 1)) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}

void ObjectPrinter::print(double value) {

	m_cursor->stack().push_back(&m_object);
	m_cursor->stack().push_back(create_number(value));
	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (!call_overload(m_cursor, "write", 1)) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}

void ObjectPrinter::print(bool value) {

	m_cursor->stack().push_back(&m_object);
	m_cursor->stack().push_back(create_boolean(value));
	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (!call_overload(m_cursor, "write", 1)) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}
