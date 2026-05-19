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
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/object.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/casttool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/processor.h"
#include <chrono>
#include <future>
#include <memory>
#include <utility>

namespace {

mint::Reference mint_promise_start_member(mint::FunctionHelper& helper, const mint::Reference& object,
    const mint::Reference& method, const mint::Reference& args) {

	mint::Scheduler& scheduler = helper.scheduler();
	auto thread_cursor = std::make_unique<mint::Cursor>(scheduler.ast());
	const auto signature = static_cast<int>(args.data<mint::Iterator>().ctx.size());

	if (const auto* info = find_member_info(object.data<mint::Object>(), method)) {
		thread_cursor->waiting_calls().emplace(method, info->owner);
	}
	else {
		auto [member, owner] = mint::get_member(*thread_cursor, object, mint::Symbol(to_string(method)));
		thread_cursor->waiting_calls().emplace(std::move(member), owner);
	}

	thread_cursor->stack().emplace_back(object);
	thread_cursor->stack().append_range(args.data<mint::Iterator>().ctx);

	mint::call_member_operator(*thread_cursor, signature);
	return mint::create_c_object(helper.cursor().ast(),
	    new std::future<mint::Reference>(scheduler.create_async_thread(std::move(thread_cursor))));
}

mint::Reference mint_promise_start(mint::FunctionHelper& helper, const mint::Reference& func,
    const mint::Reference& args) {

	mint::Scheduler& scheduler = helper.scheduler();
	auto thread_cursor = std::make_unique<mint::Cursor>(scheduler.ast());
	const auto signature = static_cast<int>(args.data<mint::Iterator>().ctx.size());

	thread_cursor->waiting_calls().emplace(func);
	thread_cursor->stack().append_range(args.data<mint::Iterator>().ctx);

	mint::call_operator(*thread_cursor, signature);
	return create_c_object(helper.cursor().ast(),
	    new std::future<mint::Reference>(scheduler.create_async_thread(std::move(thread_cursor))));
}

mint::Reference mint_promise_spawn(mint::FunctionHelper& helper, const mint::Reference& coroutine) {

	mint::Scheduler& scheduler = helper.scheduler();
	auto thread_cursor = std::make_unique<mint::Cursor>(scheduler.ast());

	if (mint::is_instance_of(coroutine, mint::Data::Format::coroutine)) {
		coroutine.data<mint::Coroutine>().call(*thread_cursor, mint::Reference(coroutine));
	}
	else {
		thread_cursor->stack().emplace_back(coroutine);
	}

	return create_c_object(helper.cursor().ast(),
	    new std::future<mint::Reference>(scheduler.create_async_thread(std::move(thread_cursor))));
}

mint::Reference mint_promise_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<std::future<mint::Reference>>>().ptr;
	return {};
}

mint::Reference mint_promise_wait_for(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& time) {
	if (auto* promise = d_ptr.data<mint::LibObject<std::future<mint::Reference>>>().ptr; promise->valid()) {
		mint::unlock_processor();
		switch (promise->wait_for(std::chrono::milliseconds(mint::to_signed_integer(cursor, time)))) {
		case std::future_status::deferred:
		case std::future_status::timeout:
			mint::lock_processor();
			return mint::create_boolean(false);
		case std::future_status::ready:
			mint::lock_processor();
			return mint::create_boolean(true);
		}
	}
	return mint::create_boolean(true);
}

mint::Reference mint_promise_wait(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	if (auto* promise = d_ptr.data<mint::LibObject<std::future<mint::Reference>>>().ptr; promise->valid()) {
		mint::unlock_processor();
		promise->wait();
		mint::lock_processor();
	}
	return {};
}

mint::Reference mint_promise_is_valid(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_boolean(d_ptr.data<mint::LibObject<std::future<mint::Reference>>>().ptr->valid());
}

mint::Reference mint_promise_get(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return d_ptr.data<mint::LibObject<std::future<mint::Reference>>>().ptr->get();
}

}

MINT_EXPORT_FUNCTION(mint_promise_start_member, 3)
MINT_EXPORT_FUNCTION(mint_promise_start, 2)
MINT_EXPORT_FUNCTION(mint_promise_spawn, 1)
MINT_EXPORT_FUNCTION(mint_promise_delete, 1)
MINT_EXPORT_FUNCTION(mint_promise_wait_for, 2)
MINT_EXPORT_FUNCTION(mint_promise_wait, 1)
MINT_EXPORT_FUNCTION(mint_promise_is_valid, 1)
MINT_EXPORT_FUNCTION(mint_promise_get, 1)
