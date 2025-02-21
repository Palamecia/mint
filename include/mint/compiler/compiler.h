/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#ifndef MINT_COMPILER_H
#define MINT_COMPILER_H

#include "mint/system/datastream.h"
#include "mint/ast/module.h"

namespace mint {

class MINT_EXPORT Compiler {
public:
	enum DataHint : std::uint8_t {
		DATA_UNKNOWN_HINT,
		DATA_NUMBER_HINT,
		DATA_STRING_HINT,
		DATA_REGEX_HINT,
		DATA_TRUE_HINT,
		DATA_FALSE_HINT,
		DATA_NULL_HINT,
		DATA_NONE_HINT
	};

	Compiler();

	[[nodiscard]] bool is_printing() const;
	void set_printing(bool enabled);

	bool build(DataStream *stream, const Module::Info &node);

	static Data *make_data(const std::string &token, DataHint hint);
	static Data *make_library(const std::string &token);
	static Data *make_array();
	static Data *make_hash();
	static Data *make_none();

private:
	bool m_printing;
};

}

#endif // MINT_COMPILER_H
