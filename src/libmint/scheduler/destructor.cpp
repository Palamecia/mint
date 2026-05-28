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

#include "mint/scheduler/destructor.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/processor.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/memory/operator_tools.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/assert.h"
#include <cassert>
#include <functional>
#include <memory>

using namespace mint;

namespace {

auto make_thread(Scheduler& scheduler) {
	return std::make_unique<Cursor>(scheduler.ast());
}

}

Destructor::Destructor(Scheduler& scheduler, Object* object, const Reference& member, Class& owner,
    const Process* process) :
    Process(scheduler, process ? process->cursor().make_thread() : make_thread(scheduler)),
    _owner(owner),
    _object(object),
    _member(member) {
	if (process) {
		set_thread_id(process->get_thread_id());
	}
}

Destructor::Destructor(Scheduler& scheduler, Object* object, const Reference& member, Class& owner,
    const Process& process) :
    Process(scheduler, process.cursor().make_thread()),
    _owner(owner),
    _object(object),
    _member(member) {
	set_thread_id(process.get_thread_id());
}

Destructor::Destructor(Scheduler& scheduler, Object* object, const Reference& member, Class& owner) :
    Process(scheduler, std::make_unique<Cursor>(scheduler.ast())),
    _owner(owner),
    _object(object),
    _member(member) {}

Destructor::~Destructor() {}

void Destructor::setup() {
	auto _ = ProcessorLocker();
	assert(_member.data().format() == Data::Format::function);
	cursor().stack().emplace_back(Reference::default_flags, *_object);
	cursor().waiting_calls().emplace(_member, _owner);
	call_member_operator(cursor(), 0);
}

void Destructor::cleanup() {
	auto _ = ProcessorLocker();
	cursor().stack().pop_back();                   // Pop destructor result
	GarbageCollector::instance().destroy(_object); // Free memory owned by object
}

bool mint::is_destructor(Process& process) {
	return dynamic_cast<Destructor*>(&process) != nullptr;
}
