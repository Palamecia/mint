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

#include "mint/memory/builtin/iterator.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/reference.h"
#include "mint/system/errno.h"
#include "mint/system/filesystem.h"
#include "mint/ast/cursor.h"

#include <filesystem>
#include <utility>

namespace {

mint::WeakReference mint_directory_to_native_path(mint::Cursor& cursor, const mint::Reference& path) {
	return mint::create_string(cursor.ast(), mint::FileSystem::normalized(to_string(path)).generic_string());
}

mint::WeakReference mint_directory_set_current(mint::Cursor& /*cursor*/, const mint::Reference& path) {
	try {
		std::filesystem::current_path(to_string(path));
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::WeakReference mint_directory_absolute_path(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return mint::create_iterator_from(cursor,
		    mint::create_string(cursor.ast(), std::filesystem::absolute(to_string(path)).generic_string()),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::WeakReference mint_directory_canonical_path(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return mint::create_iterator_from(cursor,
		    mint::create_string(cursor.ast(), std::filesystem::canonical(to_string(path)).generic_string()),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::WeakReference mint_directory_relative_path(mint::Cursor& cursor, const mint::Reference& root,
    const mint::Reference& path) {
	try {
		return mint::create_iterator_from(cursor,
		    mint::create_string(cursor.ast(),
		        std::filesystem::relative(to_string(path), to_string(root)).generic_string()),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::WeakReference mint_directory_list(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		mint::WeakReference entries = mint::create_iterator(cursor.ast());
		for (const auto& entry : std::filesystem::directory_iterator {to_string(path)}) {
			iterator_yield(cursor, entries.data<mint::Iterator>(),
			    mint::create_string(cursor.ast(), entry.path().filename().generic_string()));
		}
		return mint::create_iterator_from(cursor, std::move(entries), mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::WeakReference mint_directory_rmdir(mint::Cursor& /*cursor*/, const mint::Reference& path) {
	try {
		if (!std::filesystem::remove(to_string(path))) {
			return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
		}
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::WeakReference mint_directory_rmpath(mint::Cursor& /*cursor*/, const mint::Reference& path) {
	try {
		if (!std::filesystem::remove_all(to_string(path))) {
			return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
		}
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::WeakReference mint_directory_mkdir(mint::Cursor& /*cursor*/, const mint::Reference& path) {
	try {
		std::filesystem::create_directory(to_string(path));
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::WeakReference mint_directory_mkpath(mint::Cursor& /*cursor*/, const mint::Reference& path) {
	try {
		std::filesystem::create_directories(to_string(path));
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::WeakReference mint_directory_is_subpath(mint::Cursor& cursor, const mint::Reference& path,
    const mint::Reference& sub_path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::is_subpath(to_string(sub_path), to_string(path))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

}

MINT_RAW_FUNCTION(mint_directory_native_separator, 0, cursor) {
	cursor.stack().emplace_back(mint::create_string(cursor.ast(), {std::filesystem::path::preferred_separator}));
}

MINT_RAW_FUNCTION(mint_directory_root, 0, cursor) {
	cursor.stack().emplace_back(mint::create_string(cursor.ast(), mint::FileSystem::root_path().generic_string()));
}

MINT_RAW_FUNCTION(mint_directory_home, 0, cursor) {
	cursor.stack().emplace_back(mint::create_string(cursor.ast(), mint::FileSystem::home_path().generic_string()));
}

MINT_RAW_FUNCTION(mint_directory_current, 0, cursor) {
	cursor.stack().emplace_back(mint::create_string(cursor.ast(), std::filesystem::current_path().generic_string()));
}

MINT_EXPORT_FUNCTION(mint_directory_to_native_path, 1)
MINT_EXPORT_FUNCTION(mint_directory_set_current, 1);
MINT_EXPORT_FUNCTION(mint_directory_absolute_path, 1);
MINT_EXPORT_FUNCTION(mint_directory_canonical_path, 1);
MINT_EXPORT_FUNCTION(mint_directory_relative_path, 2);
MINT_EXPORT_FUNCTION(mint_directory_list, 1);
MINT_EXPORT_FUNCTION(mint_directory_rmdir, 1);
MINT_EXPORT_FUNCTION(mint_directory_rmpath, 1);
MINT_EXPORT_FUNCTION(mint_directory_mkdir, 1);
MINT_EXPORT_FUNCTION(mint_directory_mkpath, 1);
MINT_EXPORT_FUNCTION(mint_directory_is_subpath, 2);
