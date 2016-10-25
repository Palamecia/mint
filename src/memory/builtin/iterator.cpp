#include "memory/builtin/iterator.h"

IteratorClass *IteratorClass::instance() {

	static IteratorClass *g_instance = new IteratorClass;

	return g_instance;
}

Iterator::Iterator() : Object(IteratorClass::instance()) {}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	/// \todo register operator overloads
}
