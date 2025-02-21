/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#include <filesystem>
#include <cstdint>

using namespace mint;

namespace {

enum class StandardPath : std::uint8_t {
	ROOT,
	HOME,
	DESKTOP,
	DOCUMENTS,
	MUSICS,
	MOVIES,
	PICTURES,
	DOWNLOAD,
	APPLICATIONS,
	TEMPORARY,
	FONTS,
	CACHE,
	GLOBAL_CACHE,
	DATA,
	LOCAL_DATA,
	GLOBAL_DATA,
	CONFIG,
	GLOBAL_CONFIG
};

std::vector<std::filesystem::path> standard_paths(StandardPath type) {
	switch (type) {
	case StandardPath::ROOT:
		return {FileSystem::root_path()};
	case StandardPath::HOME:
		return {FileSystem::home_path()};
	case StandardPath::DESKTOP:
		return {FileSystem::home_path() / "Desktop"};
	case StandardPath::DOCUMENTS:
		return {FileSystem::home_path() / "Documents"};
	case StandardPath::MUSICS:
		return {FileSystem::home_path() / "Musics"};
	case StandardPath::MOVIES:
		return {FileSystem::home_path() / "Movies"};
	case StandardPath::PICTURES:
		return {FileSystem::home_path() / "Pictures"};
	case StandardPath::DOWNLOAD:
		return {FileSystem::home_path() / "Downloads"};
	case StandardPath::APPLICATIONS:
#if defined(OS_UNIX)
		return {
			"/usr/bin",
			"/bin",
			"/usr/sbin"
			"/usr/local/bin",
		};
#elif defined(OS_WINDOWS)
		return {
			FileSystem::root_path() / "Program Files",
			FileSystem::root_path() / "Program Files (x86)",
		};
#elif defined(OS_MAC)
		return {};
#else
		return {};
#endif
	case StandardPath::TEMPORARY:
#if defined(OS_UNIX)
		return {"/tmp"};
#elif defined(OS_WINDOWS)
		return {
			FileSystem::home_path() / "AppData" / "Local" / "Temp",
			FileSystem::root_path() / "Windows" / "Temp",
		};
#elif defined(OS_MAC)
		return {};
#else
		return {};
#endif
	case StandardPath::FONTS:
		return {};
	case StandardPath::CACHE:
		return {};
	case StandardPath::GLOBAL_CACHE:
		return {};
	case StandardPath::DATA:
		return {};
	case StandardPath::LOCAL_DATA:
		return {};
	case StandardPath::GLOBAL_DATA:
		return {};
	case StandardPath::CONFIG:
		return {};
	case StandardPath::GLOBAL_CONFIG:
		return {};
	}
	return {};
};

}

MINT_FUNCTION(mint_fs_get_paths, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.pop_parameter();
	WeakReference result = create_array();

	for (const std::filesystem::path &path : standard_paths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		array_append(result.data<Array>(), create_string(path.generic_string()));
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_fs_get_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.pop_parameter();

	if (const auto paths = standard_paths(static_cast<StandardPath>(to_integer(cursor, type))); !paths.empty()) {
		helper.return_value(create_string(paths.front().generic_string()));
	}
}

MINT_FUNCTION(mint_fs_get_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.pop_parameter();
	Reference &type = helper.pop_parameter();

	if (const auto paths = standard_paths(static_cast<StandardPath>(to_integer(cursor, type))); !paths.empty()) {
		helper.return_value(create_string(std::filesystem::weakly_canonical(paths.front() / to_string(path)).generic_string()));
	}
}

MINT_FUNCTION(mint_fs_find_paths, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.pop_parameter();
	Reference &type = helper.pop_parameter();
	WeakReference result = create_array();

	for (const std::filesystem::path &root : standard_paths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		std::filesystem::path full_path = std::filesystem::weakly_canonical(root / to_string(path));
		if (std::filesystem::exists(full_path)) {
			array_append(result.data<Array>(), create_string(full_path.generic_string()));
		}
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_fs_find_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.pop_parameter();
	Reference &type = helper.pop_parameter();

	for (const std::filesystem::path &root : standard_paths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		std::filesystem::path full_path = std::filesystem::weakly_canonical(root / to_string(path));
		if (std::filesystem::exists(full_path)) {
			helper.return_value(create_string(full_path.generic_string()));
			break;
		}
	}
}
