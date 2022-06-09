#include "memory/builtin/libobject.h"
#include "memory/globaldata.h"

using namespace mint;

LibObjectClass *LibObjectClass::instance() {
	return GlobalData::instance()->builtin<LibObjectClass>(Class::libobject);
}

LibObjectClass::LibObjectClass() : Class("libobject", Class::libobject) {
	createBuiltinMember(delete_operator);
}
