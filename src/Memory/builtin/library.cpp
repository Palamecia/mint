#include "Memory/class.h"
#include "Memory/memorytool.h"
#include "Memory/casttool.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "System/plugin.h"
#include "System/error.h"

using namespace std;

LibraryClass::LibraryClass() : Class("lib") {

	createBuiltinMember("new", 2, AbstractSynatxTree::createBuiltinMethode(LIBRARY_TYPE, [] (AbstractSynatxTree *ast) {

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

	createBuiltinMember("call", -2, AbstractSynatxTree::createBuiltinMethode(LIBRARY_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Iterator *va_args = (Iterator *)ast->stack().at(base)->data();
							string fcn = to_string(*ast->stack().at(base - 1));
							Plugin *plugin = ((Library *)ast->stack().at(base - 2)->data())->plugin;

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().pop_back();

							for (Iterator::ctx_type::value_type &arg : va_args->ctx) {
								ast->stack().push_back(arg);
							}

							if (!plugin->call(fcn, ast)) {
								/// \todo
								error("");
							}
						}));
}
