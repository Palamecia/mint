#include "memory/builtin/library.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/plugin.h"
#include "system/error.h"

using namespace std;
using namespace mint;

LibraryClass *LibraryClass::instance() {

	static LibraryClass g_instance;
	return &g_instance;
}

Library::Library() : Object(LibraryClass::instance()) {
	plugin = nullptr;
}

Library::~Library() {
	delete plugin;
}

LibraryClass::LibraryClass() : Class("lib", Class::library) {

	createBuiltinMember("new", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							if (Plugin *plugin = Plugin::load(to_string(rvalue))) {
								self.data<Library>()->plugin = plugin;
								cursor->stack().pop_back();
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("call", VARIADIC 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Iterator *va_args = cursor->stack().at(base)->data<Iterator>();
							std::string fcn = to_string(*cursor->stack().at(base - 1));
							Plugin *plugin = cursor->stack().at(base - 2)->data<Library>()->plugin;

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();

							for (Iterator::ctx_type::value_type &arg : va_args->ctx) {
								cursor->stack().push_back(arg);
							}
							int signature = va_args->ctx.size();

							if (!plugin->call(fcn, signature, cursor)) {
								error("no function '%s' taking %d arguments found in plugin '%s'", fcn.c_str(), signature, plugin->getPath().c_str());
							}
						}));
}
