/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/class.h"
#include "mint/debug/debugtool.h"
#include "mint/compiler/compiler.h"
#include "mint/system/filestream.h"
#include "mint/system/filesystem.h"
#include "mint/system/bufferstream.h"
#include "threadentrypoint.h"

#include <filesystem>
#include <algorithm>

using namespace mint;

ThreadEntryPoint ThreadEntryPoint::g_instance;
AbstractSyntaxTree *AbstractSyntaxTree::g_instance = nullptr;

AbstractSyntaxTree::BuiltinModuleInfo::BuiltinModuleInfo(const Module::Info &infos) {
	id = infos.id;
	module = infos.module;
	debug_info = infos.debug_info;
	state = infos.state;
}

AbstractSyntaxTree::AbstractSyntaxTree() {
	g_instance = this;
	m_builtin_modules.reserve(Class::BUILTIN_CLASS_COUNT);
}

AbstractSyntaxTree::~AbstractSyntaxTree() {
	cleanup_memory();
	cleanup_modules();
	cleanup_metadata();
	g_instance = nullptr;
}

AbstractSyntaxTree *AbstractSyntaxTree::instance() {
	return g_instance;
}

void AbstractSyntaxTree::cleanup_memory() {

	// cleanup cursors
	while (!m_cursors.empty()) {
		delete *m_cursors.rbegin();
	}

	// cleanup global data
	m_global_data.cleanup_memory();
}

void AbstractSyntaxTree::cleanup_modules() {

	// cleanup modules
	std::for_each(m_modules.begin(), m_modules.end(), [](const Module::Info &info) {
		delete info.module;
		delete info.debug_info;
	});
	m_modules.clear();

	// cleanup module cache
	m_module_cache.clear();
}

void AbstractSyntaxTree::cleanup_metadata() {

	// cleanup global data
	m_global_data.cleanup_metadata();

	// cleanup builtin data
	m_global_data.cleanup_builtin();
	m_builtin_modules.clear();
}

std::pair<int, Module::Handle *> AbstractSyntaxTree::create_builtin_method(const Class *type, int signature,
																		   BuiltinMethod method) {

	BuiltinModuleInfo &module = builtin_module(-type->metatype());

	const size_t offset = module.module->next_node_offset() + 2;
	const size_t index = m_builtin_methods.size();
	m_builtin_methods.emplace_back(method);

	// clang-format off
	module.module->push_nodes({
		Node::JUMP, static_cast<int>(offset) + 3,
		Node::CALL_BUILTIN, static_cast<int>(index),
		Node::EXIT_CALL, Node::EXIT_MODULE
	});
	// clang-format on

	return std::make_pair(signature, module.module->make_builtin_handle(type->get_package(), module.id, offset));
}

std::pair<int, Module::Handle *> AbstractSyntaxTree::create_builtin_method(const Class *type, int signature,
																		   const std::string &method) {

	const BuiltinModuleInfo &module = builtin_module(-type->metatype());
	BufferStream stream(method);
	const size_t offset = module.module->end() + 3;

	Compiler compiler;
	compiler.build(&stream, module);

	return std::make_pair(signature, module.module->find_handle(module.id, offset));
}

Cursor *AbstractSyntaxTree::create_cursor(Cursor *parent) {
	std::unique_lock<std::mutex> lock(m_mutex);
	return *m_cursors.insert(new Cursor(this, ThreadEntryPoint::instance(), parent)).first;
}

Cursor *AbstractSyntaxTree::create_cursor(Module::Id module, Cursor *parent) {
	std::unique_lock<std::mutex> lock(m_mutex);
	return *m_cursors.insert(new Cursor(this, get_module(module), parent)).first;
}

Module::Info AbstractSyntaxTree::create_module(Module::State state) {
	Module::Info info {
		/*.id = */ m_modules.size(),
		/*.module = */ new Module,
		/*.debug_info = */ new DebugInfo,
		/*.state = */ state,
	};
	m_modules.push_back(info);
	return info;
}

Module::Info AbstractSyntaxTree::create_main_module(Module::State state) {
	if (m_modules.empty()) {
		return create_module(state);
	}
	m_modules.front().state = state;
	return m_modules.front();
}

Module::Info AbstractSyntaxTree::create_module_from_file_path(const std::filesystem::path &file_path,
															  Module::State state) {
	auto it = m_module_cache.find(file_path);
	if (it == m_module_cache.end()) {
		if (UNLIKELY(m_modules.empty())) {
			create_main_module(Module::NOT_COMPILED);
		}
		Module::Info info = create_module(state);
		m_module_cache.emplace(file_path, info.id);
		return info;
	}
	m_modules[it->second].state = state;
	return m_modules[it->second];
}

Module::Info AbstractSyntaxTree::module_info(const std::string &module) {

	if (module == Module::MAIN_NAME) {
		return main();
	}

	std::filesystem::path path = FileSystem::instance().get_module_path(module);
	if (UNLIKELY(path.empty())) {
		return {};
	}

	if (auto it = m_module_cache.find(path); it != m_module_cache.end()) {
		return m_modules[it->second];
	}

	if (std::filesystem::exists(path)) {
		if (UNLIKELY(m_modules.empty())) {
			create_main_module(Module::NOT_COMPILED);
		}
		Module::Info info = create_module(Module::NOT_COMPILED);
		m_module_cache.emplace(path, info.id);
		return info;
	}

	return {};
}

Module::Info AbstractSyntaxTree::load_module(const std::string &module) {

	std::filesystem::path path = FileSystem::instance().get_module_path(module);
	if (UNLIKELY(path.empty())) {
		return {};
	}

	auto it = m_module_cache.find(path);
	if (it == m_module_cache.end()) {
		it = m_module_cache.emplace(path, create_module(Module::NOT_COMPILED).id).first;
	}

	if (m_modules[it->second].state == Module::NOT_COMPILED) {
		Compiler compiler;
		FileStream stream(path);
		compiler.build(&stream, m_modules[it->second]);
		m_modules[it->second].state = Module::NOT_LOADED;
	}

	return m_modules[it->second];
}

Module::Info AbstractSyntaxTree::main() {
	if (m_modules.empty()) {
		return create_module(Module::NOT_COMPILED);
	}
	return m_modules.front();
}

std::string AbstractSyntaxTree::get_module_name(const Module *module) {
	if (module == main().module) {
		return Module::MAIN_NAME;
	}
	for (auto &[file_path, id] : m_module_cache) {
		if (module == m_modules[id].module) {
			return to_module_path(file_path);
		}
	}
	return Module::INVALID_NAME;
}

Module::Id AbstractSyntaxTree::get_module_id(const Module *module) {
	auto it = std::find_if(m_modules.begin(), m_modules.end(), [module](const Module::Info &info) {
		return module == info.module;
	});
	if (it != m_modules.end()) {
		return it->id;
	}
	return Module::INVALID_ID;
}

AbstractSyntaxTree::BuiltinModuleInfo &AbstractSyntaxTree::builtin_module(int module) {

	auto index = static_cast<size_t>(~module);

	for (size_t i = m_builtin_modules.size(); i <= index; ++i) {
		m_builtin_modules.emplace_back(create_module(Module::READY));
	}

	return m_builtin_modules[index];
}

void AbstractSyntaxTree::set_module_state(Module::Id id, Module::State state) {
	m_modules[id].state = state;
}

void AbstractSyntaxTree::remove_cursor(Cursor *cursor) {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cursors.erase(cursor);
}
