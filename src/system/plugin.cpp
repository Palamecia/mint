#include "system/plugin.h"
#include "system/filesystem.h"

#ifdef _WIN32
/// \todo Windows includes
#else
#include <dlfcn.h>
#endif

using namespace std;

Plugin::Plugin(const string &path) : m_path(path) {

#ifdef _WIN32
/// \todo Windows open dll
#else
	m_handle = dlopen(m_path.c_str(), RTLD_LAZY);
#endif
}

Plugin::~Plugin() {

#ifdef _WIN32
/// \todo Windows close dll
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

string Plugin::functionName(const string &name, int signature) {

	if (signature < 0) {
		return name + "_v" + to_string(-signature);
	}

	return name + "_" + to_string(signature);
}

bool Plugin::call(const string &function, int signature, AbstractSyntaxTree *ast) {

	if (function_type fcn_handle = getFunction(functionName(function, signature))) {
		fcn_handle(ast);
		return true;
	}

	for (int i = 1; i <= signature; ++i) {
		if (function_type fcn_handle = getFunction(functionName(function, -i))) {
			fcn_handle(ast);
			return true;
		}
	}

	return false;
}

string Plugin::getPath() const {
	return m_path;
}

Plugin::function_type Plugin::getFunction(const std::string &name) {

#ifdef _WIN32
/// \todo Windows get dll function
#else
	return (function_type)dlsym(m_handle, name.c_str());
#endif
}
