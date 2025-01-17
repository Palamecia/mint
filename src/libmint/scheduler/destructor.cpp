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

#include "mint/scheduler/destructor.h"
#include "mint/scheduler/processor.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/operatortool.h"

using namespace mint;

Destructor::Destructor(Object *object, Reference &&member, Class *owner, const Process *process) :
	Process(AbstractSyntaxTree::instance()->create_cursor(process ? process->cursor() : nullptr)),
	m_owner(owner),
	m_object(object),
	m_member(std::move(member)) {
	if (process) {
		set_thread_id(process->get_thread_id());
	}
}

Destructor::~Destructor() {}

void Destructor::setup() {
	lock_processor();
	assert(m_member.data()->format == Data::FMT_FUNCTION);
	cursor()->stack().emplace_back(Reference::DEFAULT, m_object);
	cursor()->waiting_calls().emplace(std::forward<Reference>(m_member));
	cursor()->waiting_calls().top().set_metadata(m_owner);
	call_member_operator(cursor(), 0);
	unlock_processor();
}

void Destructor::cleanup() {
	lock_processor();
	cursor()->stack().pop_back();					// Pop destructor result
	GarbageCollector::instance().destroy(m_object); // Free memory owned by object
	unlock_processor();
}

bool mint::is_destructor(Process *process) {
	return dynamic_cast<Destructor *>(process) != nullptr;
}
