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

#include "mint/ast/printer.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/functiontool.h"
#include "mint/ast/cursor.h"
#include "mint/memory/reference.h"

namespace {

mint::WeakReference mint_printer_write(mint::Cursor& /*cursor*/, const mint::Reference& object,
    const mint::Reference& data) {
	mint::Printer* printer = object.data<mint::LibObject<mint::Printer>>().ptr;
	printer->print(data);
	return {};
}

}

MINT_RAW_FUNCTION(mint_printer_current_handle, 0, cursor) {

	cursor.exit_call();
	cursor.exit_call();

	if (mint::Printer* printer = cursor.printer()) {
		cursor.stack().emplace_back(mint::create_c_object(cursor.ast(), printer));
	}
	else {
		cursor.stack().emplace_back(mint::create_none());
	}
}

MINT_EXPORT_FUNCTION(mint_printer_write, 2);
