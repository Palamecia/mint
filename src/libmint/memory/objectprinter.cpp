/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include "mint/memory/objectprinter.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/reference.h"
#include "mint/memory/object.h"
#include "mint/system/error.h"
#include "mint/ast/module.h"
#include "mint/ast/cursor.h"

using namespace mint;
using namespace std;

class ResultHandler : public Module {
public:
	ResultHandler() {
		push_nodes({Node::unload_reference, Node::module_end});
	}

	static ResultHandler &instance() {
		static ResultHandler g_instance;
		return g_instance;
	}
};

ObjectPrinter::ObjectPrinter(Cursor *cursor, Reference::Flags flags, Object *object) :
	m_object(flags, object),
	m_cursor(cursor) {

}

void ObjectPrinter::print(Reference &reference) {

	m_cursor->stack().emplace_back(WeakReference::share(m_object));
	m_cursor->stack().emplace_back(WeakReference::share(reference));
	m_cursor->call(&ResultHandler::instance(), 0, GlobalData::instance());
	
	if (UNLIKELY(!call_overload(m_cursor, Symbol::write_method, 1))) {
		m_cursor->exit_module();
		string type = type_name(m_object);
		error("class '%s' dosen't ovreload 'write'(1)", type.c_str());
	}
}
