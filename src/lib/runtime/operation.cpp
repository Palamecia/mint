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
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/async_io.h"
#include <cstddef>
#include <functional>
#include <system_error>
#include <utility>

namespace {

class DefaultAsyncOperation : public mint::MintAsyncOperation {
	std::reference_wrapper<mint::AsyncRuntime> _scheduler;
public:
	DefaultAsyncOperation(mint::Reference self, mint::AsyncRuntime& scheduler) :
	    mint::MintAsyncOperation(std::move(self), mint::invalid_handle),
	    _scheduler(scheduler) {}

	std::error_code start() override {
		return _scheduler.get().post_deferred_completion(*this);
	}

	void complete(std::error_code /*error*/, std::size_t /*bytes_transferred*/) override {
		done(mint::create_none());
	}
};

mint::Reference mint_operation_create(mint::Cursor& cursor, mint::Reference& self, const mint::Reference& scheduler) {
	return mint::create_async_operation(cursor.ast(),
	    new DefaultAsyncOperation(self, *scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr));
}

mint::Reference mint_operation_delete(mint::Cursor& /*cursor*/, mint::Reference& self) {
	delete self.data<mint::LibObject<mint::MintAsyncOperation>>().ptr;
	return {};
}

mint::Reference mint_operation_get(mint::Cursor& /*cursor*/, mint::Reference& self) {
	return self.data<mint::LibObject<mint::MintAsyncOperation>>().ptr->get();
}

}

MINT_EXPORT_FUNCTION(mint_operation_create, 2)
MINT_EXPORT_FUNCTION(mint_operation_delete, 1)
MINT_EXPORT_FUNCTION(mint_operation_get, 1)
