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
#include "mint/memory/reference.h"
#include "mint/system/errno.h"
#include "mint/system/utf8.h"
#include "mint/system/filesystem.h"
#include "mint/system/stdio.h"

#include <filesystem>
#include <cstddef>
#include <string>
#include <chrono>

using namespace mint;

namespace {

WeakReference file_time_to_date(const std::filesystem::file_time_type &time) {
	return create_number(static_cast<double>(
		std::chrono::duration_cast<std::chrono::milliseconds>(FileSystem::to_system_time(time).time_since_epoch())
			.count()));
}

}

MINT_FUNCTION(mint_file_read_symlink, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_string(std::filesystem::read_symlink(std::filesystem::absolute(to_string(path)))
											  .generic_string()),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_birth_time, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		const std::filesystem::file_time_type time = FileSystem::birth_time(std::filesystem::absolute(to_string(path)));
		helper.return_value(create_iterator(file_time_to_date(time), WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_last_read_time, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		const std::filesystem::file_time_type time = FileSystem::last_read_time(
			std::filesystem::absolute(to_string(path)));
		helper.return_value(create_iterator(file_time_to_date(time), WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_last_write_time, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		const std::filesystem::file_time_type time = std::filesystem::last_write_time(
			std::filesystem::absolute(to_string(path)));
		helper.return_value(create_iterator(file_time_to_date(time), WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_exists, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(std::filesystem::exists(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_size, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(create_iterator(create_number(static_cast<double>(std::filesystem::file_size(
												std::filesystem::absolute(to_string(path))))),
											WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_root, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(FileSystem::is_root(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_regular_file, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(std::filesystem::is_regular_file(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_directory, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(std::filesystem::is_directory(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_symlink, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(std::filesystem::is_symlink(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_bundle, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(FileSystem::is_bundle(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_readable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(
			FileSystem::check_file_access(std::filesystem::absolute(to_string(path)), FileSystem::READABLE_FLAG)),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_writable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(
			FileSystem::check_file_access(std::filesystem::absolute(to_string(path)), FileSystem::WRITABLE_FLAG)),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_executable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(
			FileSystem::check_file_access(std::filesystem::absolute(to_string(path)), FileSystem::EXECUTABLE_FLAG)),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_is_hidden, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(FileSystem::is_hidden(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_owner, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_string(FileSystem::owner(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_owner_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_number(FileSystem::owner_id(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_group, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_string(FileSystem::group(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_group_id, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_number(FileSystem::group_id(std::filesystem::absolute(to_string(path)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_permission, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &permissions = helper.pop_parameter();
	Reference &path = helper.pop_parameter();

	try {
		helper.return_value(
			create_iterator(create_boolean(FileSystem::check_file_permissions(to_string(path),
																			  static_cast<FileSystem::Permissions>(
																				  to_number(cursor, permissions)))),
							WeakReference::create<None>()));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(
			create_iterator(WeakReference::create<None>(), create_number(mint::errno_from_error_code(error.code()))));
	}
}

MINT_FUNCTION(mint_file_create_symlink, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &target = helper.pop_parameter();
	const Reference &source = helper.pop_parameter();

	try {
		std::filesystem::create_symlink(std::filesystem::absolute(to_string(source)),
										std::filesystem::absolute(to_string(target)));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_file_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &target = helper.pop_parameter();
	const Reference &source = helper.pop_parameter();

	try {
		std::filesystem::copy(std::filesystem::absolute(to_string(source)),
							  std::filesystem::absolute(to_string(target)),
							  std::filesystem::copy_options::overwrite_existing
								  | std::filesystem::copy_options::recursive
								  | std::filesystem::copy_options::create_symlinks
								  | std::filesystem::copy_options::create_hard_links);
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_file_rename, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &target = helper.pop_parameter();
	const Reference &source = helper.pop_parameter();

	try {
		std::filesystem::rename(std::filesystem::absolute(to_string(source)),
								std::filesystem::absolute(to_string(target)));
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_file_remove, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &path = helper.pop_parameter();

	try {
		if (!std::filesystem::remove(std::filesystem::absolute(to_string(path)))) {
			helper.return_value(create_number(mint::errno_from_error_code(mint::last_error_code())));
		}
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_file_fopen, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	std::string mode = to_string(helper.pop_parameter());
	std::string path = to_string(helper.pop_parameter());

	try {
		if (FILE *file = open_file(path, mode.c_str())) {
			helper.return_value(create_iterator(create_object(file), WeakReference::create<None>()));
		}
		else {
			helper.return_value(create_iterator(WeakReference::create<Null>(), create_number(errno)));
		}
	}
	catch (const std::filesystem::filesystem_error &error) {
		helper.return_value(create_number(mint::errno_from_error_code(error.code())));
	}
}

MINT_FUNCTION(mint_file_fclose, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	Reference &file = helper.pop_parameter();

	if (file.data<LibObject<FILE>>()->impl) {
		int status = fclose(file.data<LibObject<FILE>>()->impl);
		file.move_data(WeakReference::create<Null>());
		helper.return_value(status ? create_number(errno) : WeakReference::create<None>());
	}
}

MINT_FUNCTION(mint_file_fileno, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	const Reference &file = helper.pop_parameter();
	int fd = fileno(file.data<LibObject<FILE>>()->impl);

	if (fd != -1) {
		helper.return_value(create_number(fd));
	}
}

MINT_FUNCTION(mint_file_ftell, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &file = helper.pop_parameter();

	long pos = ftell(file.data<LibObject<FILE>>()->impl);

	Reference &&result = create_iterator();
	iterator_yield(result.data<Iterator>(), create_number(pos));
	iterator_yield(result.data<Iterator>(), (pos == -1L) ? create_number(errno) : WeakReference::create<None>());
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_file_fseek, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &pos = helper.pop_parameter();
	Reference &file = helper.pop_parameter();

	auto cursor_pos = static_cast<long>(to_integer(cursor, pos));
	int status = fseek(file.data<LibObject<FILE>>()->impl, cursor_pos, (cursor_pos < 0) ? SEEK_END : SEEK_SET);
	helper.return_value((status != 0) ? create_number(errno) : WeakReference::create<None>());
}

MINT_FUNCTION(mint_file_at_end, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	const Reference &file = helper.pop_parameter();
	helper.return_value(create_boolean(feof(file.data<LibObject<FILE>>()->impl)));
}

MINT_FUNCTION(mint_file_fgetc, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	const Reference &file = helper.pop_parameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		std::string result(1, static_cast<char>(cptr));
		size_t length = utf8_code_point_length(static_cast<uint8_t>(cptr));
		while (--length) {
			result += static_cast<char>(fgetc(file.data<LibObject<FILE>>()->impl));
		}
		helper.return_value(create_string(result));
	}
}

MINT_FUNCTION(mint_file_fgetw, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	const Reference &file = helper.pop_parameter();

	char *word = nullptr;

	if (ssize_t read = fscanf(file.data<LibObject<FILE>>()->impl, "%ms", &word); read != EOF) {
		helper.return_value(create_string(std::string {word, static_cast<size_t>(read)}));
		free(word);
	}
}

MINT_FUNCTION(mint_file_readline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	const Reference &file = helper.pop_parameter();

	size_t len = 0;
	char *line = nullptr;

	if (ssize_t read = getline(&line, &len, file.data<LibObject<FILE>>()->impl); read != EOF) {
		line[read - 1] = '\0';
		helper.return_value(create_string(line));
		free(line);
	}
}

MINT_FUNCTION(mint_file_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	const Reference &file = helper.pop_parameter();

	std::string result;
	ssize_t read = 0;
	size_t len = 0;
	char *line = nullptr;

	while ((read = getline(&line, &len, file.data<LibObject<FILE>>()->impl)) != EOF) {
		result += line;
	}

	free(line);
	helper.return_value(create_string(result));
}

MINT_FUNCTION(mint_file_fwrite, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	Reference &value = helper.pop_parameter();
	Reference &file = helper.pop_parameter();

	FILE *stream = file.data<LibObject<FILE>>()->impl;
	std::string str = to_string(value);

	auto amount = fwrite(str.c_str(), sizeof(char), str.size(), stream);

	Reference &&result = create_iterator();
	iterator_yield(result.data<Iterator>(), create_number(static_cast<double>(amount)));
	iterator_yield(result.data<Iterator>(),
					(amount < str.size()) ? create_number(errno) : WeakReference::create<None>());
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_file_read_byte, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	const Reference &buffer = helper.pop_parameter();
	const Reference &file = helper.pop_parameter();

	int cptr = fgetc(file.data<LibObject<FILE>>()->impl);

	if (cptr != EOF) {
		buffer.data<LibObject<std::vector<uint8_t>>>()->impl->push_back(static_cast<uint8_t>(cptr));
		helper.return_value(create_boolean(true));
	}
	else {
		helper.return_value(create_boolean(false));
	}
}

MINT_FUNCTION(mint_file_read_binary, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	const Reference &buffer = helper.pop_parameter();
	const Reference &file = helper.pop_parameter();

	uint8_t chunk[BUFSIZ];
	std::vector<uint8_t> *bytearray = buffer.data<LibObject<std::vector<uint8_t>>>()->impl;

	while (!feof(file.data<LibObject<FILE>>()->impl)) {
		auto amount = fread(chunk, sizeof(uint8_t), sizeof(chunk), file.data<LibObject<FILE>>()->impl);
		copy_n(chunk, amount, back_inserter(*bytearray));
	}

	helper.return_value(create_boolean(!bytearray->empty()));
}

MINT_FUNCTION(mint_file_fwrite_binary, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	Reference &buffer = helper.pop_parameter();
	Reference &file = helper.pop_parameter();

	FILE *stream = file.data<LibObject<FILE>>()->impl;
	std::vector<uint8_t> *bytearray = buffer.data<LibObject<std::vector<uint8_t>>>()->impl;

	auto amount = fwrite(bytearray->data(), sizeof(uint8_t), bytearray->size(), stream);

	helper.return_value(
		create_iterator(create_number(static_cast<double>(amount)),
						(amount < bytearray->size()) ? create_number(errno) : WeakReference::create<None>()));
}

MINT_FUNCTION(mint_file_fflush, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &file = helper.pop_parameter();
	FILE *stream = file.data<LibObject<FILE>>()->impl;
	int status = fflush(stream);
	helper.return_value(status ? create_number(errno) : WeakReference::create<None>());
}
