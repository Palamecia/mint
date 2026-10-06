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

#ifndef MINT_COMPILER_CLASS_BUILDER_H
#define MINT_COMPILER_CLASS_BUILDER_H

#include "mint/compiler/descriptions.h"
#include "mint/memory/reference.h"
#include "mint/program/program.h"
#include "mint/program/symbol.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mint {

class ClassBuilder {
	std::reference_wrapper<Program> _program;
public:
	explicit ClassBuilder(Program& program);

	std::pair<std::unique_ptr<Class>, std::vector<std::reference_wrapper<Class>>> make_class(
	    const ClassDescription& description);

	[[nodiscard]] Program& program();

private:
	static std::unique_ptr<Class::MemberInfo> create_member_info(Class& metadata, const Class::MemberInfo& member);
	static Class::MemberInfo* update_member_info(Class& metadata, Symbol symbol, Reference& value,
	    std::unordered_map<Symbol, std::vector<std::reference_wrapper<const Reference>>>& member_overrides);
};

}

#endif // MINT_COMPILER_CLASS_BUILDER_H
