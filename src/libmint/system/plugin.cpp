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

#include "mint/system/plugin.h"
#include "mint/system/filesystem.h"

#include <map>

#ifdef OS_UNIX
#include <dlfcn.h>
#endif

using namespace mint;

struct PluginHandle {
	explicit PluginHandle(const std::string &path) :
#ifdef OS_WINDOWS
		handle(LoadLibraryW(string_to_windows_path(path).c_str())) {
#else
		handle(dlopen(path.c_str(), RTLD_LAZY)) {
#endif
	}

	~PluginHandle() {
#ifdef OS_WINDOWS
		FreeLibrary(handle);
#else
		dlclose(handle);
#endif
	}

	Plugin::handle_type handle;
};

static Plugin::handle_type load_plugin(const std::string &path) {

	static std::map<std::string, std::unique_ptr<PluginHandle>> g_plugin_cache;
	auto i = g_plugin_cache.find(path);

	if (i == g_plugin_cache.end()) {
		i = g_plugin_cache.emplace(path, new PluginHandle(path)).first;
	}

	return i->second->handle;
}

Plugin::Plugin(const std::string &path) :
	m_path(path),
	m_handle(load_plugin(path)) {

}

Plugin::~Plugin() {

}

Plugin *Plugin::load(const std::string &plugin) {

	std::string path = FileSystem::instance().get_plugin_path(plugin);

	if (path.empty()) {
		return nullptr;
	}

	return new Plugin(path);
}

std::string Plugin::function_name(const std::string &name, int signature) {

	if (signature < 0) {
		return name + "_v" + std::to_string(~signature);
	}

	return name + "_" + std::to_string(signature);
}

bool Plugin::call(const std::string &function, int signature, Cursor *cursor) {
	
	if (function_type fcn_handle = get_function(function_name(function, signature))) {
		fcn_handle(cursor);
		return true;
	}

	for (int i = 1; i <= signature; ++i) {
		if (function_type fcn_handle = get_function(function_name(function, -i))) {
			fcn_handle(cursor);
			return true;
		}
	}

	return false;
}

std::string Plugin::get_path() const {
	return m_path;
}

Plugin::function_type Plugin::get_function(const std::string &name) {
#ifdef OS_WINDOWS
	return (function_type)GetProcAddress(m_handle, name.c_str());
#else
	return (function_type)dlsym(m_handle, name.c_str());
#endif
}
