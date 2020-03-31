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

							SharedReference &name = cursor->stack().at(base);
							SharedReference &self = cursor->stack().at(base - 1);

							if (Plugin *plugin = Plugin::load(to_string(name))) {
								self->data<Library>()->plugin = plugin;
								cursor->stack().pop_back();
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(SharedReference::unique(StrongReference::create<None>()));
							}
						}));

	createBuiltinMember("call", VARIADIC 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference va_args = move(cursor->stack().at(base));
							SharedReference function = move(cursor->stack().at(base - 1));
							SharedReference self = move(cursor->stack().at(base - 2));

							std::string func_name = to_string(function);
							Plugin *plugin = self->data<Library>()->plugin;

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();

							for (Iterator::ctx_type::value_type &arg : va_args->data<Iterator>()->ctx) {
								cursor->stack().emplace_back(move(arg));
							}
							int signature = static_cast<int>(va_args->data<Iterator>()->ctx.size());

							if (!plugin->call(func_name, signature, cursor)) {
								error("no function '%s' taking %d arguments found in plugin '%s'", func_name.c_str(), signature, plugin->getPath().c_str());
							}
						}));
}
