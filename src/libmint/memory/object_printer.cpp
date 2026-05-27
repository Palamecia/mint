/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/memory/object_printer.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/ast/symbol.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/operator_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/error.h"

using namespace mint;

class ResultHandler : public Module {
public:
	ResultHandler() {
		push_nodes({Node::Command::unload_reference, Node::Command::exit_module});
	}

	static ResultHandler& instance() {
		static ResultHandler g_instance;
		return g_instance;
	}
};

ObjectPrinter::ObjectPrinter(Cursor& cursor, Reference::Flags flags, Object& object) :
    _object(flags, object),
    _cursor(cursor) {}

void ObjectPrinter::print(const Reference& reference) {

	_cursor.get().stack().emplace_back(_object);
	_cursor.get().stack().emplace_back(reference);
	_cursor.get().call(ResultHandler::instance(), 0uz, _cursor.get().ast().global_data());

	if (!call_overload(_cursor.get(), builtin_symbols::write_method, 1)) [[unlikely]] {
		_cursor.get().exit_module();
		error("class '{}' doesn't overload 'write'(1)", type_name(_object));
	}
}
