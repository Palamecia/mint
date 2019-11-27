#include "debug/debugtool.h"
#include "system/filesystem.h"

#include <fstream>

using namespace std;
using namespace mint;

static string g_main_module_path;

void mint::set_main_module_path(const string &path) {

	g_main_module_path = path;

	string load_path = path;
	auto pos = load_path.rfind(FileSystem::separator);

	if (pos != string::npos) {
		FileSystem::instance().addToPath(FileSystem::instance().absolutePath(load_path.erase(pos)));
	}
}

string mint::get_module_line(const string &module, size_t line) {

	string path = FileSystem::instance().getModulePath(module);

	if (module == "main") {
		path = FileSystem::instance().absolutePath(g_main_module_path);
	}

	string line_content;
	ifstream stream(path);

	for (size_t i = 0; i < line; ++i) {
		getline(stream, line_content, '\n');
	}

	return line_content;
}
