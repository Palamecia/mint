#include "System/filesystem.h"

#include <sstream>
#include <unistd.h>

#ifdef _WIN32
#define PATH_SEPARATOR ';'
#else
#define PATH_SEPARATOR ':'
#endif

#define LIBRARY_PATH_VAR "MINT_LIBRARY_PATH"

using namespace std;

FileSystem::FileSystem() {

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

	string modulePath = module;
	for (char &cptr : modulePath) {
		if (cptr == '.') {
			cptr = '/';
		}
	}
	modulePath += ".mn";

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
