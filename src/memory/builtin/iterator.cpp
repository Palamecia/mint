#include "memory/builtin/iterator.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "system/error.h"

IteratorClass *IteratorClass::instance() {

	static IteratorClass *g_instance = new IteratorClass;

	return g_instance;
}

Iterator::Iterator() : Object(IteratorClass::instance()) {}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &other = *ast->stack().at(base);
							Reference &self = *ast->stack().at(base - 1);

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

							ast->stack().pop_back();
						}));

	createBuiltinMember("next", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							Reference &self = *ast->stack().back();
							SharedReference result;

							if (iterator_next((Iterator *)self.data(), result)) {
								ast->stack().pop_back();
								ast->stack().push_back(result);
							}
							else {
								ast->stack().pop_back();
								ast->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	/// \todo register operator overloads
}
