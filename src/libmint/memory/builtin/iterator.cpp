#include "memory/builtin/iterator.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

IteratorClass *IteratorClass::instance() {

	static IteratorClass *g_instance = new IteratorClass;

	return g_instance;
}

Iterator::Iterator() : Object(IteratorClass::instance()) {}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Iterator it;
							iterator_init(&it, other);
							for (SharedReference &item : ((Iterator *)self.data())->ctx) {
								if ((item->flags() & Reference::const_ref) && (item->data()->format != Data::fmt_none)) {
									error("invalid modification of constant reference");
								}
								if (it.ctx.empty()) {
									item->move(Reference(Reference::standard, Reference::alloc<None>()));
								}
								else {
									item->move(*it.ctx.front());
									it.ctx.pop_front();
								}
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("next", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();
							SharedReference result;

							if (iterator_next((Iterator *)self.data(), result)) {
								cursor->stack().pop_back();
								cursor->stack().push_back(result);
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("values", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();
							Reference *result = Reference::create<Array>();

							((Object *)result->data())->construct();
							for (SharedReference &item : ((Iterator *)self.data())->ctx) {
								array_append((Array *)result->data(), SharedReference::unique(new Reference(*item)));
							}

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();
							Reference *result = Reference::create<Number>();

							((Number *)result->data())->value = ((Iterator *)self.data())->ctx.size();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	/// \todo register operator overloads
}
