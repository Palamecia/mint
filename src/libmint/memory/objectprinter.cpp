#include "memory/objectprinter.h"
#include "memory/operatortool.h"
#include "memory/functiontool.h"
#include "memory/reference.h"
#include "memory/object.h"
#include "system/error.h"
#include "ast/module.h"
#include "ast/cursor.h"

using namespace mint;
using namespace std;

class ResultHandler : public Module {
public:
	ResultHandler() {
		pushNodes({Node::unload_reference, Node::module_end});
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
	case Printer::none:
		m_cursor->stack().emplace_back(WeakReference::share(m_object));
		m_cursor->stack().emplace_back(WeakReference::create<None>());
		break;

	case Printer::null:
		m_cursor->stack().emplace_back(WeakReference::share(m_object));
		m_cursor->stack().emplace_back(WeakReference::create<Null>());
		break;

	case Printer::regex:
	case Printer::array:
	case Printer::hash:
	case Printer::iterator:
	case Printer::object:
		m_cursor->stack().emplace_back(WeakReference::share(m_object));
		m_cursor->stack().emplace_back(WeakReference(flags, static_cast<Object *>(value)));
		break;

	case Printer::package:
		m_cursor->stack().emplace_back(WeakReference::share(m_object));
		m_cursor->stack().emplace_back(WeakReference(flags, static_cast<Package *>(value)));
		break;

	case Printer::function:
		m_cursor->stack().emplace_back(WeakReference::share(m_object));
		m_cursor->stack().emplace_back(WeakReference(flags, static_cast<Function *>(value)));
		break;
	}

	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (UNLIKELY(!call_overload(m_cursor, Symbol::Write, 1))) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}

	return true;
}

void ObjectPrinter::print(const char *value) {

	m_cursor->stack().emplace_back(WeakReference::share(m_object));
	m_cursor->stack().emplace_back(create_string(value));
	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (UNLIKELY(!call_overload(m_cursor, Symbol::Write, 1))) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}

void ObjectPrinter::print(double value) {

	m_cursor->stack().emplace_back(WeakReference::share(m_object));
	m_cursor->stack().emplace_back(WeakReference::create<Number>(value));
	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (UNLIKELY(!call_overload(m_cursor, Symbol::Write, 1))) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}

void ObjectPrinter::print(bool value) {

	m_cursor->stack().emplace_back(WeakReference::share(m_object));
	m_cursor->stack().emplace_back(WeakReference::create<Boolean>(value));
	m_cursor->call(&ResultHandler::instance(), 0, &GlobalData::instance());

	if (UNLIKELY(!call_overload(m_cursor, Symbol::Write, 1))) {
		m_cursor->exitModule();
		error("class '%s' dosen't ovreload 'write'(1)", type_name(m_object).c_str());
	}
}
