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

#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/system/filesystem.h"

#include <filesystem>
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <vector>

namespace {

enum class StandardPath : std::uint8_t {
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

StandardPath to_standard_path(mint::Cursor& cursor, const mint::Reference& value) {
	return static_cast<StandardPath>(mint::to_integer<std::underlying_type_t<StandardPath>>(cursor, value));
}

std::vector<std::filesystem::path> standard_paths(StandardPath type) {
	switch (type) {
	case StandardPath::root:
		return {mint::FileSystem::root_path()};
	case StandardPath::home:
		return {mint::FileSystem::home_path()};
	case StandardPath::desktop:
		return {mint::FileSystem::home_path() / "Desktop"};
	case StandardPath::documents:
		return {mint::FileSystem::home_path() / "Documents"};
	case StandardPath::musics:
		return {mint::FileSystem::home_path() / "Musics"};
	case StandardPath::movies:
		return {mint::FileSystem::home_path() / "Movies"};
	case StandardPath::pictures:
		return {mint::FileSystem::home_path() / "Pictures"};
	case StandardPath::download:
		return {mint::FileSystem::home_path() / "Downloads"};
	case StandardPath::applications:
#ifdef MINT_OS_UNIX
		return {
		    "/usr/bin",
		    "/bin",
		    "/usr/sbin"
		    "/usr/local/bin",
		};
#elifdef MINT_OS_WINDOWS
		return {
		    mint::FileSystem::root_path() / "Program Files",
		    mint::FileSystem::root_path() / "Program Files (x86)",
		};
#elifdef MINT_OS_MAC
		return {};
#else
		return {};
#endif
	case StandardPath::temporary:
#ifdef MINT_OS_UNIX
		return {"/tmp"};
#elifdef MINT_OS_WINDOWS
		return {
		    mint::FileSystem::home_path() / "AppData" / "Local" / "Temp",
		    mint::FileSystem::root_path() / "Windows" / "Temp",
		};
#elifdef MINT_OS_MAC
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
	return {};
};

mint::WeakReference mint_fs_get_paths(mint::Cursor& cursor, const mint::Reference& type) {
	return mint::create_array(cursor.ast(),
	    {std::from_range, std::views::transform(standard_paths(to_standard_path(cursor, type)),
	                          [&cursor](const std::filesystem::path& path) {
		                          return mint::create_string(cursor.ast(), path.generic_string());
	                          })});
}

mint::WeakReference mint_fs_get_path(mint::Cursor& cursor, const mint::Reference& type) {
	if (const auto paths = standard_paths(to_standard_path(cursor, type)); !paths.empty()) {
		return mint::create_string(cursor.ast(), paths.front().generic_string());
	}
	return {};
}

mint::WeakReference mint_fs_get_path(mint::Cursor& cursor, const mint::Reference& type, const mint::Reference& path) {
	if (const auto paths = standard_paths(to_standard_path(cursor, type)); !paths.empty()) {
		return mint::create_string(cursor.ast(),
		    std::filesystem::weakly_canonical(paths.front() / to_string(path)).generic_string());
	}
	return {};
}

mint::WeakReference mint_fs_find_paths(mint::Cursor& cursor, const mint::Reference& type, const mint::Reference& path) {
	return mint::create_array(cursor.ast(),
	    {std::from_range, std::views::transform(standard_paths(to_standard_path(cursor, type)), //
	                          [path = to_string(path)](const std::filesystem::path& root) {
		                          return std::filesystem::weakly_canonical(root / path);
	                          })
	                          | std::views::filter([](const std::filesystem::path& full_path) {
		                            return std::filesystem::exists(full_path);
	                            })
	                          | std::views::transform([&cursor](const std::filesystem::path& full_path) {
		                            return mint::create_string(cursor.ast(), full_path.generic_string());
	                            })});
}

mint::WeakReference mint_fs_find_path(mint::Cursor& cursor, const mint::Reference& type, const mint::Reference& path) {
	for (const std::filesystem::path& root : standard_paths(to_standard_path(cursor, type))) {
		const auto full_path = std::filesystem::weakly_canonical(root / to_string(path));
		if (std::filesystem::exists(full_path)) {
			return mint::create_string(cursor.ast(), full_path.generic_string());
		}
	}
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_fs_get_paths, 1)
MINT_EXPORT_FUNCTION_OVERLOAD(mint_fs_get_path, 1, mint::Cursor&, const mint::Reference&)
MINT_EXPORT_FUNCTION_OVERLOAD(mint_fs_get_path, 2, mint::Cursor&, const mint::Reference&, const mint::Reference&)
MINT_EXPORT_FUNCTION(mint_fs_find_paths, 2)
MINT_EXPORT_FUNCTION(mint_fs_find_path, 2)
