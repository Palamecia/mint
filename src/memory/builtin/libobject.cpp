#include "memory/builtin/libobject.h"

LibObjectClass *LibObjectClass::instance() {

	static LibObjectClass *g_instance = new LibObjectClass;

	return g_instance;
}

LibObjectClass::LibObjectClass() : Class("libobject", Class::libobject) {}
