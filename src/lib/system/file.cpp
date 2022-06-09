#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/utf8iterator.h"
#include "system/filesystem.h"
#include "system/stdio.h"

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_file_symlink_target, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::symlinkTarget(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_birth_time, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::birthTime(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_last_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::lastRead(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_last_modified, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::lastModified(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_exists, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::exists)));
}

MINT_FUNCTION(mint_file_size, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_number(static_cast<double>(FileSystem::sizeOf(FileSystem::instance().absolutePath(to_string(path))))));
}

MINT_FUNCTION(mint_file_is_root, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isRoot(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_file, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isFile(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_directory, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isDirectory(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_symlink, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isSymlink(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_bundle, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isBundle(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_is_readable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::readable)));
}

MINT_FUNCTION(mint_file_is_writable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::writable)));
}

MINT_FUNCTION(mint_file_is_executable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFileAccess(FileSystem::instance().absolutePath(to_string(path)), FileSystem::executable)));
}

MINT_FUNCTION(mint_file_is_hidden, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::isHidden(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_owner, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::owner(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_owner_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_number(FileSystem::ownerId(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_group, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_string(FileSystem::group(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_group_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_number(FileSystem::groupId(FileSystem::instance().absolutePath(to_string(path)))));
}

MINT_FUNCTION(mint_file_permission, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference permissions = move(helper.popParameter());
	WeakReference path = move(helper.popParameter());
	helper.returnValue(create_boolean(FileSystem::checkFilePermissions(to_string(path), static_cast<FileSystem::Permissions>(to_number(cursor, permissions)))));
}

MINT_FUNCTION(mint_file_link, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference target = move(helper.popParameter());
	WeakReference source = move(helper.popParameter());

	FileSystem::Status status = FileSystem::instance().createLink(FileSystem::instance().absolutePath(to_string(source)), FileSystem::instance().absolutePath(to_string(target)));

	if (status) {
		helper.returnValue(WeakReference::create<None>());
	}
	else {
		helper.returnValue(create_number(status.getErrno()));
	}
}

MINT_FUNCTION(mint_file_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference target = move(helper.popParameter());
	WeakReference source = move(helper.popParameter());

	FileSystem::Status status = FileSystem::instance().copy(FileSystem::instance().absolutePath(to_string(source)), FileSystem::instance().absolutePath(to_string(target)));

	if (status) {
		helper.returnValue(WeakReference::create<None>());
	}
	else {
		helper.returnValue(create_number(status.getErrno()));
	}
}

MINT_FUNCTION(mint_file_rename, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference target = move(helper.popParameter());
	WeakReference source = move(helper.popParameter());

	FileSystem::Status status = FileSystem::instance().rename(FileSystem::instance().absolutePath(to_string(source)), FileSystem::instance().absolutePath(to_string(target)));

	if (status) {
		helper.returnValue(WeakReference::create<None>());
	}
	else {
		helper.returnValue(create_number(status.getErrno()));
	}
}

MINT_FUNCTION(mint_file_remove, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference path = move(helper.popParameter());

	FileSystem::Status status = FileSystem::instance().remove(FileSystem::instance().absolutePath(to_string(path)));

	if (status) {
		helper.returnValue(WeakReference::create<None>());
	}
	else {
		helper.returnValue(create_number(status.getErrno()));
	}
}

MINT_FUNCTION(mint_file_fopen, 2,cursor) {

	FunctionHelper helper(cursor, 2);

	string mode = to_string(helper.popParameter());
	string path = to_string(helper.popParameter());

	Reference &&result = create_iterator();
	if (FILE *file = open_file(path.c_str(), mode.c_str())) {
		iterator_insert(result.data<Iterator>(), create_object(file));
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<Null>());
		iterator_insert(result.data<Iterator>(), create_number(errno));
	}
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_file_fclose, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = helper.popParameter();

	if (file.data<LibObject<FILE>>()->impl) {
		int status = fclose(file.data<LibObject<FILE>>()->impl);
		file.move(WeakReference::create<Null>());
		helper.returnValue(status ? create_number(errno) : WeakReference::create<None>());
	}
}

MINT_FUNCTION(mint_file_fileno, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = helper.popParameter();
	int fd = fileno(file.data<LibObject<FILE>>()->impl);

	if (fd != -1) {
		helper.returnValue(create_number(fd));
	}
}

MINT_FUNCTION(mint_file_ftell, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &file = helper.popParameter();

	long pos = ftell(file.data<LibObject<FILE>>()->impl);

	Reference &&result = create_iterator();
	iterator_insert(result.data<Iterator>(), create_number(pos));
	iterator_insert(result.data<Iterator>(), (pos == -1L) ? create_number(errno) : WeakReference::create<None>());
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_file_fseek, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &pos = helper.popParameter();
	Reference &file = helper.popParameter();

	long cursor_pos = to_integer(cursor, pos);
	int status = fseek(file.data<LibObject<FILE>>()->impl, cursor_pos, (cursor_pos < 0) ? SEEK_END : SEEK_SET);
	helper.returnValue((status != 0) ? create_number(errno) : WeakReference::create<None>());
}

MINT_FUNCTION(mint_file_at_end, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = helper.popParameter();
	helper.returnValue(create_boolean(feof(file.data<LibObject<FILE>>()->impl)));
}

MINT_FUNCTION(mint_file_fgetc, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = helper.popParameter();

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

	Reference &file = helper.popParameter();

	ssize_t read;
	char *word = nullptr;

	if ((read = fscanf(file.data<LibObject<FILE>>()->impl, "%ms", &word)) != EOF) {
		helper.returnValue(create_string(word));
		free(word);
	}
}

MINT_FUNCTION(mint_file_readline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = helper.popParameter();

	ssize_t read;
	size_t len = 0;
	char *line = nullptr;

	if ((read = getline(&line, &len, file.data<LibObject<FILE>>()->impl)) != EOF) {
		line[read - 1] = '\0';
		helper.returnValue(create_string(line));
		free(line);
	}
}

MINT_FUNCTION(mint_file_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = helper.popParameter();

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

	Reference &value = helper.popParameter();
	Reference &file = helper.popParameter();

	FILE *stream = file.data<LibObject<FILE>>()->impl;
	string str = to_string(value);

	auto amount = fwrite(str.c_str(), sizeof(char), str.size(), stream);

	Reference &&result = create_iterator();
	iterator_insert(result.data<Iterator>(), create_number(static_cast<double>(amount)));
	iterator_insert(result.data<Iterator>(), (amount < str.size()) ? create_number(errno) : WeakReference::create<None>());
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_file_read_byte, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	Reference &buffer = helper.popParameter();
	Reference &file = helper.popParameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		buffer.data<LibObject<vector<uint8_t>>>()->impl->push_back(static_cast<uint8_t>(cptr));
		helper.returnValue(create_boolean(true));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}

MINT_FUNCTION(mint_file_read_binary, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	Reference &buffer = helper.popParameter();
	Reference &file = helper.popParameter();

	uint8_t chunk[BUFSIZ];
	vector<uint8_t> *bytearray = buffer.data<LibObject<vector<uint8_t>>>()->impl;

	while (!feof(file.data<LibObject<FILE>>()->impl)) {
		auto amount = fread(chunk, sizeof(uint8_t), sizeof(chunk), file.data<LibObject<FILE>>()->impl);
		copy_n(chunk, amount, back_inserter(*bytearray));
	}

	helper.returnValue(create_boolean(!bytearray->empty()));
}

MINT_FUNCTION(mint_file_fwrite_binary, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	Reference &buffer = helper.popParameter();
	Reference &file = helper.popParameter();

	FILE *stream = file.data<LibObject<FILE>>()->impl;
	vector<uint8_t> *bytearray = buffer.data<LibObject<vector<uint8_t>>>()->impl;

	auto amount = fwrite(bytearray->data(), sizeof(uint8_t), bytearray->size(), stream);

	Reference &&result = create_iterator();
	iterator_insert(result.data<Iterator>(), create_number(static_cast<double>(amount)));
	iterator_insert(result.data<Iterator>(), (amount < bytearray->size()) ? create_number(errno) : WeakReference::create<None>());
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_file_fflush, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference file = move(helper.popParameter());
	FILE *stream = file.data<LibObject<FILE>>()->impl;
	int status = fflush(stream);
	helper.returnValue(status ? create_number(errno) : WeakReference::create<None>());
}
