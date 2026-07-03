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

#ifndef MINT_AST_MODULE_H
#define MINT_AST_MODULE_H

#include "mint/ast/class_register.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/ast/node.h"
#include "mint/debug/debug_info.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/reference.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mint {

class AbstractSyntaxTree;
class ClassDescription;
class PackageData;
class parser;

struct FunctionHandle;

class MINT_EXPORT Module : public ClassRegister, public MemoryRoot {
	friend class AbstractSyntaxTree;
	friend class MainBranch;
	friend class BubBranch;
public:
	using Id = std::size_t;

	static constexpr const char* invalid_name = "unknown";
	static constexpr const Id invalid_id = std::numeric_limits<std::size_t>::max();

	static constexpr const char* main_name = "main";
	static constexpr const Id main_id = 0;

	enum class State : std::uint8_t {
		not_compiled,
		not_loaded,
		ready
	};

	Module(AbstractSyntaxTree& ast);
	Module(Module&& other) noexcept;
	Module(const Module& other) = delete;
	~Module();

	Module& operator=(Module&& other) noexcept;
	Module& operator=(const Module& other) = delete;

	[[nodiscard]] inline const Node& node_at(std::size_t idx) const;
	[[nodiscard]] inline Node& node_at(std::size_t idx);
	[[nodiscard]] inline std::size_t end() const;
	[[nodiscard]] inline std::size_t next_node_offset() const;

	[[nodiscard]] FunctionHandle* find_handle(std::size_t offset) const;
	FunctionHandle& get_handle(PackageData& package, std::size_t offset);
	FunctionHandle& make_handle(PackageData& package, std::size_t offset);
	FunctionHandle& make_builtin_handle(PackageData& package, std::size_t offset);
	FunctionHandle& make_builtin_async_handle(PackageData& package, std::size_t offset);

	template<std::derived_from<Data> Type, typename... Args>
	Reference* make_constant(Args&&... args);
	Reference* make_constant(Data& data);
	Symbol* make_symbol(const std::string& name);
	ClassDescription* make_class(AbstractSyntaxTree& ast, const std::string& name);

	void add_internal_register(std::unique_ptr<ClassRegister>&& class_register);

	void cleanup_memory() override;
	void cleanup_metadata() override;

	void mark() override;

protected:
	void push_node(const Node& node);
	void push_nodes(const std::vector<Node>& nodes);
	void push_nodes(const std::initializer_list<Node>& nodes);
	void replace_node(std::size_t offset, const Node& node);

private:
	std::vector<Node> _tree;
	std::vector<std::unique_ptr<FunctionHandle>> _handles;
	std::vector<std::unique_ptr<Reference>> _constants;
	std::vector<std::unique_ptr<ClassDescription>> _classes;
	std::vector<std::unique_ptr<ClassRegister>> _internal_registers;
	std::unordered_map<std::string, std::unique_ptr<Symbol>> _symbols;
};

struct ModuleInfo {
	Module bytecode;
	DebugInfo debug_info;
	Module::Id id = Module::invalid_id;
	Module::State state = Module::State::not_compiled;
};

struct FunctionHandle {
	Module& module;
	std::size_t offset;
	PackageData& package;
	std::size_t fast_count;
	bool symbols: 1 = false;
	bool generator: 1 = false;
	bool async: 1 = false;
};

const Node& Module::node_at(std::size_t idx) const {
	return _tree[idx];
}

Node& Module::node_at(std::size_t idx) {
	return _tree[idx];
}

std::size_t Module::end() const {
	return _tree.size() - 1;
}

std::size_t Module::next_node_offset() const {
	return _tree.size();
}

template<std::derived_from<Data> Type, typename... Args>
Reference* Module::make_constant(Args&&... args) {
	return _constants
	    .emplace_back(std::make_unique<Reference>(Reference::const_address | Reference::const_value,
	        std::in_place_type<Type>, std::forward<Args>(args)...))
	    .get();
}

}

#endif // MINT_AST_MODULE_H
