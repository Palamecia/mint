#include "system/plugin.h"
#include "system/filesystem.h"

#include <map>

#ifdef OS_UNIX
#include <dlfcn.h>
#endif

using namespace std;
using namespace mint;

struct PluginHandle {
	PluginHandle(const string &path) {
#ifdef OS_WINDOWS
		handle = LoadLibraryW(string_to_windows_path(path).c_str());
#else
		handle = dlopen(path.c_str(), RTLD_LAZY);
#endif
	}

	~PluginHandle() {
#ifdef OS_WINDOWS
		/// \todo Fix read access violation when using data returned by a closed plugin
		/// FreeLibrary(handle);
#else
		dlclose(handle);
#endif
	}

	Plugin::handle_type handle;
};

static Plugin::handle_type load_plugin(const string &path) {

	static map<string, unique_ptr<PluginHandle>> g_plugin_cache;
	auto i = g_plugin_cache.find(path);

	if (i == g_plugin_cache.end()) {
		i = g_plugin_cache.emplace(path, new PluginHandle(path)).first;
	}

	return i->second->handle;
}

Plugin::Plugin(const string &path) :
	m_path(path),
	m_handle(load_plugin(path)) {

}

Plugin::~Plugin() {

}

Plugin *Plugin::load(const string &plugin) {

	string path = FileSystem::instance().getPluginPath(plugin);

	if (path.empty()) {
		return nullptr;
	}

	return new Plugin(path);
}

string Plugin::functionName(const string &name, int signature) {

	if (signature < 0) {
		return name + "_v" + to_string(-signature);
	}

	return name + "_" + to_string(signature);
}

bool Plugin::call(const string &function, int signature, Cursor *cursor) {

	if (function_type fcn_handle = getFunction(functionName(function, signature))) {
		fcn_handle(cursor);
		return true;
	}

	for (int i = 1; i <= signature; ++i) {
		if (function_type fcn_handle = getFunction(functionName(function, -i))) {
			fcn_handle(cursor);
			return true;
		}
	}

	return false;
}

string Plugin::getPath() const {
	return m_path;
}

Plugin::function_type Plugin::getFunction(const string &name) {

#ifdef OS_WINDOWS
	return (function_type)GetProcAddress(m_handle, name.c_str());
#else
	return (function_type)dlsym(m_handle, name.c_str());
#endif
}
