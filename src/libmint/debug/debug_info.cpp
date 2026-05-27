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

#include "mint/debug/debug_info.h"
#include "mint/ast/module.h"
#include <cstddef>
#include <set>

using namespace mint;

std::size_t DebugInfo::line_number(std::size_t offset) {

	if (_lines.empty()) {
		return 1;
	}

	auto line = _lines.upper_bound(offset);

	if (line != _lines.begin()) {
		--line;
	}

	return line->second;
}

void DebugInfo::new_line(std::size_t offset, std::size_t line_number) {
	auto [it, inserted] = _lines.emplace(offset, line_number);
	if (!inserted) {
		it->second = line_number;
	}
}

void DebugInfo::new_line(const Module* module, std::size_t line_number) {
	auto [it, inserted] = _lines.emplace(module->next_node_offset(), line_number);
	if (!inserted) {
		it->second = line_number;
	}
}

std::size_t DebugInfo::to_executable_line_number(std::size_t line_number) {
	std::set<std::size_t> executable_line_numbers;
	for (auto [_, executable_line_number] : _lines) {
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
