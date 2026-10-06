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

#ifndef MINT_COMPILER_COMPILER_H
#define MINT_COMPILER_COMPILER_H

#include "mint/compiler/descriptions.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/config.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/program/node.h"
#include "mint/system/data_stream.h"
#include "mint/program/module.h"
#include <concepts>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace mint {

class MINT_EXPORT Compiler {
	std::reference_wrapper<Program> _program;
	std::reference_wrapper<ModuleInfo> _data;
	bool _printing = false;
public:
	enum class DataHint : std::uint8_t {
		data_unknown_hint,
		data_number_hint,
		data_string_hint,
		data_regex_hint,
		data_true_hint,
		data_false_hint,
		data_null_hint,
		data_none_hint,
	};

	Compiler(Program& program, ModuleInfo& data);

	[[nodiscard]] bool is_printing() const;
	void set_printing(bool enabled);

	bool build(DataStream& stream);

	Reference* make_constant(const std::string& token, DataHint hint);
	Data* make_data(const std::string& token, DataHint hint);

	template<std::derived_from<Data> T, typename... Args>
	Reference* make_constant(Args&&... args) {
		return data().bytecode.make_constant(make_data<T>(std::forward<Args>(args)...));
	}

	template<std::derived_from<Data> T, typename... Args>
	T* make_data(Args&&... args) {
		auto* data = GarbageCollector::instance().alloc<T>(std::forward<Args>(args)...);
		if constexpr (std::derived_from<T, Object>) {
			data->construct();
		}
		return data;
	}

	void push_node(const Node& node);
	void push_nodes(const std::vector<Node>& nodes);

	Program& program();
	ModuleInfo& data();
};

template<>
Library* Compiler::make_data<Library>(const std::string& token);

}

#endif // MINT_COMPILER_COMPILER_H
