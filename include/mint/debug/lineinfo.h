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

#ifndef MINT_LINEINFO_H
#define MINT_LINEINFO_H

#include "mint/ast/module.h"

#include <string>
#include <vector>

namespace mint {

class AbstractSyntaxTree;

class MINT_EXPORT LineInfo {
public:
	LineInfo(AbstractSyntaxTree *ast, const std::string &module, size_t line_number = 0);
	LineInfo(Module::Id moduleId, const std::string &module, size_t line_number = 0);
	LineInfo();

	Module::Id module_id() const;
	std::string module_name() const;
	size_t line_number() const;
	std::string to_string() const;

	std::string system_path() const;
	std::string system_file_name() const;

private:
	Module::Id m_module_id;
	std::string m_module_name;
	size_t m_line_number;
};

typedef std::vector<LineInfo> LineInfoList;

}

#endif // MINT_LINEINFO_H
