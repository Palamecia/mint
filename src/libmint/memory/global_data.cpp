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

#include "mint/memory/global_data.h"
#include "mint/compiler/descriptions.h"
#include "mint/program/program.h"
#include "mint/program/symbol.h"
#include <algorithm>
#include <string>

using namespace mint;

mint::PackageData::PackageData(Program& program, const PackageDescription& description) :
    _symbols(program.global_data()),
    _program(program),
    _description(description) {}

std::string mint::PackageData::full_name() const {
	return _description.get().full_name();
}

Symbol mint::PackageData::name() const {
	return _description.get().name();
}

const PackageDescription& mint::PackageData::get_description() const {
	return _description;
}

void PackageData::mark() {
	_symbols.mark();
}

GlobalData::GlobalData(Program& program, GlobalDataDescription& description) :
    PackageData(program, description) {}

void GlobalData::cleanup_builtin() {
	// cleanup builtin classes
	std::ranges::for_each(_builtin, [](auto& builtin) {
		builtin.reset();
	});
}
