#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/filesystem.h"
#include "ast/cursor.h"

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_directory_native_separator, 0, cursor) {
	cursor->stack().emplace_back(create_string({FileSystem::separator}));
}

MINT_FUNCTION(mint_directory_to_native_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();

	helper.returnValue(create_string(FileSystem::nativePath(to_string(path))));
}

MINT_FUNCTION(mint_directory_root, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().rootPath()));
}

MINT_FUNCTION(mint_directory_home, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().homePath()));
}

MINT_FUNCTION(mint_directory_current, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().currentPath()));
}

MINT_FUNCTION(mint_directory_set_current, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();

	if (FileSystem::Error error = FileSystem::instance().setCurrentPath(to_string(path))) {
		helper.returnValue(create_number(error.getErrno()));
	}
}

MINT_FUNCTION(mint_directory_absolute_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();

	helper.returnValue(create_string(FileSystem::instance().absolutePath(to_string(path))));
}

MINT_FUNCTION(mint_directory_relative_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.popParameter();
	Reference &root = helper.popParameter();

	helper.returnValue(create_string(FileSystem::instance().relativePath(to_string(root), to_string(path))));
}

MINT_FUNCTION(mint_directory_list, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();
	WeakReference entries = create_iterator();

	FileSystem &fs = FileSystem::instance();
	for (auto it = fs.browse(to_string(path)); it != fs.end(); ++it) {
		iterator_insert(entries.data<Iterator>(), create_string(*it));
	}

	helper.returnValue(std::move(entries));
}

MINT_FUNCTION(mint_directory_rmdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();

	if (FileSystem::Error error = FileSystem::instance().removeDirectory(to_string(path), false)) {
		helper.returnValue(create_number(error.getErrno()));

	}
}

MINT_FUNCTION(mint_directory_rmpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();

	if (FileSystem::Error error = FileSystem::instance().removeDirectory(to_string(path), true)) {
		helper.returnValue(create_number(error.getErrno()));
	}
}

MINT_FUNCTION(mint_directory_mkdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();

	if (FileSystem::Error error = FileSystem::instance().createDirectory(to_string(path), false)) {
		helper.returnValue(create_number(error.getErrno()));
	}
}

MINT_FUNCTION(mint_directory_mkpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &path = helper.popParameter();

	if (FileSystem::Error error = FileSystem::instance().createDirectory(to_string(path), true)) {
		helper.returnValue(create_number(error.getErrno()));
	}
}
