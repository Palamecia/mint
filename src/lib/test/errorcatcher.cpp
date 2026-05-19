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

#include "mint/ast/cursor.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/casttool.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/exception.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/error.h"
#include "mint/system/mintruntimeerror.h"
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct ErrorCatcher {
	std::vector<std::pair<int, std::function<void(const std::string&)>>> error_callbacks;
	int on_error = -1;
};

mint::Reference mint_test_error_catcher_install(mint::FunctionHelper& helper) {

	auto& cursor = helper.cursor();
	auto& scheduler = helper.scheduler();
	auto& runtime_error_class = helper.reference("Test").member("RuntimeError")->data<mint::Object>().metadata;

	return mint::create_c_object(cursor.ast(),
	    new ErrorCatcher {
	        .error_callbacks = mint::take_error_callbacks(),
	        .on_error = mint::add_error_callback(
	            [&cursor, &scheduler, &runtime_error_class](const std::string& message) {
		            throw mint::MintException(cursor,
		                scheduler.invoke(runtime_error_class, mint::create_string(cursor.ast(), message)));
	            }),
	    });
}

mint::Reference mint_test_error_catcher_remove(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	mint::remove_error_callback(d_ptr.data<mint::LibObject<ErrorCatcher>>().ptr->on_error);
	mint::restore_error_callbacks(std::move(d_ptr.data<mint::LibObject<ErrorCatcher>>().ptr->error_callbacks));
	delete d_ptr.data<mint::LibObject<ErrorCatcher>>().ptr;
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_test_error_catcher_install, 0)
MINT_EXPORT_FUNCTION(mint_test_error_catcher_remove, 1)
