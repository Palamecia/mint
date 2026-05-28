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

#ifndef MINT_COMPILER_COMPILER_H
#define MINT_COMPILER_COMPILER_H

#include "mint/ast/class_register.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/system/data_stream.h"
#include "mint/ast/module.h"
#include <cstdint>
#include <functional>
#include <string>

namespace mint {

class MINT_EXPORT Compiler {
public:
	enum class DataHint : std::uint8_t {
		data_unknown_hint,
		data_number_hint,
		data_string_hint,
		data_regex_hint,
		data_true_hint,
		data_false_hint,
		data_null_hint,
		data_none_hint
	};

	Compiler(AbstractSyntaxTree& ast);

	[[nodiscard]] bool is_printing() const;
	void set_printing(bool enabled);

	bool build(DataStream& stream, ModuleInfo& node);

	Data* make_data(const std::string& token, DataHint hint);
	Data& make_library(const std::string& token);
	static Data& make_package(PackageData& package);
	static Data& make_number(double value);
	static Data& make_boolean(bool value);
	Data& make_array();
	Data& make_hash();
	static Data& make_none();

	AbstractSyntaxTree& ast();

private:
	std::reference_wrapper<AbstractSyntaxTree> _ast;
	bool _printing = false;
};

}

#endif // MINT_COMPILER_COMPILER_H
