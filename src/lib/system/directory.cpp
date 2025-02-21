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

#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/reference.h"
#include "mint/system/errno.h"
#include "mint/system/filesystem.h"
#include "mint/ast/cursor.h"

#include <filesystem>

using namespace mint;

MINT_FUNCTION(mint_directory_native_separator, 0, cursor) {
	cursor->stack().emplace_back(create_string({std::filesystem::path::preferred_separator}));
}

MINT_FUNCTION(mint_directory_to_native_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	helper.return_value(create_string(FileSystem::normalized(to_string(path)).generic_string()));
}

MINT_FUNCTION(mint_directory_root, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::root_path().generic_string()));
}

MINT_FUNCTION(mint_directory_home, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::home_path().generic_string()));
}

MINT_FUNCTION(mint_directory_current, 0, cursor) {
	cursor->stack().emplace_back(create_string(std::filesystem::current_path().generic_string()));
}

MINT_FUNCTION(mint_directory_set_current, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		std::filesystem::current_path(to_string(path));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_directory_absolute_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(create_iterator(create_string(std::filesystem::absolute(to_string(path)).generic_string()),
											WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_directory_canonical_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(create_iterator(create_string(std::filesystem::canonical(to_string(path)).generic_string()),
											WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_directory_relative_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &path = helper.pop_parameter();
	const Reference &root = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_string(std::filesystem::relative(to_string(path), to_string(root)).generic_string()),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_directory_list, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.pop_parameter();

	try {
		WeakReference entries = create_iterator();
		for (const auto &entry : std::filesystem::directory_iterator {to_string(path)}) {
			iterator_yield(entries.data<Iterator>(), create_string(entry.path().filename().generic_string()));
		}
		helper.return_value(create_iterator(std::move(entries), WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_directory_rmdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		if (!std::filesystem::remove(to_string(path))) {
			helper.return_value(create_number(mint::errno_from_error_code(mint::last_error_code())));
		}
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_directory_rmpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		if (!std::filesystem::remove_all(to_string(path))) {
			helper.return_value(create_number(mint::errno_from_error_code(mint::last_error_code())));
		}
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_directory_mkdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		std::filesystem::create_directory(to_string(path));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_directory_mkpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		std::filesystem::create_directories(to_string(path));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_directory_is_subpath, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &sub_path = helper.pop_parameter();
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(create_iterator(create_boolean(FileSystem::is_subpath(to_string(sub_path), to_string(path))),
											WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(errno_from_error_code(error.code()))));
	}
}
