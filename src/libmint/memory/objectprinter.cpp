#include "memory/objectprinter.h"
#include "memory/operatortool.h"
#include "memory/memorytool.h"
#include "memory/globaldata.h"
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

void ObjectPrinter::print(Reference &reference) {

	m_cursor->stack().emplace_back(WeakReference::share(m_object));
	m_cursor->stack().emplace_back(WeakReference::share(reference));
	m_cursor->call(&ResultHandler::instance(), 0, GlobalData::instance());

	if (UNLIKELY(!call_overload(m_cursor, Symbol::Write, 1))) {
		m_cursor->exitModule();
		string type = type_name(m_object);
		error("class '%s' dosen't ovreload 'write'(1)", type.c_str());
	}
}
