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

#include "mint/scheduler/exception.h"
#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/processor.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/casttool.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/assert.h"
#include "mint/system/error.h"
#include <cassert>
#include <utility>

using namespace mint;

Exception::Exception(Reference&& reference, const Process& process) :
    Process(process.cursor().make_thread()),
    _reference(std::move(reference)),
    _handled(false) {
	set_thread_id(process.get_thread_id());
}

Exception::~Exception() {}

void Exception::setup() {

	auto _ = ProcessorLocker();

	if (is_instance_of(_reference, Data::Format::object)) {

		auto& object = _reference.data<Object>();
		auto& metadata = object.metadata;

		if (WeakReference* data = object.data) {
			if (auto* member = metadata.find_member(builtin_symbols::show_method)) {
				WeakReference handler = Class::MemberInfo::get(*member, data);
				if (is_instance_of(handler, Data::Format::function)) {
					auto* scheduler = Scheduler::instance();
					assert_x(scheduler, __func__, "execution should be done using a scheduler");
					call_error_callbacks(to_string(scheduler->invoke(_reference, Symbol("toString"))));
					cursor().stack().emplace_back(std::forward<Reference>(_reference));
					cursor().waiting_calls().emplace(std::forward<Reference>(handler), member->owner);
					call_member_operator(cursor(), 0);
					_handled = true;
				}
			}
		}
	}
}

void Exception::cleanup() {

	if (_handled) {
		call_exit_callback();
	}
	else {
		lock_processor();
		error("exception : {}", to_string(_reference));
	}
}

bool mint::is_exception(Process& process) {
	return dynamic_cast<Exception*>(&process) != nullptr;
}
