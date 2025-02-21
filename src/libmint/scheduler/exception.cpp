/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "mint/scheduler/processor.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/casttool.h"
#include "mint/system/error.h"

using namespace mint;

Exception::Exception(Reference &&reference, const Process *process) :
	Process(AbstractSyntaxTree::instance()->create_cursor(process->cursor())),
	m_reference(std::forward<Reference>(reference)),
	m_handled(false) {
	set_thread_id(process->get_thread_id());
}

Exception::~Exception() {}

void Exception::setup() {

	lock_processor();

	if (m_reference.data()->format == Data::FMT_OBJECT) {

		Object *object = m_reference.data<Object>();
		Class *metadata = object->metadata;

		if (WeakReference *data = object->data) {
			auto member = metadata->members().find(builtin_symbols::SHOW_METHOD);
			if (member != metadata->members().end()) {
				WeakReference handler = WeakReference::share(Class::MemberInfo::get(member->second, data));
				if (handler.data()->format == Data::FMT_FUNCTION) {
					call_error_callbacks();
					cursor()->stack().emplace_back(std::forward<Reference>(m_reference));
					cursor()->waiting_calls().emplace(std::forward<Reference>(handler));
					cursor()->waiting_calls().top().set_metadata(member->second->owner);
					call_member_operator(cursor(), 0);
					m_handled = true;
				}
			}
		}
	}

	unlock_processor();
}

void Exception::cleanup() {

	if (m_handled) {
		call_exit_callback();
	}
	else {
		lock_processor();
		error("exception : %s", to_string(m_reference).c_str());
	}
}

bool mint::is_exception(Process *process) {
	return dynamic_cast<Exception *>(process) != nullptr;
}
