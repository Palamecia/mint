#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/utf8iterator.h"
#include "system/filesystem.h"

#include <stdio.h>

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_file_symlink_target, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_string(FileSystem::symlinkTarget(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_birth_time, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_number(FileSystem::birthTime(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_last_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_number(FileSystem::lastRead(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_last_modified, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_number(FileSystem::lastModified(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_exists, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(*path)), FileSystem::exists)));
}

MINT_FUNCTION(mint_file_size, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_number(FileSystem::sizeOf(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_is_root, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::isRoot(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_is_file, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::isFile(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_is_directory, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::isDirectory(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_is_symlink, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::isSymlink(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_is_bundle, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::isBundle(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_is_readable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(*path)), FileSystem::readable)));
}

MINT_FUNCTION(mint_file_is_writable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(*path)), FileSystem::writable)));
}

MINT_FUNCTION(mint_file_is_executable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(*path)), FileSystem::executable)));
}

MINT_FUNCTION(mint_file_is_hidden, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::isHidden(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_owner, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_string(FileSystem::owner(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_owner_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_number(FileSystem::ownerId(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_group, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_string(FileSystem::group(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_group_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_number(FileSystem::groupId(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_permission, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference permissions = helper.popParameter();
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::checkFilePermissions(to_string(*path), static_cast<FileSystem::Permissions>(to_number(cursor, *permissions)))));
}

MINT_FUNCTION(mint_file_link, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference target = helper.popParameter();
	SharedReference source = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().createLink(FileSystem::instance().absolutePath(to_string(*source)), FileSystem::instance().absolutePath(to_string(*target)))));
}

MINT_FUNCTION(mint_file_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference target = helper.popParameter();
	SharedReference source = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().copy(FileSystem::instance().absolutePath(to_string(*source)), FileSystem::instance().absolutePath(to_string(*target)))));
}

MINT_FUNCTION(mint_file_rename, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference target = helper.popParameter();
	SharedReference source = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().rename(FileSystem::instance().absolutePath(to_string(*source)), FileSystem::instance().absolutePath(to_string(*target)))));
}

MINT_FUNCTION(mint_file_remove, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = helper.popParameter();
	helper.returnValue(create_boolean(FileSystem::instance().remove(FileSystem::instance().absolutePath(to_string(*path)))));
}

MINT_FUNCTION(mint_file_fopen, 2,cursor) {

	FunctionHelper helper(cursor, 2);

	string mode = to_string(*helper.popParameter());
	string path = to_string(*helper.popParameter());

	if (FILE *file = open_file(path.c_str(), mode.c_str())) {
		helper.returnValue(create_object(file));
	}
	else {
		helper.returnValue(Reference::create<Null>());
	}
}

MINT_FUNCTION(mint_file_fclose, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	if (file.data<LibObject<FILE>>()->impl) {
		fclose(file.data<LibObject<FILE>>()->impl);
		file.data<LibObject<FILE>>()->impl = nullptr;
	}
}

MINT_FUNCTION(mint_file_fgetc, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		string result(1, cptr);
		size_t length = utf8char_length(cptr);
		while (--length) {
			result += fgetc(file.data<LibObject<FILE>>()->impl);
		}
		helper.returnValue(create_string(result));
	}
}

MINT_FUNCTION(mint_file_readline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		string result;
		while ((cptr != '\n') && (cptr != EOF)) {
			result += cptr;
			cptr = fgetc(file.data<LibObject<FILE>>()->impl);
		}
		helper.returnValue(create_string(result));
	}
}

MINT_FUNCTION(mint_file_fwrite, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	Reference &value = *helper.popParameter();
	Reference &file = *helper.popParameter();

	string str = to_string(value);
	auto amount = fwrite(str.c_str(), sizeof(char), str.size(), file.data<LibObject<FILE>>()->impl);

	helper.returnValue(create_number(amount));
}

MINT_FUNCTION(mint_file_fflush, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference file = helper.popParameter();
	fflush(file->data<LibObject<FILE>>()->impl);
}
