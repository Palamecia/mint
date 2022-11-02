#include <system/filesystem.h>
#include <system/error.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>

#include "dictionnary.h"
#include "parser.h"

using namespace mint;
using namespace std;

struct Options {
	string root;
	string output;
};

void printHelp() {
	puts("Usage : mintdoc [path] [option]");
	puts("Generate a mint project's documentation from formated comments.");
	puts("The mint project directory must be identified by path.");
	puts("Options :");
	puts("  --help              : Print this help message and exit");
	puts("  -o, --output 'path' : Set a custom path for the generated documents (the default path is ./build/)");
}

bool parseArgument(Options *options, int argc, int &argn, char **argv) {

	if (!strcmp(argv[argn], "-o") || !strcmp(argv[argn], "--output")) {
		if (++argn < argc) {
			options->output = argv[argn];
			return true;
		}
	}
	else if (!strcmp(argv[argn], "--help")) {
		printHelp();
		return false;
	}
	else if (options->root.empty()) {
		options->root = argv[argn];
		return true;
	}

	printHelp();
	error("parameter %d ('%s') is not valid", argn, argv[argn]);
	return false;
}

bool parseArguments(Options *options, int argc, char **argv) {

	for (int argn = 1; argn < argc; argn++) {
		if (!parseArgument(options, argc, argn, argv)) {
			return false;
		}
	}

	return true;
}

bool ends_with(const string &str, const string &suffix) {
	return (str.size() >= suffix.size()) && (str.rfind(suffix) == str.size() - suffix.size());
}

string base_name(const string &filename) {
	return filename.substr(0, filename.rfind('.'));
}

string module_path_to_string(const vector<string> &path, const string &module) {
	string name;
	for (const string &scope : path) {
		name += scope + ".";
	}
	return name + base_name(module);
}

void setup(Dictionnary *dictionnary, vector<string> *module_path, const string &path) {
	for (auto i = FileSystem::instance().browse(path); i != FileSystem::instance().end(); ++i) {
		string entry_path = path + FileSystem::separator + (*i);
		if (ends_with(entry_path, ".")) {
			continue;
		}
		if (FileSystem::isDirectory(entry_path)) {
			dictionnary->openModuleGroup(module_path_to_string(*module_path, *i));
			module_path->push_back(*i);
			setup(dictionnary, module_path, entry_path);
			module_path->pop_back();
			dictionnary->closeModule();
		}
		else if (ends_with(entry_path, ".mn")) {
			stringstream stream;
			ifstream file(entry_path);
			stream << file.rdbuf();
			Parser parser(stream.str());
			dictionnary->openModule(module_path_to_string(*module_path, *i));
			parser.parse(dictionnary);
			dictionnary->closeModule();
		}
		else if (ends_with(entry_path, ".mintdoc")) {
			string name = base_name(*i);
			stringstream stream;
			ifstream file(entry_path);
			stream << file.rdbuf();
			if (name == "module") {
				dictionnary->setModuleDoc(stream.str());
			}
			else if (name == "package") {
				dictionnary->setPackageDoc(stream.str());
			}
			else {
				dictionnary->setPageDoc(name, stream.str());
			}
		}
	}
}

int run(int argc, char **argv) {

	Options options;
	Dictionnary dictionnary;
	vector<string> module_path;

	options.output = FileSystem::instance().currentPath() + FileSystem::separator + "build";

	if (!parseArguments(&options, argc, argv)) {
		return EXIT_FAILURE;
	}

	if (!FileSystem::instance().checkFileAccess(options.root, FileSystem::exists)) {
		error("'%s' is not a valid mint project directory", options.root.c_str());
		return EXIT_FAILURE;
	}

	setup(&dictionnary, &module_path, options.root);

	FileSystem::instance().createDirectory(options.output, true);
	dictionnary.generate(options.output);

	return EXIT_SUCCESS;
}

#ifdef OS_WINDOWS

#include <Windows.h>

int wmain(int argc, wchar_t **argv) {
	char **utf8_argv = static_cast<char **>(alloca(argc * sizeof(char *)));
	for (int i = 0; i < argc; ++i) {
		int length = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
		utf8_argv[i] = static_cast<char *>(alloca(length * sizeof(char)));
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, utf8_argv[i], length, nullptr, nullptr);
	}
	return run(argc, utf8_argv);
}
#else
int main(int argc, char **argv) {
	return run(argc, argv);
}
#endif
