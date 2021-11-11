#include "memory/builtin/libobject.h"

using namespace mint;

LibObjectClass *LibObjectClass::instance() {

	static LibObjectClass g_instance;
	return &g_instance;
}

LibObjectClass::LibObjectClass() : Class("libobject", Class::libobject) {
	createBuiltinMember(delete_operator);
}
