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

#include "mint/memory/builtin/libobject.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/reference.h"
#include "mint/system/errno.h"
#include "mint/system/utf8.h"
#include "mint/system/filesystem.h"
#include "mint/system/stdio.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <cstddef>
#include <iterator>
#include <stdio.h>
#include <string>
#include <chrono>
#include <vector>

namespace {

mint::Reference file_time_to_date(const std::filesystem::file_time_type& time) {
	return mint::create_signed_number(
	    std::chrono::duration_cast<std::chrono::milliseconds>(mint::FileSystem::to_system_time(time).time_since_epoch())
	        .count());
}

mint::Reference mint_file_read_symlink(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_string(cursor.ast(),
		        std::filesystem::read_symlink(std::filesystem::absolute(to_string(path))).generic_string()),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_birth_time(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		const std::filesystem::file_time_type time = mint::FileSystem::birth_time(
		    std::filesystem::absolute(to_string(path)));
		return create_iterator_from(cursor, file_time_to_date(time), mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_last_read_time(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		const std::filesystem::file_time_type time = mint::FileSystem::last_read_time(
		    std::filesystem::absolute(to_string(path)));
		return create_iterator_from(cursor, file_time_to_date(time), mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_last_write_time(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		const std::filesystem::file_time_type time = std::filesystem::last_write_time(
		    std::filesystem::absolute(to_string(path)));
		return create_iterator_from(cursor, file_time_to_date(time), mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_exists(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(std::filesystem::exists(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_size(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_unsigned_number(std::filesystem::file_size(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_root(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::is_root(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_regular_file(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(std::filesystem::is_regular_file(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_directory(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(std::filesystem::is_directory(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_symlink(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(std::filesystem::is_symlink(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_bundle(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::is_bundle(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_readable(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::check_file_access(std::filesystem::absolute(to_string(path)),
		        mint::FileSystem::readable_flag)),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_writable(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::check_file_access(std::filesystem::absolute(to_string(path)),
		        mint::FileSystem::writable_flag)),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_executable(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::check_file_access(std::filesystem::absolute(to_string(path)),
		        mint::FileSystem::executable_flag)),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_is_hidden(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::is_hidden(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_owner(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_string(cursor.ast(), mint::FileSystem::owner(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_owner_id(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_signed_number(mint::FileSystem::owner_id(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_group(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_string(cursor.ast(), mint::FileSystem::group(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_group_id(mint::Cursor& cursor, const mint::Reference& path) {
	try {
		return create_iterator_from(cursor,
		    mint::create_signed_number(mint::FileSystem::group_id(std::filesystem::absolute(to_string(path)))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_permission(mint::Cursor& cursor, const mint::Reference& path,
    mint::Reference& permissions) {
	try {
		return create_iterator_from(cursor,
		    mint::create_boolean(mint::FileSystem::check_file_permissions(to_string(path),
		        mint::to_integer<mint::FileSystem::Permissions>(cursor, permissions))),
		    mint::create_none());
	}
	catch (const std::filesystem::filesystem_error& error) {
		return create_iterator_from(cursor, mint::create_none(),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_file_create_symlink(mint::Cursor& /*cursor*/, const mint::Reference& source,
    const mint::Reference& target) {
	try {
		std::filesystem::create_symlink(std::filesystem::absolute(to_string(source)),
		    std::filesystem::absolute(to_string(target)));
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::Reference mint_file_copy(mint::Cursor& /*cursor*/, const mint::Reference& source,
    const mint::Reference& target) {
	try {
		std::filesystem::copy(std::filesystem::absolute(to_string(source)),
		    std::filesystem::absolute(to_string(target)),
		    std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive
		        | std::filesystem::copy_options::create_symlinks | std::filesystem::copy_options::create_hard_links);
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::Reference mint_file_rename(mint::Cursor& /*cursor*/, const mint::Reference& source,
    const mint::Reference& target) {
	try {
		std::filesystem::rename(std::filesystem::absolute(to_string(source)),
		    std::filesystem::absolute(to_string(target)));
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::Reference mint_file_remove(mint::Cursor& /*cursor*/, const mint::Reference& path) {
	try {
		if (!std::filesystem::remove(std::filesystem::absolute(to_string(path)))) {
			return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
		}
		return {};
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::Reference mint_file_fopen(mint::Cursor& cursor, const mint::Reference& path, const mint::Reference& mode) {
	try {
		if (FILE* file = mint::open_file(mint::to_string(path), mint::to_string(mode).c_str())) {
			return create_iterator_from(cursor, mint::create_c_object(cursor.ast(), file), mint::create_none());
		}
		return create_iterator_from(cursor, mint::create_null(), mint::create_number(errno));
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::Reference mint_file_fclose(mint::Cursor& /*cursor*/, mint::Reference& d_ptr) {
	if (FILE* file = d_ptr.data<mint::LibObject<FILE>>().ptr) {
		const auto status = std::fclose(file);
		d_ptr.move_data(mint::create_null());
		if (status) {
			return mint::create_number(errno);
		}
	}
	return {};
}

mint::Reference mint_file_fileno(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	const auto fd = fileno(d_ptr.data<mint::LibObject<FILE>>().ptr);
	if (fd != -1) {
		return mint::create_number(fd);
	}
	return {};
}

mint::Reference mint_file_ftell(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	const auto pos = ftell(d_ptr.data<mint::LibObject<FILE>>().ptr);
	return mint::create_iterator_from(cursor, mint::create_number(pos),
	    (pos == -1L) ? mint::create_number(errno) : mint::create_none());
}

mint::Reference mint_file_fseek(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& pos) {
	auto cursor_pos = mint::to_integer<long>(cursor, pos);
	const auto status = fseek(d_ptr.data<mint::LibObject<FILE>>().ptr, cursor_pos,
	    (cursor_pos < 0) ? SEEK_END : SEEK_SET);
	return (status != 0) ? mint::create_number(errno) : mint::create_none();
}

mint::Reference mint_file_at_end(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_boolean(std::feof(d_ptr.data<mint::LibObject<FILE>>().ptr));
}

mint::Reference mint_file_fgetc(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	if (const int cptr = fgetc(d_ptr.data<mint::LibObject<FILE>>().ptr); cptr != EOF) {
		std::string result(1, static_cast<char>(cptr));
		std::size_t length = mint::utf8_code_point_length(static_cast<std::uint8_t>(cptr));
		while (--length) {
			result += static_cast<char>(fgetc(d_ptr.data<mint::LibObject<FILE>>().ptr));
		}
		return mint::create_string(cursor.ast(), result);
	}
	return {};
}

mint::Reference mint_file_fgetw(mint::Cursor& cursor, const mint::Reference& d_ptr) {

	char* word = nullptr;

	if (const auto read = fscanf(d_ptr.data<mint::LibObject<FILE>>().ptr, "%ms", &word); read != EOF) {
		return mint::create_string(cursor.ast(), std::string(word, static_cast<std::size_t>(read)));
		std::free(word);
	}

	return {};
}

mint::Reference mint_file_readline(mint::Cursor& cursor, const mint::Reference& d_ptr) {

	if (FILE* stream = d_ptr.data<mint::LibObject<FILE>>().ptr; !std::feof(stream)) {
		auto line = mint::get_line(stream);
		line.back() = '\0';
		return mint::create_string(cursor.ast(), line);
	}

	return {};
}

mint::Reference mint_file_read(mint::Cursor& cursor, const mint::Reference& d_ptr) {

	std::string result;

	for (FILE* stream = d_ptr.data<mint::LibObject<FILE>>().ptr; !std::feof(stream);) {
		result += mint::get_line(stream);
	}

	return mint::create_string(cursor.ast(), result);
}

mint::Reference mint_file_fwrite(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& value) {

	FILE* stream = d_ptr.data<mint::LibObject<FILE>>().ptr;
	const std::string str = to_string(value);

	const auto amount = fwrite(str.data(), sizeof(char), str.size(), stream);
	return mint::create_iterator_from(cursor, mint::create_number(static_cast<double>(amount)),
	    (amount < str.size()) ? mint::create_number(errno) : mint::create_none());
}

mint::Reference mint_file_read_byte(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& buffer) {
	if (const int cptr = fgetc(d_ptr.data<mint::LibObject<FILE>>().ptr); cptr != EOF) {
		buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->push_back(static_cast<std::uint8_t>(cptr));
		return mint::create_boolean(true);
	}
	return mint::create_boolean(false);
}

mint::Reference mint_file_read_binary(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& buffer) {

	std::array<std::uint8_t, BUFSIZ> chunk = {};
	std::vector<std::uint8_t>* bytearray = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	while (!std::feof(d_ptr.data<mint::LibObject<FILE>>().ptr)) {
		const auto amount = fread(chunk.data(), sizeof(std::uint8_t), chunk.size(),
		    d_ptr.data<mint::LibObject<FILE>>().ptr);
		std::copy_n(chunk.data(), amount, std::back_inserter(*bytearray));
	}

	return mint::create_boolean(!bytearray->empty());
}

mint::Reference mint_file_fwrite_binary(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& buffer) {

	FILE* stream = d_ptr.data<mint::LibObject<FILE>>().ptr;
	std::vector<std::uint8_t>* bytearray = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	const auto amount = fwrite(bytearray->data(), sizeof(std::uint8_t), bytearray->size(), stream);

	return create_iterator_from(cursor, mint::create_number(static_cast<double>(amount)),
	    (amount < bytearray->size()) ? mint::create_number(errno) : mint::create_none());
}

mint::Reference mint_file_fflush(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	FILE* stream = d_ptr.data<mint::LibObject<FILE>>().ptr;
	const int status = std::fflush(stream);
	return status ? mint::create_number(errno) : mint::create_none();
}

}

MINT_EXPORT_FUNCTION(mint_file_read_symlink, 1)
MINT_EXPORT_FUNCTION(mint_file_birth_time, 1)
MINT_EXPORT_FUNCTION(mint_file_last_read_time, 1)
MINT_EXPORT_FUNCTION(mint_file_last_write_time, 1)
MINT_EXPORT_FUNCTION(mint_file_exists, 1)
MINT_EXPORT_FUNCTION(mint_file_size, 1)
MINT_EXPORT_FUNCTION(mint_file_is_root, 1)
MINT_EXPORT_FUNCTION(mint_file_is_regular_file, 1)
MINT_EXPORT_FUNCTION(mint_file_is_directory, 1)
MINT_EXPORT_FUNCTION(mint_file_is_symlink, 1)
MINT_EXPORT_FUNCTION(mint_file_is_bundle, 1)
MINT_EXPORT_FUNCTION(mint_file_is_readable, 1)
MINT_EXPORT_FUNCTION(mint_file_is_writable, 1)
MINT_EXPORT_FUNCTION(mint_file_is_executable, 1)
MINT_EXPORT_FUNCTION(mint_file_is_hidden, 1)
MINT_EXPORT_FUNCTION(mint_file_owner, 1)
MINT_EXPORT_FUNCTION(mint_file_owner_id, 1)
MINT_EXPORT_FUNCTION(mint_file_group, 1)
MINT_EXPORT_FUNCTION(mint_file_group_id, 1)
MINT_EXPORT_FUNCTION(mint_file_permission, 2)
MINT_EXPORT_FUNCTION(mint_file_create_symlink, 2)
MINT_EXPORT_FUNCTION(mint_file_copy, 2)
MINT_EXPORT_FUNCTION(mint_file_rename, 2)
MINT_EXPORT_FUNCTION(mint_file_remove, 1)
MINT_EXPORT_FUNCTION(mint_file_fopen, 2)
MINT_EXPORT_FUNCTION(mint_file_fclose, 1)
MINT_EXPORT_FUNCTION(mint_file_fileno, 1)
MINT_EXPORT_FUNCTION(mint_file_ftell, 1)
MINT_EXPORT_FUNCTION(mint_file_fseek, 2)
MINT_EXPORT_FUNCTION(mint_file_at_end, 1)
MINT_EXPORT_FUNCTION(mint_file_fgetc, 1)
MINT_EXPORT_FUNCTION(mint_file_fgetw, 1)
MINT_EXPORT_FUNCTION(mint_file_readline, 1)
MINT_EXPORT_FUNCTION(mint_file_read, 1)
MINT_EXPORT_FUNCTION(mint_file_fwrite, 2)
MINT_EXPORT_FUNCTION(mint_file_read_byte, 2)
MINT_EXPORT_FUNCTION(mint_file_read_binary, 2)
MINT_EXPORT_FUNCTION(mint_file_fwrite_binary, 2)
MINT_EXPORT_FUNCTION(mint_file_fflush, 1)
