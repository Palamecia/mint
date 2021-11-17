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

Library::Library() : Object(LibraryClass::instance()),
	plugin(nullptr) {

}

Library::Library(const Library &other) : Object(LibraryClass::instance()),
	plugin(other.plugin ? new Plugin(other.plugin->getPath()) :nullptr) {

}

Library::~Library() {
	delete plugin;
}

LibraryClass::LibraryClass() : Class("lib", Class::library) {

	createBuiltinMember(Class::new_operator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &name = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							if (Plugin *plugin = Plugin::load(to_string(name))) {
								self.data<Library>()->plugin = plugin;
								cursor->stack().pop_back();
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(WeakReference::create<None>());
							}
						}));

	createBuiltinMember("call", AbstractSyntaxTree::createBuiltinMethode(this, VARIADIC 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference va_args = move_from_stack(cursor, base);
							WeakReference function = move_from_stack(cursor, base - 1);
							WeakReference self = move_from_stack(cursor, base - 2);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();

							std::string func_name = to_string(function);
							Plugin *plugin = self.data<Library>()->plugin;

							for (Iterator::ctx_type::value_type &arg : va_args.data<Iterator>()->ctx) {
								cursor->stack().emplace_back(forward<Reference>(arg));
							}
							int signature = static_cast<int>(va_args.data<Iterator>()->ctx.size());

							if (UNLIKELY(!plugin->call(func_name, signature, cursor))) {
								error("no function '%s' taking %d arguments found in plugin '%s'", func_name.c_str(), signature, plugin->getPath().c_str());
							}
						}));
}
