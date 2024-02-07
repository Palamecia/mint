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

#include <mint/memory/functiontool.h>
#include <mint/memory/operatortool.h>
#include <mint/memory/memorytool.h>
#include <mint/memory/casttool.h>
#include <mint/ast/abstractsyntaxtree.h>
#include <mint/scheduler/scheduler.h>
#include <mint/scheduler/processor.h>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_future_start_member, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &args = helper.pop_parameter();
	Reference &method = helper.pop_parameter();
	Reference &object = helper.pop_parameter();


	if (Scheduler *scheduler = Scheduler::instance()) {
		
		Cursor *thread_cursor = cursor->ast()->create_cursor();
		int signature = static_cast<int>(args.data<Iterator>()->ctx.size());
		
		if (Class::MemberInfo *info = find_member_info(object.data<Object>(), method)) {
			thread_cursor->waiting_calls().emplace(std::move(method));
			thread_cursor->waiting_calls().top().set_metadata(info->owner);
		}
		else {
			Class *owner = nullptr;
			thread_cursor->waiting_calls().emplace(get_member(thread_cursor, object, Symbol(to_string(method)), &owner));
			thread_cursor->waiting_calls().top().set_metadata(owner);
		}

		thread_cursor->stack().emplace_back(std::move(object));
		thread_cursor->stack().insert(thread_cursor->stack().end(),
									  std::make_move_iterator(args.data<Iterator>()->ctx.begin()),
									  std::make_move_iterator(args.data<Iterator>()->ctx.end()));


		call_member_operator(thread_cursor, signature);
		helper.return_value(create_object(new future<WeakReference>(scheduler->create_async(thread_cursor))));
	}
}

MINT_FUNCTION(mint_future_start, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &args = helper.pop_parameter();
	Reference &func = helper.pop_parameter();


	if (Scheduler *scheduler = Scheduler::instance()) {
		
		Cursor *thread_cursor = cursor->ast()->create_cursor();
		int signature = static_cast<int>(args.data<Iterator>()->ctx.size());
		
		thread_cursor->waiting_calls().emplace(std::move(func));
		thread_cursor->stack().insert(thread_cursor->stack().end(),
									  std::make_move_iterator(args.data<Iterator>()->ctx.begin()),
									  std::make_move_iterator(args.data<Iterator>()->ctx.end()));

		call_operator(thread_cursor, signature);
		helper.return_value(create_object(new future<WeakReference>(scheduler->create_async(thread_cursor))));
	}
}

MINT_FUNCTION(mint_future_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	delete d_ptr.data<LibObject<future<WeakReference>>>()->impl;
}

MINT_FUNCTION(mint_future_wait_for, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &time = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();

	if (d_ptr.data<LibObject<future<WeakReference>>>()->impl->valid()) {
		unlock_processor();
		switch (d_ptr.data<LibObject<future<WeakReference>>>()->impl->wait_for(chrono::milliseconds(to_integer(cursor, time)))) {
		case std::future_status::deferred:
		case std::future_status::timeout:
			lock_processor();
			helper.return_value(create_boolean(false));
			break;
		case std::future_status::ready:
			lock_processor();
			helper.return_value(create_boolean(true));
		}
	}
	else {
		helper.return_value(create_boolean(true));
	}
}

MINT_FUNCTION(mint_future_wait, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	if (d_ptr.data<LibObject<future<WeakReference>>>()->impl->valid()) {
		unlock_processor();
		d_ptr.data<LibObject<future<WeakReference>>>()->impl->wait();
		lock_processor();
	}
}

MINT_FUNCTION(mint_future_is_valid, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(create_boolean(d_ptr.data<LibObject<future<WeakReference>>>()->impl->valid()));
}

MINT_FUNCTION(mint_future_get, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(std::move(d_ptr.data<LibObject<future<WeakReference>>>()->impl->get()));
}
