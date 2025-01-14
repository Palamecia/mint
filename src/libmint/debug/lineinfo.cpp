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

#include "mint/debug/lineinfo.h"
#include "mint/debug/debugtool.h"
#include "mint/system/filesystem.h"
#include "mint/ast/abstractsyntaxtree.h"

using namespace mint;

LineInfo::LineInfo(AbstractSyntaxTree *ast, const std::string &module, size_t line_number) :
	m_module_id(ast->module_info(module).id),
	m_module_name(module),
	m_line_number(line_number) {}

LineInfo::LineInfo(mint::Module::Id moduleId, const std::string &module, size_t line_number) :
	m_module_id(moduleId),
	m_module_name(module),
	m_line_number(line_number) {}

LineInfo::LineInfo() :
	m_module_id(Module::INVALID_ID),
	m_module_name("<unknown>"),
	m_line_number(0) {}

Module::Id LineInfo::module_id() const {
	return m_module_id;
}

std::string LineInfo::module_name() const {
	return m_module_name;
}

size_t LineInfo::line_number() const {
	return m_line_number;
}

std::string LineInfo::to_string() const {

	if (m_line_number) {
		return "Module '" + m_module_name + "', line " + std::to_string(m_line_number);
	}

	return "Module '" + m_module_name + "', line unknown";
}

std::string LineInfo::system_path() const {
	return to_system_path(m_module_name);
}

std::string LineInfo::system_file_name() const {
	const std::string &path = system_path();
	auto pos = path.rfind(FileSystem::SEPARATOR);
	if (pos != std::string::npos) {
		return path.substr(pos + 1);
	}
	return path;
}
