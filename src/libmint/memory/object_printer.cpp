/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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
#include "mint/program/program.h"
#include "mint/program/cursor.h"
#include "mint/program/module.h"
#include "mint/program/node.h"
#include "mint/program/symbol.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/operator_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/error.h"

using namespace mint;

namespace {

class ResultHandler : public Module {
public:
	explicit ResultHandler(Program& program) :
	    Module(program) {
		push_nodes({Node::Command::unload_reference, Node::Command::exit_module});
	}

	static ResultHandler& instance(Program& program) {
		return program.unique_module<ResultHandler>();
	}
};

}

ObjectPrinter::ObjectPrinter(Cursor& cursor, Reference::Flags flags, Object& object) :
    _object(flags, object),
    _cursor(cursor) {}

void ObjectPrinter::print(const Reference& reference) {

	_cursor.get().stack().emplace_back(_object);
	_cursor.get().stack().emplace_back(reference);
	_cursor.get().call(ResultHandler::instance(_cursor.get().program()), 0uz, _cursor.get().program().global_data());

	if (!call_overload(_cursor, builtin_symbols::write_method, 1)) [[unlikely]] {
		_cursor.get().exit_module();
		error("class '{}' doesn't overload 'write'(1)", type_name(_object));
	}
}
