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

#include "mint/ast/symbol.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/reference.h"
#include "mint/ast/cursor.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/casttool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/processor.h"
#include "mint/system/errno.h"

#include <chrono>
#include <memory>
#include <system_error>
#include <thread>
#include <utility>

namespace {

std::thread* get_thread_handle(const mint::Scheduler& scheduler, mint::Process::ThreadId thread_id) {
	if (const mint::Process* thread = scheduler.find_thread(thread_id)) {
		return thread->get_thread_handle();
	}
	return nullptr;
}

mint::WeakReference mint_thread_current_id(mint::Cursor& /*cursor*/) {
	if (const mint::Process* process = mint::Scheduler::current_process()) {
		return mint::create_number(process->get_thread_id());
	}
	return {};
}

mint::WeakReference mint_thread_start_member(mint::FunctionHelper& helper, const mint::Reference& object,
    const mint::Reference& method, const mint::Reference& args) {

	mint::Scheduler& scheduler = helper.scheduler();
	auto thread_cursor = std::make_unique<mint::Cursor>(scheduler.ast());
	const auto signature = static_cast<int>(args.data<mint::Iterator>().ctx.size());

	if (auto* info = find_member_info(object.data<mint::Object>(), method)) {
		thread_cursor->waiting_calls().emplace(method, info->owner);
	}
	else {
		auto [member, owner] = mint::get_member(*thread_cursor, object, mint::Symbol(to_string(method)));
		thread_cursor->waiting_calls().emplace(std::move(member), owner);
	}

	thread_cursor->stack().emplace_back(object);
	thread_cursor->stack().append_range(args.data<mint::Iterator>().ctx);

	mint::call_member_operator(*thread_cursor, signature);
	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());
	try {
		const auto thread_id = scheduler.create_thread(std::move(thread_cursor));
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(thread_id));
	}
	catch (const std::system_error& error) {
		iterator_yield(result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}
	return result;
}

mint::WeakReference mint_thread_start(mint::FunctionHelper& helper, const mint::Reference& func,
    const mint::Reference& args) {

	mint::Scheduler& scheduler = helper.scheduler();
	auto thread_cursor = std::make_unique<mint::Cursor>(scheduler.ast());
	const auto signature = static_cast<int>(args.data<mint::Iterator>().ctx.size());

	thread_cursor->waiting_calls().emplace(func);
	thread_cursor->stack().append_range(args.data<mint::Iterator>().ctx);

	mint::call_operator(*thread_cursor, signature);
	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());
	try {
		const auto thread_id = scheduler.create_thread(std::move(thread_cursor));
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(thread_id));
	}
	catch (const std::system_error& error) {
		iterator_yield(result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}
	return result;
}

mint::WeakReference mint_thread_is_running(mint::FunctionHelper& helper, const mint::Reference& thread_id) {
	return mint::create_boolean(
	    get_thread_handle(helper.scheduler(), mint::to_integer<mint::Process::ThreadId>(helper.cursor(), thread_id))
	    != nullptr);
}

mint::WeakReference mint_thread_is_joinable(mint::FunctionHelper& helper, const mint::Reference& thread_id) {
	if (const auto* handle = get_thread_handle(helper.scheduler(),
	        mint::to_integer<mint::Process::ThreadId>(helper.cursor(), thread_id))) {
		return mint::create_boolean(handle->joinable());
	}
	return mint::create_boolean(false);
}

mint::WeakReference mint_thread_join(mint::FunctionHelper& helper, const mint::Reference& thread_id) {
	try {
		mint::Scheduler& scheduler = helper.scheduler();
		mint::unlock_processor();
		scheduler.join_thread(to_integer<mint::Process::ThreadId>(helper.cursor(), thread_id));
		mint::lock_processor();
	}
	catch (const std::system_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
	return {};
}

mint::WeakReference mint_thread_wait(mint::Cursor& /*cursor*/) {
	mint::unlock_processor();
	std::this_thread::yield();
	mint::lock_processor();
	return {};
}

mint::WeakReference mint_thread_sleep(mint::Cursor& cursor, const mint::Reference& time) {
	mint::unlock_processor();
	std::this_thread::sleep_for(std::chrono::milliseconds(mint::to_signed_integer(cursor, time)));
	mint::lock_processor();
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_thread_current_id, 0)
MINT_EXPORT_FUNCTION(mint_thread_start_member, 3)
MINT_EXPORT_FUNCTION(mint_thread_start, 2)
MINT_EXPORT_FUNCTION(mint_thread_is_running, 1)
MINT_EXPORT_FUNCTION(mint_thread_is_joinable, 1)
MINT_EXPORT_FUNCTION(mint_thread_join, 1)
MINT_EXPORT_FUNCTION(mint_thread_wait, 0)
MINT_EXPORT_FUNCTION(mint_thread_sleep, 1)
