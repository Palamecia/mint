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

#include "mint/scheduler/generator.h"
#include "mint/scheduler/processor.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/savedstate.h"

using namespace mint;
using namespace std;

Generator::Generator(unique_ptr<SavedState> state, Process *process) :
	Process(AbstractSyntaxTree::instance()->create_cursor(process->cursor())),
	m_state(std::move(state)) {
	set_thread_id(process->get_thread_id());
}

Generator::~Generator() {

}

void Generator::setup() {
	lock_processor();
	cursor()->restore(std::move(m_state));
	unlock_processor();
}

void Generator::cleanup() {

}

bool mint::is_generator(Process *process) {
	return dynamic_cast<Generator *>(process) != nullptr;
}
