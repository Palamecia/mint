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

#ifndef MINT_AST_ABSTRACT_SYNTAX_TREE_H
#define MINT_AST_ABSTRACT_SYNTAX_TREE_H

#include "mint/ast/function_literal.h"
#include "mint/ast/module.h"
#include "mint/config.h"
#include "mint/debug/debug_info.h"
#include "mint/memory/global_data.h"
#include "module.h"

#include <cstddef>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <mutex>

namespace mint {

class Cursor;
class Class;

class MINT_EXPORT AbstractSyntaxTree {
	friend class Cursor;
public:
	AbstractSyntaxTree();
	AbstractSyntaxTree(AbstractSyntaxTree&& other) = delete;
	AbstractSyntaxTree(const AbstractSyntaxTree& other) = delete;
	~AbstractSyntaxTree();

	AbstractSyntaxTree& operator=(AbstractSyntaxTree&& other) = delete;
	AbstractSyntaxTree& operator=(const AbstractSyntaxTree& other) = delete;

	using GlobalBuiltinMethod = std::add_pointer_t<void(Class&, Cursor&)>;
	using BuiltinMethod = std::add_pointer_t<void(Cursor&)>;

	std::pair<int, Module::Handle&> create_global_builtin_method(Class& type, int signature, GlobalBuiltinMethod method);
	std::pair<int, Module::Handle&> create_builtin_method(const Class& type, const FunctionLiteral& method);
	std::pair<int, Module::Handle&> create_builtin_method(const Class& type, int signature, BuiltinMethod method);
	std::pair<int, Module::Handle&> create_builtin_async_method(const Class& type, int signature, BuiltinMethod method);
	void call_global_builtin_method(std::size_t method, Cursor& cursor);
	inline void call_builtin_method(std::size_t method, Cursor& cursor);

	ModuleInfo& main();
	ModuleInfo& create_module(Module::State state);
	ModuleInfo& create_main_module(Module::State state);
	ModuleInfo& create_module_from_file_path(const std::filesystem::path& file_path, Module::State state);
	ModuleInfo& load_module(const std::string& module_name);
	const ModuleInfo& module_info(const std::string& module_name);

	[[nodiscard]] inline const Module* find_module(Module::Id module_id) const;
	[[nodiscard]] inline const DebugInfo* find_debug_info(Module::Id module_id) const;
	[[nodiscard]] inline const DebugInfo* find_debug_info(const Module& module) const;
	[[nodiscard]] std::string get_module_name(const Module& module) const;
	[[nodiscard]] Module::Id get_module_id(const Module& module) const;
	[[nodiscard]] bool is_main(const Module& module) const;

	[[nodiscard]] inline const GlobalData& global_data() const;
	[[nodiscard]] inline GlobalData& global_data();

	void cleanup_memory();
	void cleanup_metadata();
	void cleanup_modules();

protected:
	ModuleInfo& builtin_module(std::size_t module_index);

	void set_module_state(Module::Id module_id, Module::State state);

private:
	std::mutex _mutex;
	std::deque<ModuleInfo> _modules;
	std::map<std::filesystem::path, std::reference_wrapper<ModuleInfo>> _module_cache;

	GlobalData _global_data {*this};
	std::vector<std::reference_wrapper<ModuleInfo>> _builtin_modules;
	std::vector<GlobalBuiltinMethod> _global_builtin_methods;
	std::vector<BuiltinMethod> _builtin_methods;
};

void AbstractSyntaxTree::call_builtin_method(std::size_t method, Cursor& cursor) {
	_builtin_methods[method](cursor);
}

const Module* AbstractSyntaxTree::find_module(Module::Id module_id) const {
	return (module_id < _modules.size()) ? &_modules[module_id].bytecode : nullptr;
}

const DebugInfo* AbstractSyntaxTree::find_debug_info(Module::Id module_id) const {
	return (module_id < _modules.size()) ? &_modules[module_id].debug_info : nullptr;
}

const DebugInfo* AbstractSyntaxTree::find_debug_info(const Module& module) const {
	return find_debug_info(get_module_id(module));
}

const GlobalData& AbstractSyntaxTree::global_data() const {
	return _global_data;
}

GlobalData& AbstractSyntaxTree::global_data() {
	return _global_data;
}

}

#endif // MINT_AST_ABSTRACT_SYNTAX_TREE_H
