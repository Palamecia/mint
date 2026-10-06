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
#include "mint/system/error.h"
#include <concepts>
#include <cstdint>
#include <exception>
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

	const Reference* make_constant(const std::string& token, DataHint hint);
	Data* make_data(const std::string& token, DataHint hint);

	const Reference* make_constant(Reference::Flags flags, Data* data);

	template<std::derived_from<Data> Type, typename... Args>
	const Reference* make_constant(Args&&... args) {
		return data().bytecode.make_constant(make_data<Type>(std::forward<Args>(args)...));
	}

	template<std::derived_from<Data> Type, typename... Args>
	Type* make_data(Args&&... args) {
		if constexpr (std::is_same_v<Type, Library> && std::constructible_from<std::string, Args...>) {
			const auto token = std::string(std::forward<Args>(args)...);
			try {
				auto* data = GarbageCollector::instance().alloc<Type>(_program, token_to_string(token));
				data->construct();
				return data;
			}
			catch (const std::exception& error) {
				mint::error("failed to load plugin {}: {}", token, error.what());
			}
		}
		else if constexpr (std::derived_from<Type, Object>) {
			auto* data = GarbageCollector::instance().alloc<Type>(_program, std::forward<Args>(args)...);
			data->construct();
			return data;
		}
		else {
			return GarbageCollector::instance().alloc<Type>(std::forward<Args>(args)...);
		}
	}

	void push_node(const Node& node);
	void push_nodes(const std::vector<Node>& nodes);

	Program& program();
	ModuleInfo& data();

private:
	static std::string token_to_string(const std::string& token);
};

}

#endif // MINT_COMPILER_COMPILER_H
