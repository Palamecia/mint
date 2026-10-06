/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#ifndef LIBMINT_COMPILER_CASE_TABLE_H
#define LIBMINT_COMPILER_CASE_TABLE_H

#include "branch.h"
#include "mint/compiler/build_context.h"
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mint {

class Branch;
class BuildContext;

struct CaseTable {
	struct Label {
		std::vector<std::string> tokens;
	};

	struct Case {
		explicit Case(Branch& parent);

		SubBranch condition;
		std::size_t offset;
		bool terminated = false;
	};

	struct DefaultCase {
		std::size_t offset = invalid_offset;
		bool terminated = false;
	};

	std::optional<std::pair<Label, Case>> current_case;
	std::vector<std::pair<Label, Case>> cases;
	std::optional<DefaultCase> default_case;
	std::size_t origin = 0;
};

bool operator==(const CaseTable::Label& lhs, const CaseTable::Label& rhs);

}

#endif // LIBMINT_COMPILER_CASE_TABLE_H
