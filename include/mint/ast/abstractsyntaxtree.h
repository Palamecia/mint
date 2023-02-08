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

#ifndef MINT_ABSTRACTSYNTAXTREE_H
#define MINT_ABSTRACTSYNTAXTREE_H

#include "mint/ast/module.h"
#include "mint/memory/globaldata.h"
#include "mint/system/filesystem.h"

#if 0
#include <functional>
#else
#include <type_traits>
#endif

#include <vector>
#include <mutex>

namespace mint {

class Cursor;
class Class;

class MINT_EXPORT AbstractSyntaxTree {
	friend class Cursor;
public:
	AbstractSyntaxTree();
	~AbstractSyntaxTree();

	AbstractSyntaxTree(const AbstractSyntaxTree &other) = delete;
	AbstractSyntaxTree &operator =(const AbstractSyntaxTree &other) = delete;

#if 0
	using BuiltinMethode = std::function<void(Cursor *)>;
#else
	using BuiltinMethode = std::add_pointer<void(Cursor *)>::type;
#endif

	static AbstractSyntaxTree *instance();

	std::pair<int, Module::Handle *> create_builtin_methode(Class *type, int signature, BuiltinMethode methode);
	std::pair<int, Module::Handle *> create_builtin_methode(Class *type, int signature, const std::string &methode);
	inline void call_builtin_methode(size_t methode, Cursor *cursor);

	Cursor *create_cursor(Cursor *parent = nullptr);
	Cursor *create_cursor(Module::Id module, Cursor *parent = nullptr);

	Module::Info create_module(Module::State state);
	Module::Info module_info(const std::string &module);
	Module::Info load_module(const std::string &module);
	Module::Info main();

	inline Module *get_module(Module::Id id);
	inline DebugInfo *get_debug_info(Module::Id id);
	Module::Id get_module_id(const Module *module);
	std::string get_module_name(const Module *module);

	inline GlobalData &global_data();

	void cleanup_memory();
	void cleanup_modules();
	void cleanup_metadata();

protected:
	struct BuiltinModuleInfo : public Module::Info {
		BuiltinModuleInfo(const Module::Info &infos);
	};

	BuiltinModuleInfo &builtin_module(int module);

	void set_module_state(Module::Id id, Module::State state);

	void remove_cursor(Cursor *cursor);

private:
	static AbstractSyntaxTree *g_instance;

	std::mutex m_mutex;
	std::set<Cursor *> m_cursors;
	std::vector<Module::Info> m_modules;
	std::map<std::string, size_t, FileSystem::path_less> m_module_cache;

	GlobalData m_global_data;
	std::vector<BuiltinModuleInfo> m_builtin_modules;
	std::vector<BuiltinMethode> m_builtin_methodes;
};

void AbstractSyntaxTree::call_builtin_methode(size_t methode, Cursor *cursor) {
	m_builtin_methodes[methode](cursor);
}

Module *AbstractSyntaxTree::get_module(Module::Id id) {
	assert(id < m_modules.size());
	return m_modules[id].module;
}

DebugInfo *AbstractSyntaxTree::get_debug_info(Module::Id id) {
	return (id < m_modules.size()) ? m_modules[id].debug_info : nullptr;
}

GlobalData &AbstractSyntaxTree::global_data() {
	return m_global_data;
}

}

#endif // MINT_ABSTRACTSYNTAXTREE_H
