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
#include "mint/ast/cursor.h"

using namespace mint;

MINT_FUNCTION(mint_directory_native_separator, 0, cursor) {
	cursor->stack().emplace_back(create_string({FileSystem::SEPARATOR}));
}

MINT_FUNCTION(mint_directory_to_native_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();
	
	helper.return_value(create_string(FileSystem::native_path(to_string(path))));
}

MINT_FUNCTION(mint_directory_root, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().root_path()));
}

MINT_FUNCTION(mint_directory_home, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().home_path()));
}

MINT_FUNCTION(mint_directory_current, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().current_path()));
}

MINT_FUNCTION(mint_directory_set_current, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	if (SystemError error = FileSystem::instance().set_current_path(to_string(path))) {
		helper.return_value(create_number(error.get_errno()));
	}
}

MINT_FUNCTION(mint_directory_absolute_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();
	
	helper.return_value(create_string(FileSystem::instance().absolute_path(to_string(path))));
}

MINT_FUNCTION(mint_directory_relative_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &path = helper.pop_parameter();
	const Reference &root = helper.pop_parameter();
	
	helper.return_value(create_string(FileSystem::instance().relative_path(to_string(root), to_string(path))));
}

MINT_FUNCTION(mint_directory_list, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.pop_parameter();
	WeakReference entries = create_iterator();

	FileSystem &fs = FileSystem::instance();
	for (auto it = fs.browse(to_string(path)); it != fs.end(); ++it) {
		iterator_insert(entries.data<Iterator>(), create_string(*it));
	}
	
	helper.return_value(std::move(entries));
}

MINT_FUNCTION(mint_directory_rmdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();
	
	if (SystemError error = FileSystem::instance().remove_directory(to_string(path), false)) {
		helper.return_value(create_number(error.get_errno()));

	}
}

MINT_FUNCTION(mint_directory_rmpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();
	
	if (SystemError error = FileSystem::instance().remove_directory(to_string(path), true)) {
		helper.return_value(create_number(error.get_errno()));
	}
}

MINT_FUNCTION(mint_directory_mkdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();
	
	if (SystemError error = FileSystem::instance().create_directory(to_string(path), false)) {
		helper.return_value(create_number(error.get_errno()));
	}
}

MINT_FUNCTION(mint_directory_mkpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();
	
	if (SystemError error = FileSystem::instance().create_directory(to_string(path), true)) {
		helper.return_value(create_number(error.get_errno()));
	}
}

MINT_FUNCTION(mint_directory_is_sub_path, 2, cursor) {
	
	FunctionHelper helper(cursor, 2);
	const Reference &sub_path = helper.pop_parameter();
	const Reference &path = helper.pop_parameter();
	
	helper.return_value(create_boolean(FileSystem::instance().is_sub_path(to_string(sub_path), to_string(path))));
}
