#include "system/filesystem.h"

#include <sstream>
#include <algorithm>

#ifdef OS_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <unistd.h>

#ifdef OS_WINDOWS
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

#ifdef OS_WINDOWS
	wchar_t dli_fname[_MAX_PATH];
	GetModuleFileNameW(nullptr, dli_fname, sizeof dli_fname / sizeof(wchar_t));
	string libraryPath = get_parent_dir(windows_path_to_string(dli_fname));
#else
	Dl_info dl_info;
	dladdr((void *)find_mint, &dl_info);
	string libraryPath = get_parent_dir(dl_info.dli_fname);
#endif
	m_libraryPath.push_back(libraryPath);
	m_libraryPath.push_back(libraryPath + "/mint");

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
#ifdef OS_WINDOWS
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

#ifdef OS_WINDOWS
wstring mint::string_to_windows_path(const string &str) {

	wchar_t buffer[_MAX_PATH];

	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), buffer, sizeof buffer / sizeof(wchar_t))) {
		return buffer;
	}

	return wstring(str.begin(), str.end());
}
#endif

#ifdef OS_WINDOWS
string mint::windows_path_to_string(const wstring &path) {

	char buffer[_MAX_PATH * 4];

	if (WideCharToMultiByte(CP_UTF8, 0, path.c_str(), path.size(), buffer, sizeof buffer, nullptr, nullptr)) {
		return buffer;
	}

	return string(path.begin(), path.end());
}
#endif

FILE *mint::open_file(const char *path, const char *mode) {
#ifdef OS_WINDOWS
	return _wfopen(string_to_windows_path(path).c_str(), string_to_windows_path(mode).c_str());
#else
	return fopen(path, mode);
#endif
}
