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

#include "mint/debug/debuginfo.h"
#include "mint/ast/module.h"

using namespace std;
using namespace mint;

size_t DebugInfo::line_number(size_t offset) {

	if (m_lines.empty()) {
		return 1;
	}

	auto line = m_lines.upper_bound(offset);

	if (line != m_lines.begin()) {
		--line;
	}

	return line->second;
}

void DebugInfo::new_line(size_t offset, size_t line_number) {
	auto [it, inserted] = m_lines.emplace(offset, line_number);
	if (!inserted) {
		it->second = line_number;
	}
}

void DebugInfo::new_line(Module *module, size_t line_number) {
	auto [it, inserted] = m_lines.emplace(module->next_node_offset(), line_number);
	if (!inserted) {
		it->second = line_number;
	}
}

size_t DebugInfo::to_executable_line_number(size_t line_number) {
	set<size_t> executable_line_numbers;
	for (auto [_, executable_line_number] : m_lines) {
		if (executable_line_number == line_number) {
			return executable_line_number;
		}
		executable_line_numbers.insert(executable_line_number);
	}
	if (auto it = executable_line_numbers.lower_bound(line_number); it != executable_line_numbers.end()) {
		return *it;
	}
	return 0;
}
