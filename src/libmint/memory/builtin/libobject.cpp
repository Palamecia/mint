#include "memory/builtin/libobject.h"

using namespace mint;

LibObjectClass *LibObjectClass::instance() {

	static LibObjectClass g_instance;
	return &g_instance;
}

LibObjectClass::LibObjectClass() : Class("libobject", Class::libobject) {
	MemberInfo *infos = new MemberInfo;
	infos->owner = this;
	infos->offset = members().size();
	members().emplace("delete", infos);
}
