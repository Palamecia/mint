#include "system/plugin.h"
#include "system/filesystem.h"

#ifdef _WIN32

#else
#include <dlfcn.h>
#endif

using namespace std;

Plugin::Plugin(const string &path) {

#ifdef _WIN32

#else
	m_handle = dlopen(path.c_str(), RTLD_LAZY);
#endif
}

Plugin::~Plugin() {

#ifdef _WIN32

#else
	dlclose(m_handle);
#endif
}

Plugin *Plugin::load(const std::string &plugin) {

	string path = FileSystem::instance().getPluginPath(plugin);

	if (path.empty()) {
		return nullptr;
	}

	return new Plugin(path);

}

bool Plugin::call(const string &function, AbstractSynatxTree *ast) {

#ifdef _WIN32

#else
	if (function_type fcn_handle = (function_type)dlsym(m_handle, function.c_str())) {
#endif
		fcn_handle(ast);
		return true;
	}

	return false;
}
