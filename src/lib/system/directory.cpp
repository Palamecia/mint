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
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::nativePath(to_string(path))));
}

MINT_FUNCTION(mint_directory_home, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().homePath()));
}

MINT_FUNCTION(mint_directory_current, 0, cursor) {
	cursor->stack().emplace_back(create_string(FileSystem::instance().currentPath()));
}

MINT_FUNCTION(mint_directory_absolute_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::instance().absolutePath(to_string(path))));
}

MINT_FUNCTION(mint_directory_relative_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference path = move(helper.popParameter());
	WeakReference root = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::instance().relativePath(to_string(root), to_string(path))));
}

MINT_FUNCTION(mint_directory_list, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	WeakReference entries = create_iterator();
	FileSystem::iterator it = FileSystem::instance().browse(to_string(path));
	while (it != FileSystem::instance().end()) {
		iterator_insert(entries.data<Iterator>(), create_string(*it++));
	}
	helper.returnValue(move(entries));
}

MINT_FUNCTION(mint_directory_rmdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().removeDirectory(to_string(path), false)));
}

MINT_FUNCTION(mint_directory_rmpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().removeDirectory(to_string(path), true)));
}

MINT_FUNCTION(mint_directory_mkdir, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().createDirectory(to_string(path), false)));
}

MINT_FUNCTION(mint_directory_mkpath, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().createDirectory(to_string(path), true)));
}
