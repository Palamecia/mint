/**
 * Copyright (c) 2025 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/memory/builtin/library.h"
#include "config.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/plugin.h"
#include "mint/system/error.h"
#include <utility>

using namespace mint;

LibraryClass *LibraryClass::instance() {
	return GlobalData::instance()->builtin<LibraryClass>(Class::LIBRARY);
}

Library::Library() :
	Object(LibraryClass::instance()),
	plugin(nullptr) {}

Library::Library(Library &&other) noexcept :
	Object(LibraryClass::instance()),
	plugin(other.plugin) {
	other.plugin = nullptr;
}

Library::Library(const Library &other) :
	Object(LibraryClass::instance()),
	plugin(other.plugin ? new Plugin(other.plugin->get_path()) : nullptr) {}

Library &Library::operator=(Library &&other) noexcept {
	std::swap(plugin, other.plugin);
	return *this;
}

Library &Library::operator=(const Library &other) {
	if (UNLIKELY(this == &other)) {
		return *this;
	}
	delete plugin;
	plugin = other.plugin ? new Plugin(other.plugin->get_path()) : nullptr;
	return *this;
}

Library::~Library() {
	delete plugin;
}

LibraryClass::LibraryClass() :
	Class("lib", Class::LIBRARY) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();

	create_builtin_member(NEW_OPERATOR, ast->create_builtin_method(this, 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		const Reference &name = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);

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

	create_builtin_member("call", ast->create_builtin_method(this, VARIADIC 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		WeakReference va_args = move_from_stack(cursor, base);
		WeakReference function = move_from_stack(cursor, base - 1);
		WeakReference self = move_from_stack(cursor, base - 2);

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();

		std::string func_name = to_string(function);
		Plugin *plugin = self.data<Library>()->plugin;

		const int signature = static_cast<int>(va_args.data<Iterator>()->ctx.size());
		for (Iterator::Context::value_type &arg : va_args.data<Iterator>()->ctx) {
			cursor->stack().emplace_back(std::forward<Reference>(arg));
		}

		if (UNLIKELY(!plugin->call(func_name, signature, cursor))) {
			error("no function '%s' taking %d arguments found in plugin '%s'", func_name.c_str(), signature,
				  plugin->get_path().c_str());
		}
	}));
}
