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

#include <mint/system/filesystem.h>
#include <mint/system/error.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>

#include "dictionary.h"
#include "parser.h"

using namespace mint;

struct Options {
	std::vector<std::string> roots;
	std::string output;
};

void printHelp() {
	puts("Usage : mintdoc [path] [option]");
	puts("Generate a mint project's documentation from formatted comments.");
	puts("The mint project directory must be identified by path.");
	puts("Options :");
	puts("  --help              : Print this help message and exit");
	puts("  -o, --output 'path' : Set a custom path for the generated documents (the default path is ./build/)");
}

bool parse_argument(Options *options, int argc, int &argn, char **argv) {

	if (!strcmp(argv[argn], "-o") || !strcmp(argv[argn], "--output")) {
		if (++argn < argc) {
			options->output = FileSystem::clean_path(argv[argn]);
			return true;
		}
	}
	else if (!strcmp(argv[argn], "--help")) {
		printHelp();
		return false;
	}
	else {
		options->roots.push_back(FileSystem::clean_path(argv[argn]));
		return true;
	}

	printHelp();
	error("parameter %d ('%s') is not valid", argn, argv[argn]);
	return false;
}

bool parse_arguments(Options *options, int argc, char **argv) {

	for (int argn = 1; argn < argc; argn++) {
		if (!parse_argument(options, argc, argn, argv)) {
			return false;
		}
	}

	return true;
}

bool ends_with(const std::string &str, const std::string &suffix) {
	return (str.size() >= suffix.size()) && (str.rfind(suffix) == str.size() - suffix.size());
}

std::string base_name(const std::string &filename) {
	return filename.substr(0, filename.rfind('.'));
}

std::string module_path_to_string(const std::vector<std::string> &path, const std::string &module) {
	std::string name;
	for (const std::string &scope : path) {
		name += scope + ".";
	}
	return name + base_name(module);
}

void setup(Dictionary *dictionary, std::vector<std::string> *module_path, const std::string &path) {
	for (auto i = FileSystem::instance().browse(path); i != FileSystem::instance().end(); ++i) {
		std::string entry_path = path + FileSystem::SEPARATOR + (*i);
		if (ends_with(entry_path, ".")) {
			continue;
		}
		if (FileSystem::is_directory(entry_path)) {
			dictionary->open_module_group(module_path_to_string(*module_path, *i));
			module_path->push_back(*i);
			setup(dictionary, module_path, entry_path);
			module_path->pop_back();
			dictionary->close_module();
		}
		else if (ends_with(entry_path, ".mn")) {
			Parser parser(entry_path);
			dictionary->open_module(module_path_to_string(*module_path, *i));
			parser.parse(dictionary);
			dictionary->close_module();
		}
		else if (ends_with(entry_path, ".mintdoc")) {
			std::string name = base_name(*i);
			std::stringstream stream;
			std::ifstream file(entry_path);
			stream << file.rdbuf();
			if (name == "module") {
				dictionary->set_module_doc(stream.str());
			}
			else if (name == "package") {
				dictionary->set_package_doc(stream.str());
			}
			else {
				dictionary->set_page_doc(name, stream.str());
			}
		}
	}
}

int run(int argc, char **argv) {

	Options options;
	Dictionary dictionary;
	std::vector<std::string> module_path;

	options.output = FileSystem::instance().current_path() + FileSystem::SEPARATOR + "build";

	if (!parse_arguments(&options, argc, argv)) {
		return EXIT_FAILURE;
	}

	for (const std::string &root : options.roots) {

		if (!FileSystem::instance().check_file_access(root, FileSystem::EXISTS_FLAG)) {
			error("'%s' is not a valid mint project directory", root.c_str());
			return EXIT_FAILURE;
		}

		setup(&dictionary, &module_path, root);
	}

	FileSystem::instance().create_directory(options.output, true);
	dictionary.generate(options.output);

	return EXIT_SUCCESS;
}

#ifdef OS_WINDOWS

#include <Windows.h>

int wmain(int argc, wchar_t **argv) {
	char **utf8_argv = static_cast<char **>(malloc(argc * sizeof(char *)));
	for (int i = 0; i < argc; ++i) {
		int length = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
		utf8_argv[i] = static_cast<char *>(malloc(length * sizeof(char)));
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, utf8_argv[i], length, nullptr, nullptr);
	}
	int status = run(argc, utf8_argv);
	for (int i = 0; i < argc; ++i) {
		free(utf8_argv[i]);
	}
	free(utf8_argv);
	return status;
}
#else
int main(int argc, char **argv) {
	return run(argc, argv);
}
#endif
