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
#include <iterator>
#include <set>
#include <utility>

using namespace mint;

std::size_t DebugInfo::line_number(std::size_t offset) const {

	if (_offset_to_line.empty()) {
		return 1;
	}

	auto line = _offset_to_line.upper_bound(offset);

	if (line != _offset_to_line.begin()) {
		line = std::prev(line);
	}

	return line->second;
}

void DebugInfo::new_line(std::size_t offset, std::size_t line_number) {
	_offset_to_line.insert_or_assign(offset, line_number);
	_line_to_offset.emplace(line_number, offset);
}

void DebugInfo::new_line(const Module& module, std::size_t line_number) {
	new_line(module.next_node_offset(), line_number);
}

std::size_t DebugInfo::to_executable_line_number(std::size_t line_number) const {
	auto executable_line_numbers = std::set<std::size_t>();
	for (auto [_, executable_line_number] : _offset_to_line) {
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

const FunctionInfo* mint::DebugInfo::find_function_from_line_number(std::size_t line_number) const {
	if (const auto it = _line_to_offset.lower_bound(line_number); it != _line_to_offset.end()) {
		return find_function_from_offset(it->second);
	}
	return nullptr;
}

const mint::FunctionInfo* mint::DebugInfo::find_function_from_offset(std::size_t offset) const {
	for (const auto& function_info : _functions) {
		if (function_info.begin_offset <= offset && offset < function_info.end_offset) {
			return &function_info;
		}
	}
	return nullptr;
}

void mint::DebugInfo::register_function(FunctionInfo function_info) {
	_functions.push_back(std::move(function_info));
}
