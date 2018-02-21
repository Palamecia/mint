#include "memory/builtin/iterator.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

using namespace mint;

IteratorClass *IteratorClass::instance() {

	static IteratorClass g_instance;
	return &g_instance;
}

Iterator::Iterator() : Object(IteratorClass::instance()) {}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Iterator it;
							iterator_init(&it, other);
							for (SharedReference &item : self.data<Iterator>()->ctx) {
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
							SharedReference result(nullptr);

							if (iterator_next(self.data<Iterator>(), result)) {
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

							result->data<Object>()->construct();
							for (SharedReference &item : self.data<Iterator>()->ctx) {
								array_append(result->data<Array>(), SharedReference::unique(new Reference(*item)));
							}

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();
							Reference *result = Reference::create<Number>();

							result->data<Number>()->value = self.data<Iterator>()->ctx.size();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	/// \todo register operator overloads
}
