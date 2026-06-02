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

#include "mint/debug/line_info.h"
#include "mint/ast/module.h"
#include "mint/debug/debug_info.h"
#include "mint/debug/debug_tools.h"
#include "mint/ast/abstract_syntax_tree.h"
#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <utility>

using namespace mint;

LineInfo::LineInfo(AbstractSyntaxTree& ast, std::string module, std::size_t line_number) :
    _module_id(ast.module_info(module).id),
    _module_name(std::move(module)),
    _line_number(line_number) {}

LineInfo::LineInfo(mint::Module::Id module_id, std::string module, std::size_t line_number) :
    _module_id(module_id),
    _module_name(std::move(module)),
    _line_number(line_number) {}

LineInfo::LineInfo() :
    _module_id(Module::invalid_id),
    _module_name("<unknown>"),
    _line_number(0) {}

Module::Id LineInfo::module_id() const {
	return _module_id;
}

std::string LineInfo::module_name() const {
	return _module_name;
}

std::size_t LineInfo::line_number() const {
	return _line_number;
}

namespace {

std::string execution_location(const DebugInfo* debug_info, const std::string& module_name, std::size_t line_number) {

	if (debug_info == nullptr) {
		return module_name;
	}

	if (const auto* function = debug_info->find_function_from_line_number(line_number)) {
		return function->name + "()";
	}

	return module_name;
}

}

std::string LineInfo::to_string(const AbstractSyntaxTree& ast) const {
	auto module_path = to_system_path(_module_name).string();
	if (module_path.empty()) {
		module_path = "unknown";
	}
	if (_line_number) {
		const auto location = execution_location(ast.find_debug_info(_module_id), _module_name, _line_number);
		return std::format("at {} ({}:{})", location, module_path, _line_number);
	}
	return std::format("at {} ({})", _module_name, module_path);
}

std::filesystem::path LineInfo::system_path() const {
	return to_system_path(_module_name);
}

std::filesystem::path LineInfo::system_file_name() const {
	return system_path().filename();
}
