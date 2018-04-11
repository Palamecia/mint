#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/filesystem.h"
#include "ast/cursor.h"

using namespace mint;

MINT_FUNCTION(mint_directory_native_separator, 0, cursor) {
	cursor->stack().push_back(create_string({FileSystem::separator}));
}

MINT_FUNCTION(mint_directory_to_native_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_string(FileSystem::nativePath(to_string(*path))));
}

MINT_FUNCTION(mint_directory_home, 0, cursor) {
	cursor->stack().push_back(create_string(FileSystem::instance().homePath()));
}

MINT_FUNCTION(mint_directory_current, 0, cursor) {
	cursor->stack().push_back(create_string(FileSystem::instance().currentPath()));
}

MINT_FUNCTION(mint_directory_absolute_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_string(FileSystem::instance().absolutePath(to_string(*path))));
}

MINT_FUNCTION(mint_directory_relative_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference path = helper.popParameter();
	SharedReference root = helper.popParameter();
	helper.returnValue(create_string(FileSystem::instance().relativePath(to_string(*root), to_string(*path))));
}

MINT_FUNCTION(mint_directory_list, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	SharedReference entries = create_array({});
	FileSystem::iterator it = FileSystem::instance().browse(to_string(*path));
	while (it != FileSystem::instance().end()) {
		array_append(entries->data<Array>(), create_string(*it++));
	}
	helper.returnValue(entries);
}

MINT_FUNCTION(mint_directory_rmdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().removeDirectory(to_string(*path), false)));
}

MINT_FUNCTION(mint_directory_rmpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().removeDirectory(to_string(*path), true)));
}

MINT_FUNCTION(mint_directory_mkdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().createDirectory(to_string(*path), false)));
}

MINT_FUNCTION(mint_directory_mkpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().createDirectory(to_string(*path), true)));
}
