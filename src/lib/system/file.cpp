#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/utf8iterator.h"
#include "system/filesystem.h"
#include "system/stdio.h"

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_file_symlink_target, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::symlinkTarget(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_birth_time, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::birthTime(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_last_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::lastRead(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_last_modified, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::lastModified(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_exists, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::exists)));
}

MINT_FUNCTION(mint_file_size, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::sizeOf(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_is_root, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isRoot(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_file, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isFile(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_directory, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isDirectory(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_symlink, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isSymlink(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_bundle, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isBundle(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_readable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::readable)));
}

MINT_FUNCTION(mint_file_is_writable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::writable)));
}

MINT_FUNCTION(mint_file_is_executable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::executable)));
}

MINT_FUNCTION(mint_file_is_hidden, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isHidden(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_owner, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::owner(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_owner_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_number(FileSystem::ownerId(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_group, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::group(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_group_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_number(FileSystem::groupId(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_permission, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference permissions = move(helper.popParameter());
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFilePermissions(to_string(path), static_cast<FileSystem::Permissions>(to_number(cursor, permissions)))));
}

MINT_FUNCTION(mint_file_link, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference target = move(helper.popParameter());
	SharedReference source = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().createLink(FileSystem::instance().absolutePath(to_string(source)), FileSystem::instance().absolutePath(to_string(target)))));
}

MINT_FUNCTION(mint_file_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference target = move(helper.popParameter());
	SharedReference source = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().copy(FileSystem::instance().absolutePath(to_string(source)), FileSystem::instance().absolutePath(to_string(target)))));
}

MINT_FUNCTION(mint_file_rename, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference target = move(helper.popParameter());
	SharedReference source = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().rename(FileSystem::instance().absolutePath(to_string(source)), FileSystem::instance().absolutePath(to_string(target)))));
}

MINT_FUNCTION(mint_file_remove, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::instance().remove(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_fopen, 2,cursor) {

	FunctionHelper helper(cursor, 2);

	string mode = to_string(helper.popParameter());
	string path = to_string(helper.popParameter());

	if (FILE *file = open_file(path.c_str(), mode.c_str())) {
		helper.returnValue(create_object(file));
	}
	else {
		helper.returnValue(StrongReference::create<Null>());
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

MINT_FUNCTION(mint_file_fileno, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();
	int fd = fileno(file.data<LibObject<FILE>>()->impl);

	if (fd != -1) {
		helper.returnValue(create_number(fd));
	}
}

MINT_FUNCTION(mint_file_at_end, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();
	helper.returnValue(create_boolean(feof(file.data<LibObject<FILE>>()->impl)));
}

MINT_FUNCTION(mint_file_fgetc, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		string result(1, static_cast<char>(cptr));
		size_t length = utf8char_length(static_cast<uint8_t>(cptr));
		while (--length) {
			result += static_cast<char>(fgetc(file.data<LibObject<FILE>>()->impl));
		}
		helper.returnValue(create_string(result));
	}
}

MINT_FUNCTION(mint_file_fgetw, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	ssize_t read;
	char *word = nullptr;

	if ((read = fscanf(file.data<LibObject<FILE>>()->impl, "%ms", &word)) != EOF) {
		helper.returnValue(create_string(word));
		free(word);
	}
}

MINT_FUNCTION(mint_file_readline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	ssize_t read;
	size_t len = 0;
	char *line = nullptr;

	if ((read = getline(&line, &len, file.data<LibObject<FILE>>()->impl)) != EOF) {
		line[read - 1] = '\0';
		helper.returnValue(create_string(line));
		free(line);
	}
}

MINT_FUNCTION(mint_file_read_array, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();
	SharedReference result = create_array({});

	ssize_t read;
	size_t len = 0;
	char *line = nullptr;

	while ((read = getline(&line, &len, file.data<LibObject<FILE>>()->impl)) != EOF) {
		line[read - 1] = '\0';
		result->data<Array>()->values.emplace_back(create_string(line));
	}

	free(line);
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_file_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = *helper.popParameter();

	ssize_t read;
	string result;
	size_t len = 0;
	char *line = nullptr;

	while ((read = getline(&line, &len, file.data<LibObject<FILE>>()->impl)) != EOF) {
		result += line;
	}

	free(line);
	helper.returnValue(create_string(result));
}

MINT_FUNCTION(mint_file_fwrite, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference &value = helper.popParameter();
	SharedReference &file = helper.popParameter();

	string str = to_string(value);
	auto amount = fwrite(str.c_str(), sizeof(char), str.size(), file->data<LibObject<FILE>>()->impl);

	helper.returnValue(create_number(static_cast<double>(amount)));
}

MINT_FUNCTION(mint_file_read_byte, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference &buffer = helper.popParameter();
	Reference &file = *helper.popParameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		buffer->data<LibObject<vector<uint8_t>>>()->impl->push_back(static_cast<uint8_t>(cptr));
		helper.returnValue(create_boolean(true));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}

MINT_FUNCTION(mint_file_read_binary, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference &buffer = helper.popParameter();
	SharedReference &file = helper.popParameter();

	uint8_t chunk[BUFSIZ];
	vector<uint8_t> *bytearray = buffer->data<LibObject<vector<uint8_t>>>()->impl;

	while (!feof(file->data<LibObject<FILE>>()->impl)) {
		auto amount = fread(chunk, sizeof(uint8_t), sizeof(chunk), file->data<LibObject<FILE>>()->impl);
		copy_n(chunk, amount, back_inserter(*bytearray));
	}

	helper.returnValue(create_boolean(!bytearray->empty()));
}

MINT_FUNCTION(mint_file_fwrite_binary, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference &buffer = helper.popParameter();
	SharedReference &file = helper.popParameter();

	vector<uint8_t> *bytearray = buffer->data<LibObject<vector<uint8_t>>>()->impl;
	auto amount = fwrite(bytearray->data(), sizeof(uint8_t), bytearray->size(), file->data<LibObject<FILE>>()->impl);

	helper.returnValue(create_number(static_cast<double>(amount)));
}

MINT_FUNCTION(mint_file_fflush, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference file = move(helper.popParameter());
	fflush(file->data<LibObject<FILE>>()->impl);
}
