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

#ifndef MINT_PROGRAM_PROGRAM_H
#define MINT_PROGRAM_PROGRAM_H

#include "mint/program/function_literal.h"
#include "mint/program/module.h"
#include "mint/config.h"
#include "mint/debug/debug_info.h"
#include "mint/memory/global_data.h"

#include <concepts>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <mutex>

namespace mint {

class Cursor;
class Class;

class MINT_EXPORT Program {
	friend class Cursor;
public:
	Program();
	Program(Program&& other) = delete;
	Program(const Program& other) = delete;
	~Program();

	Program& operator=(Program&& other) = delete;
	Program& operator=(const Program& other) = delete;

	using GlobalBuiltinMethod = std::add_pointer_t<void(Class&, Cursor&)>;
	using BuiltinMethod = std::add_pointer_t<void(Cursor&)>;

	std::pair<int, FunctionHandle&> create_global_builtin_method(Class& type, int signature, GlobalBuiltinMethod method);
	std::pair<int, FunctionHandle&> create_builtin_method(const Class& type, const FunctionLiteral& method);
	std::pair<int, FunctionHandle&> create_builtin_method(const Class& type, int signature, BuiltinMethod method);
	std::pair<int, FunctionHandle&> create_builtin_async_method(const Class& type, int signature, BuiltinMethod method);
	void call_global_builtin_method(std::size_t method, Cursor& cursor);
	inline void call_builtin_method(std::size_t method, Cursor& cursor);

	ModuleInfo& main();
	ModuleInfo& create_module(Module::State state);
	ModuleInfo& create_main_module(Module::State state);
	ModuleInfo& create_module_from_file_path(const std::filesystem::path& file_path, Module::State state);
	ModuleInfo& load_module(const std::string& module_name);
	const ModuleInfo& module_info(const std::string& module_name);

	template<std::derived_from<Module> UniqueModule>
	UniqueModule& unique_module();

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
	std::unordered_map<std::type_index, std::unique_ptr<Module>> _unique_modules;
	std::vector<std::reference_wrapper<ModuleInfo>> _builtin_modules;
	std::vector<GlobalBuiltinMethod> _global_builtin_methods;
	std::vector<BuiltinMethod> _builtin_methods;
};

void Program::call_builtin_method(std::size_t method, Cursor& cursor) {
	_builtin_methods[method](cursor);
}

template<std::derived_from<Module> UniqueModule>
inline UniqueModule& Program::unique_module() {
	const std::type_index type_index = std::type_index(typeid(UniqueModule));
	if (const auto it = _unique_modules.find(type_index); it != _unique_modules.end()) {
		return static_cast<UniqueModule&>(*it->second);
	}
	return static_cast<UniqueModule&>(
	    *_unique_modules.emplace(type_index, std::make_unique<UniqueModule>(*this)).first->second);
}

const Module* Program::find_module(Module::Id module_id) const {
	return (module_id < _modules.size()) ? &_modules.at(module_id).bytecode : nullptr;
}

const DebugInfo* Program::find_debug_info(Module::Id module_id) const {
	return (module_id < _modules.size()) ? &_modules.at(module_id).debug_info : nullptr;
}

const DebugInfo* Program::find_debug_info(const Module& module) const {
	return find_debug_info(get_module_id(module));
}

const GlobalData& Program::global_data() const {
	return _global_data;
}

GlobalData& Program::global_data() {
	return _global_data;
}

}

#endif // MINT_PROGRAM_PROGRAM_H
