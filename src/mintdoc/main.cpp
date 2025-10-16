/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "dictionary.h"
#include "mint/system/arguments.h"
#include "mint/system/error.h"
#include "parser.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Options {
	std::vector<std::filesystem::path> roots;
	std::filesystem::path output;
};

void print_help() {
	std::puts("Usage : mintdoc [path] [option]");
	std::puts("Generate a mint project's documentation from formatted comments.");
	std::puts("The mint project directory must be identified by path.");
	std::puts("Options :");
	std::puts("  --help              : Print this help message and exit");
	std::puts("  -o, --output 'path' : Set a custom path for the generated documents (the default path is ./build/)");
}

bool parse_argument(Options& options, auto& it, const auto& end) {

	if (*it == "-o" || *it == "--output") {
		if (++it != end) {
			options.output = std::filesystem::weakly_canonical(*it);
			return true;
		}
	}
	else if (*it == "--help") {
		print_help();
		return false;
	}
	else {
		options.roots.push_back(std::filesystem::weakly_canonical(*it));
		return true;
	}

	print_help();
	mint::error("parameter ('{}') is not valid", *it);
	return false;
}

bool parse_arguments(Options& options, const std::vector<std::string>& args) {

	for (auto it = args.begin(); it != args.end(); ++it) {
		if (!parse_argument(options, it, args.end())) {
			return false;
		}
	}

	return true;
}

std::string base_name(const std::string& filename) {
	return filename.substr(0, filename.rfind('.'));
}

std::string module_path_to_string(const std::vector<std::string>& path, const std::string& module) {
	std::string name;
	for (const std::string& scope : path) {
		name += scope + ".";
	}
	return name + base_name(module);
}

void setup(Dictionary& dictionary, std::vector<std::string>& module_path, const std::filesystem::path& path) {
	for (const auto& entry : std::filesystem::directory_iterator {path}) {
		if (entry.is_directory()) {
			dictionary.open_module_group(module_path_to_string(module_path, entry.path().stem().generic_string()));
			module_path.push_back(entry.path().stem().generic_string());
			setup(dictionary, module_path, entry.path());
			module_path.pop_back();
			dictionary.close_module();
		}
		else if (entry.path().extension() == ".mn") {
			Parser parser(entry.path());
			dictionary.open_module(module_path_to_string(module_path, entry.path().stem().generic_string()));
			parser.parse(dictionary);
			dictionary.close_module();
		}
		else if (entry.path().extension() == ".mintdoc") {
			const auto name = entry.path().stem().generic_string();
			auto stream = std::stringstream();
			auto file = std::ifstream(entry.path());
			stream << file.rdbuf();
			if (name == "module") {
				dictionary.set_module_doc(std::move(stream).str());
			}
			else if (name == "package") {
				dictionary.set_package_doc(std::move(stream).str());
			}
			else {
				dictionary.set_page_doc(name, std::move(stream).str());
			}
		}
	}
}

int run(const std::vector<std::string>& args) {

	auto options = Options();
	auto dictionary = Dictionary();
	auto module_path = std::vector<std::string>();

	options.output = std::filesystem::current_path() / "build";

	if (!parse_arguments(options, args)) {
		return EXIT_FAILURE;
	}

	for (const std::filesystem::path& root : options.roots) {

		if (!std::filesystem::exists(root)) {
			mint::error("'{}' is not a valid mint project directory", root.generic_string());
			return EXIT_FAILURE;
		}

		setup(dictionary, module_path, root);
	}

	std::filesystem::create_directories(options.output);
	dictionary.generate(options.output);

	return EXIT_SUCCESS;
}

}

int main() {
	return run({std::from_range, std::views::drop(mint::arguments(), 1)});
}
