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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/system/filesystem.h"

using namespace std;
using namespace mint;

enum class StandardPath {
	root,
	home,
	desktop,
	documents,
	musics,
	movies,
	pictures,
	download,
	applications,
	temporary,
	fonts,
	cache,
	global_cache,
	data,
	local_data,
	global_data,
	config,
	global_config
};

static vector<string> standardPaths(StandardPath type) {
	switch (type) {
	case StandardPath::root:
		return { FileSystem::instance().root_path() };
	case StandardPath::home:
		return { FileSystem::instance().home_path() };
	case StandardPath::desktop:
		return { FileSystem::instance().home_path() + "/Desktop" };
	case StandardPath::documents:
		return { FileSystem::instance().home_path() + "/Documents" };
	case StandardPath::musics:
		return { FileSystem::instance().home_path() + "/Musics" };
	case StandardPath::movies:
		return { FileSystem::instance().home_path() + "/Movies" };
	case StandardPath::pictures:
		return { FileSystem::instance().home_path() + "/Pictures" };
	case StandardPath::download:
		return { FileSystem::instance().home_path() + "/Downloads" };
	case StandardPath::applications:
#if defined (OS_UNIX)
		return {
			"/usr/bin",
			"/bin",
			"/usr/sbin"
			"/usr/local/bin",
		};
#elif defined (OS_WINDOWS)
		return {
				FileSystem::instance().root_path() + "/Program Files",
			FileSystem::instance().root_path() + "/Program Files (x86)"
		};
#elif defined (OS_MAC)
		return {};
#else
		return {};
#endif
	case StandardPath::temporary:
#if defined (OS_UNIX)
		return { "/tmp" };
#elif defined (OS_WINDOWS)
		return {
			FileSystem::instance().home_path() + "/AppData/Local/Temp",
			FileSystem::instance().root_path() + "/Windows/Temp"
		};
#elif defined (OS_MAC)
		return {};
#else
		return {};
#endif
	case StandardPath::fonts:
		return {};
	case StandardPath::cache:
		return {};
	case StandardPath::global_cache:
		return {};
	case StandardPath::data:
		return {};
	case StandardPath::local_data:
		return {};
	case StandardPath::global_data:
		return {};
	case StandardPath::config:
		return {};
	case StandardPath::global_config:
		return {};
	}
};

MINT_FUNCTION(mint_fs_get_paths, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.pop_parameter();
	WeakReference result = create_array();

	for (const string &path : standardPaths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		array_append(result.data<Array>(), create_string(path));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_fs_get_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.pop_parameter();

	helper.return_value(create_string(standardPaths(static_cast<StandardPath>(to_integer(cursor, type))).front()));
}

MINT_FUNCTION(mint_fs_get_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.pop_parameter();
	Reference &type = helper.pop_parameter();

	string root = standardPaths(static_cast<StandardPath>(to_integer(cursor, type))).front();
	helper.return_value(create_string(FileSystem::clean_path(root + FileSystem::separator + to_string(path))));
}

MINT_FUNCTION(mint_fs_find_paths, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.pop_parameter();
	Reference &type = helper.pop_parameter();
	WeakReference result = create_array();

	for (const string &root : standardPaths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		string full_path = FileSystem::clean_path(root + FileSystem::separator + to_string(path));
		if (FileSystem::instance().check_file_access(full_path, FileSystem::exists)) {
			array_append(result.data<Array>(), create_string(full_path));
		}
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_fs_find_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.pop_parameter();
	Reference &type = helper.pop_parameter();

	for (const string &root : standardPaths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		string full_path = FileSystem::clean_path(root + FileSystem::separator + to_string(path));
		if (FileSystem::instance().check_file_access(full_path, FileSystem::exists)) {
			helper.return_value(create_string(full_path));
			break;
		}
	}
}
