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
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/scheduler/processor.h"
#include "mint/system/async_io.h"
#include <cassert>
#include <chrono>
#include <utility>

namespace {

mint::Reference mint_scheduler_new(mint::Cursor& cursor) {
	return mint::create_c_object(cursor.ast(), new mint::AsyncRuntime());
}

mint::Reference mint_scheduler_delete(mint::Cursor& /*cursor*/, const mint::Reference& scheduler) {
	delete scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr;
	return {};
}

mint::Reference mint_scheduler_watch(mint::Cursor& /*cursor*/, const mint::Reference& scheduler,
    const mint::Reference& handle) {
	return mint::create_number(
	    scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr->register_handle(mint::to_handle(handle)).value());
}

mint::Reference mint_scheduler_submit(mint::Cursor& /*cursor*/, const mint::Reference& scheduler,
    const mint::Reference& handle) {
	return mint::create_boolean(scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr->submit(
	    *handle.data<mint::LibObject<mint::MintAsyncOperation>>().ptr));
}

mint::Reference mint_scheduler_cancel(mint::Cursor& /*cursor*/, const mint::Reference& scheduler,
    const mint::Reference& handle) {
	return mint::create_boolean(scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr->cancel(
	    *handle.data<mint::LibObject<mint::MintAsyncOperation>>().ptr));
}

mint::Reference mint_scheduler_poll(mint::Cursor& cursor, const mint::Reference& scheduler,
    const mint::Reference& timeout) {
	{
		const auto _ = mint::ProcessorUnlocker();
		if (const auto* operation = dynamic_cast<mint::MintAsyncOperation*>(
		        scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr->poll(
		            std::chrono::milliseconds(mint::to_integer<std::chrono::milliseconds::rep>(cursor, timeout))))) {
			return mint::create_iterator_from(cursor, operation->self(), operation->get());
		}
	}
	return {};
}

}

MINT_RAW_FUNCTION(mint_scheduler_spawn, 1, cursor) {

	auto coroutine = cursor.stack().back();
	cursor.stack().pop_back();

	if (mint::is_instance_of(coroutine, mint::Data::Format::coroutine)) {
		coroutine.data<mint::Coroutine>().call(cursor, mint::Reference(coroutine));
	}
	else {
		cursor.stack().emplace_back(mint::create_none());
	}
}

MINT_RAW_FUNCTION(mint_scheduler_current_coroutine, 0, cursor) {

	cursor.exit_call();

	if (cursor.is_in_coroutine()) {
		cursor.stack().emplace_back(cursor.coroutine());
	}
	else {
		cursor.stack().emplace_back(mint::create_none());
	}
}

MINT_RAW_FUNCTION(mint_scheduler_suspend, 1, cursor) {

	auto coroutine = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	cursor.exit_call();

	assert(mint::is_instance_of(coroutine, mint::Data::Format::coroutine));
	coroutine.data<mint::Coroutine>().suspend(cursor);
	cursor.stack().emplace_back(mint::create_none());
}

MINT_RAW_FUNCTION(mint_scheduler_raise, 2, cursor) {

	auto error = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	auto coroutine = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	// cursor.exit_call();

	assert(mint::is_instance_of(coroutine, mint::Data::Format::coroutine));
	coroutine.data<mint::Coroutine>().resume(cursor);

	if (auto exception = cursor.take_exception()) {
		assert(exception->caught);
		assert(&exception->object.data() == &error.data());
		cursor.raise(mint::Reference(error), std::move(exception));
	}
	else {
		cursor.raise(mint::Reference(error));
	}
}

MINT_RAW_FUNCTION(mint_scheduler_resume, 1, cursor) {

	auto coroutine = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	assert(mint::is_instance_of(coroutine, mint::Data::Format::coroutine));
	coroutine.data<mint::Coroutine>().resume(cursor);
}

MINT_EXPORT_FUNCTION(mint_scheduler_new, 0);
MINT_EXPORT_FUNCTION(mint_scheduler_delete, 1);
MINT_EXPORT_FUNCTION(mint_scheduler_watch, 2);
MINT_EXPORT_FUNCTION(mint_scheduler_submit, 2);
MINT_EXPORT_FUNCTION(mint_scheduler_cancel, 2);
MINT_EXPORT_FUNCTION(mint_scheduler_poll, 2);
