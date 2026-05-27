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

#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/debug/debug_info.h"
#include "mint/memory/class.h"
#include "mint/debug/debug_tools.h"
#include "mint/compiler/compiler.h"
#include "mint/memory/object.h"
#include "mint/system/file_stream.h"
#include "mint/system/filesystem.h"
#include "mint/system/buffer_stream.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <ranges>
#include <string>
#include <utility>

using namespace mint;

AbstractSyntaxTree::BuiltinModuleInfo::BuiltinModuleInfo(const Module::Info& infos) {
	id = infos.id;
	bytecode = infos.bytecode;
	debug_info = infos.debug_info;
	state = infos.state;
}

AbstractSyntaxTree::AbstractSyntaxTree() {
	_builtin_modules.reserve(Class::builtin_class_count);
}

AbstractSyntaxTree::~AbstractSyntaxTree() {
	cleanup_memory();
	cleanup_metadata();
	cleanup_modules();
}

void AbstractSyntaxTree::cleanup_memory() {

	// cleanup global data
	_global_data.cleanup_memory();

	// cleanup modules
	for (auto& module : _modules) {
		module.bytecode->cleanup_memory();
	}
}

void AbstractSyntaxTree::cleanup_metadata() {

	// cleanup global data
	_global_data.cleanup_metadata();

	// cleanup modules
	for (auto& module : _modules) {
		module.bytecode->cleanup_metadata();
	}

	// cleanup builtin data
	_global_data.cleanup_builtin();
	_builtin_modules.clear();
}

void AbstractSyntaxTree::cleanup_modules() {

	// cleanup modules
	for (auto& module : _modules) {
		delete module.bytecode;
		delete module.debug_info;
	}
	_modules.clear();

	// cleanup module cache
	_module_cache.clear();
}

std::pair<int, Module::Handle&> AbstractSyntaxTree::create_global_builtin_method(Class& type, int signature,
    GlobalBuiltinMethod method) {

	const auto builtin_index = static_cast<std::size_t>(type.metatype());
	BuiltinModuleInfo& module = builtin_module(builtin_index);

	const std::size_t offset = module.bytecode->next_node_offset() + 2;
	const std::size_t index = _global_builtin_methods.size();
	_global_builtin_methods.emplace_back(method);

	// clang-format off
	module.bytecode->push_nodes({
		Node::Command::jump, static_cast<int>(offset) + 5,
		Node::Command::load_constant, module.bytecode->make_constant<Object>(type),
		Node::Command::call_global_builtin, static_cast<int>(index),
		Node::Command::exit_call, Node::Command::exit_module
	});
	// clang-format on

	return {signature, module.bytecode->make_builtin_handle(type.get_package(), offset)};
}

std::pair<int, Module::Handle&> AbstractSyntaxTree::create_builtin_method(const Class& type, int signature,
    const std::string& method) {

	const auto builtin_index = static_cast<std::size_t>(type.metatype());
	const BuiltinModuleInfo& module = builtin_module(builtin_index);
	const std::size_t offset = module.bytecode->end() + 3;

	auto compiler = Compiler(*this);
	auto stream = BufferStream(method);
	compiler.build(stream, module);

	return {signature, module.bytecode->get_handle(type.get_package(), offset)};
}

std::pair<int, Module::Handle&> AbstractSyntaxTree::create_builtin_method(const Class& type, int signature,
    BuiltinMethod method) {

	const auto builtin_index = static_cast<std::size_t>(type.metatype());
	BuiltinModuleInfo& module = builtin_module(builtin_index);

	const std::size_t offset = module.bytecode->next_node_offset() + 2;
	const std::size_t index = _builtin_methods.size();
	_builtin_methods.emplace_back(method);

	// clang-format off
	module.bytecode->push_nodes({
		Node::Command::jump, static_cast<int>(offset) + 3,
		Node::Command::call_builtin, static_cast<int>(index),
		Node::Command::exit_call, Node::Command::exit_module
	});
	// clang-format on

	return {signature, module.bytecode->make_builtin_handle(type.get_package(), offset)};
}

std::pair<int, Module::Handle&> AbstractSyntaxTree::create_builtin_async_method(const Class& type, int signature,
    BuiltinMethod method) {

	const auto builtin_index = static_cast<std::size_t>(type.metatype());
	BuiltinModuleInfo& module = builtin_module(builtin_index);

	const std::size_t offset = module.bytecode->next_node_offset() + 2;
	const std::size_t index = _builtin_methods.size();
	_builtin_methods.emplace_back(method);

	// clang-format off
	module.bytecode->push_nodes({
		Node::Command::jump, static_cast<int>(offset) + 3,
		Node::Command::call_builtin, static_cast<int>(index),
		Node::Command::resume_coroutine, Node::Command::exit_module
	});
	// clang-format on

	return {signature, module.bytecode->make_builtin_async_handle(type.get_package(), offset)};
}

void AbstractSyntaxTree::call_global_builtin_method(std::size_t method, Cursor& cursor) {
	const auto type = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	_global_builtin_methods[method](type.data<Object>().metadata, cursor);
}

Module::Info AbstractSyntaxTree::create_module(Module::State state) {
	return _modules.emplace_back(Module::Info {
	    .id = _modules.size(),
	    .bytecode = new Module,
	    .debug_info = new DebugInfo,
	    .state = state,
	});
}

Module::Info AbstractSyntaxTree::create_main_module(Module::State state) {
	if (_modules.empty()) {
		return create_module(state);
	}
	_modules.front().state = state;
	return _modules.front();
}

Module::Info AbstractSyntaxTree::create_module_from_file_path(const std::filesystem::path& file_path,
    Module::State state) {
	auto it = _module_cache.find(file_path);
	if (it == _module_cache.end()) {
		if (_modules.empty()) [[unlikely]] {
			create_main_module(Module::State::not_compiled);
		}
		Module::Info info = create_module(state);
		_module_cache.emplace(file_path, info.id);
		return info;
	}
	_modules[it->second].state = state;
	return _modules[it->second];
}

Module::Info AbstractSyntaxTree::module_info(const std::string& module) {

	if (module == Module::main_name) {
		return main();
	}

	const auto path = FileSystem::instance().get_module_path(module);
	if (path.empty()) [[unlikely]] {
		return {};
	}

	if (auto it = _module_cache.find(path); it != _module_cache.end()) {
		return _modules[it->second];
	}

	if (std::filesystem::exists(path)) {
		if (_modules.empty()) [[unlikely]] {
			create_main_module(Module::State::not_compiled);
		}
		Module::Info info = create_module(Module::State::not_compiled);
		_module_cache.emplace(path, info.id);
		return info;
	}

	return {};
}

Module::Info AbstractSyntaxTree::load_module(const std::string& module) {

	const auto path = FileSystem::instance().get_module_path(module);
	if (path.empty()) [[unlikely]] {
		return {};
	}

	auto it = _module_cache.find(path);
	if (it == _module_cache.end()) {
		it = _module_cache.emplace(path, create_module(Module::State::not_compiled).id).first;
	}

	if (_modules[it->second].state == Module::State::not_compiled) {
		auto compiler = Compiler(*this);
		auto stream = FileStream(path);
		compiler.build(stream, _modules[it->second]);
		_modules[it->second].state = Module::State::not_loaded;
	}

	return _modules[it->second];
}

Module::Info AbstractSyntaxTree::main() {
	if (_modules.empty()) {
		return create_module(Module::State::not_compiled);
	}
	return _modules.front();
}

std::string AbstractSyntaxTree::get_module_name(const Module& module) const {
	if (is_main(module)) {
		return Module::main_name;
	}
	for (const auto& [file_path, id] : _module_cache) {
		if (&module == _modules[id].bytecode) {
			return to_module_path(file_path);
		}
	}
	return Module::invalid_name;
}

Module::Id AbstractSyntaxTree::get_module_id(const Module& module) const {
	auto it = std::ranges::find_if(_modules, [&module](const Module::Info& info) {
		return &module == info.bytecode;
	});
	if (it != _modules.end()) {
		return it->id;
	}
	return Module::invalid_id;
}

bool AbstractSyntaxTree::is_main(const Module& module) const {
	return !_modules.empty() && (&module == _modules.front().bytecode);
}

AbstractSyntaxTree::BuiltinModuleInfo& AbstractSyntaxTree::builtin_module(std::size_t module_index) {
	for (std::size_t i = _builtin_modules.size(); i <= module_index; ++i) {
		_builtin_modules.emplace_back(create_module(Module::State::ready));
	}
	return _builtin_modules[module_index];
}

void AbstractSyntaxTree::set_module_state(Module::Id module_id, Module::State state) {
	_modules[module_id].state = state;
}
