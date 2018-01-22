#include "system/filesystem.h"

#include <sstream>
#include <algorithm>

#ifdef _WIN32
/// \todo Windows includes
#else
#include <dlfcn.h>
#endif

#include <unistd.h>

#ifdef _WIN32
#define PATH_SEPARATOR ';'
#else
#define PATH_SEPARATOR ':'
#endif

#define LIBRARY_PATH_VAR "MINT_LIBRARY_PATH"

using namespace std;
using namespace mint;

extern "C" {
void find_mint(void) {}
}

string format_path(const string &mint_path) {

	string path = mint_path;
	replace(path.begin(), path.end(), '.', '/');
	return path;
}

string get_parent_dir(const string &path) {
	return path.substr(0, path.find_last_of('/'));
}

FileSystem::FileSystem() {

#ifdef _WIN32
	/// \todo Windows find mint
#else
	Dl_info dl_info;
	dladdr((void *)find_mint, &dl_info);
	string libraryPath = get_parent_dir(dl_info.dli_fname);
	m_libraryPath.push_back(libraryPath);
	m_libraryPath.push_back(libraryPath + "/mint");
#endif

	if (const char *var = getenv(LIBRARY_PATH_VAR)) {

		string path;
		istringstream stream(var);

		while (getline(stream, path, PATH_SEPARATOR)) {
			m_libraryPath.push_back(path);
		}
	}
}

FileSystem &FileSystem::instance() {

	static FileSystem g_instance;

	return g_instance;
}

string FileSystem::getModulePath(const string &module) {

	string modulePath = format_path(module) + ".mn";

	if (access(modulePath.c_str(), R_OK) == 0) {
		return modulePath;
	}

	for (string path : m_libraryPath) {
		string fullPath = path + '/' + modulePath;
		if (access(fullPath.c_str(), R_OK) == 0) {
			return fullPath;
		}
	}

	return string();
}

string FileSystem::getPluginPath(const string &plugin) {

	string pluginPath = format_path(plugin);
#ifdef _WIN32
	pluginPath += ".dll";
#else
	pluginPath += ".so";
#endif

	if (access(pluginPath.c_str(), R_OK) == 0) {
		return pluginPath;
	}

	for (string path : m_libraryPath) {
		string fullPath = path + '/' + pluginPath;
		if (access(fullPath.c_str(), R_OK) == 0) {
			return fullPath;
		}
	}

	return string();
}
