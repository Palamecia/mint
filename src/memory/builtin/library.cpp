#include "memory/builtin/library.h"
#include "memory/memorytool.h"
#include "memory/casttool.h"
#include "ast/abstractsyntaxtree.h"
#include "system/plugin.h"
#include "system/error.h"

using namespace std;

LibraryClass *LibraryClass::instance() {

	static LibraryClass *g_instance = new LibraryClass;

	return g_instance;
}

Library::Library() : Object(LibraryClass::instance()) {
	plugin = nullptr;
}

Library::~Library() {
	delete plugin;
}

LibraryClass::LibraryClass() : Class("lib", Class::library) {

	createBuiltinMember("new", 2, AbstractSynatxTree::createBuiltinMethode(-metatype(), [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							if (Plugin *plugin = Plugin::load(to_string(rvalue))) {
								((Library *)lvalue.data())->plugin = plugin;
								ast->stack().pop_back();
							}
							else {
								ast->stack().pop_back();
								ast->stack().pop_back();
								ast->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("call", -2, AbstractSynatxTree::createBuiltinMethode(-metatype(), [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Iterator *va_args = (Iterator *)ast->stack().at(base)->data();
							std::string fcn = to_string(*ast->stack().at(base - 1));
							Plugin *plugin = ((Library *)ast->stack().at(base - 2)->data())->plugin;

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().pop_back();

							for (Iterator::ctx_type::value_type &arg : va_args->ctx) {
								ast->stack().push_back(arg);
							}

							if (!plugin->call(fcn, ast)) {
								error("no function '%s' taking %d arguments found in plugin '%s'", fcn.c_str(), va_args->ctx.size(), plugin->getPath().c_str());
							}
						}));
}
