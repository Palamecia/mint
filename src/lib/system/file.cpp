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

#include "mint/ast/cursor.h"
#include "mint/config.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/async_io.h"
#include "mint/system/errno.h"
#include "mint/system/filesystem.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <stdio.h>
#include <string>
#include <chrono>
#include <string_view>
#include <sys/stat.h>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <bit>
#include <Windows.h>
#include <corecrt_io.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winbase.h>
#include <winerror.h>
#include <winnt.h>
#else
#include <asm-generic/int-ll64.h>
#include <bits/types.h>
#endif

#ifdef MINT_ASYNC_BACKEND_IO_URING
#include <liburing.h>
#include <unistd.h>
#endif

namespace {

#ifdef MINT_OS_WINDOWS
struct CreateFileMode {
	DWORD access = 0;
	DWORD disposition = 0;
};

CreateFileMode fopen_mode_to_createfile(const std::string& mode) {

	const auto* mode_str = mode.c_str();
	bool read = false;
	bool write = false;
	bool append = false;
	bool plus = false;

	switch (mode_str[0]) {
	case 'r':
		read = true;
		break;

	case 'w':
		write = true;
		break;

	case 'a':
		write = true;
		append = true;
		break;

	default:
		throw std::system_error(std::make_error_code(std::errc::invalid_argument));
	}

	for (const char* p = mode_str + 1; *p; ++p) {
		switch (*p) {
		case '+':
			plus = true;
			break;

		case 'b':
		case 't':
			/* ignored */
			break;

		default:
			throw std::system_error(std::make_error_code(std::errc::invalid_argument));
		}
	}

	if (plus) {
		read = true;
		write = true;
	}

	auto createfile_mode = CreateFileMode {};

	if (read) {
		createfile_mode.access |= GENERIC_READ;
	}
	if (write) {
		createfile_mode.access |= GENERIC_WRITE;
	}

	switch (mode_str[0]) {
	case 'r':
		createfile_mode.disposition = OPEN_EXISTING;
		break;

	case 'w':
		createfile_mode.disposition = CREATE_ALWAYS;
		break;

	case 'a':
		createfile_mode.disposition = OPEN_ALWAYS;
		break;

	default:
		throw std::system_error(std::make_error_code(std::errc::invalid_argument));
	}

	return createfile_mode;
}
#endif

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

mint::Reference mint_file_permission(mint::Cursor& cursor, const mint::Reference& path, mint::Reference& permissions) {
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

mint::Reference mint_file_copy(mint::Cursor& /*cursor*/, const mint::Reference& source, const mint::Reference& target) {
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

mint::Reference mint_file_open(mint::Cursor& cursor, const mint::Reference& path, const mint::Reference& mode) {
	try {
		if (const auto fd = mint::open_file_descriptor(mint::to_string(path), mint::to_string(mode).c_str()); fd != -1) {
			return create_iterator_from(cursor, mint::create_signed_number(fd), mint::create_none());
		}
		return create_iterator_from(cursor, mint::create_null(), mint::create_number(errno));
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::Reference mint_file_close(mint::Cursor& cursor, mint::Reference& d_ptr) {
	if (const auto fd = mint::to_integer<int>(cursor, d_ptr); fd != -1) {
		if (close(fd) != 0) {
			d_ptr.move_data(mint::create_null());
			return mint::create_number(errno);
		}
		d_ptr.move_data(mint::create_null());
	}
	return {};
}

mint::Reference mint_file_get_handle(mint::Cursor& cursor, mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	return mint::create_handle(cursor.ast(),
	    std::bit_cast<mint::handle_t>(_get_osfhandle(mint::to_integer<int>(cursor, d_ptr))));
#else
	return mint::create_handle(cursor.ast(), std::bit_cast<mint::handle_t>(mint::to_integer<int>(cursor, d_ptr)));
#endif
}

mint::Reference mint_file_tell(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	const auto pos = lseek(mint::to_integer<int>(cursor, d_ptr), 0, SEEK_CUR);
	return mint::create_iterator_from(cursor, (pos == -1L) ? mint::create_number(errno) : mint::create_number(0),
	    mint::create_number(pos));
}

mint::Reference mint_file_seek(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& pos) {
	auto cursor_pos = mint::to_integer<long>(cursor, pos);
	if (lseek(mint::to_integer<int>(cursor, d_ptr), cursor_pos, (cursor_pos < 0) ? SEEK_END : SEEK_SET) < 0) {
		return mint::create_number(errno);
	}
	return mint::create_number(0);
}

mint::Reference mint_file_at_end(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	const auto fd = mint::to_integer<int>(cursor, d_ptr);
	const auto pos = lseek(fd, 0, SEEK_CUR);
	struct stat file_info = {};
	fstat(fd, &file_info);
	return mint::create_boolean(pos == file_info.st_size);
}

mint::Reference mint_file_read(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& buffer) {

	auto local_buffer = std::array<std::uint8_t, BUFSIZ>();
	auto* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	const auto fd = mint::to_integer<int>(cursor, d_ptr);

	for (;;) {
		const auto bytes_read = read(fd, local_buffer.data(), local_buffer.size());
		if (bytes_read > 0) {
			buf->append_range(std::span(local_buffer.data(), bytes_read));
		}
		else if (bytes_read == 0) {
			return mint::create_number(0);
		}
		else if (errno != EINTR) {
			return mint::create_number(errno);
		}
	}
}

mint::Reference mint_file_read_some(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& buffer,
    const mint::Reference& count) {

	const auto local_buffer_length = mint::to_integer<std::size_t>(cursor, count);
	auto local_buffer = std::make_unique<char[]>(local_buffer_length);
	auto* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	const auto fd = mint::to_integer<int>(cursor, d_ptr);

	for (;;) {
		const auto bytes_read = read(fd, local_buffer.get(), local_buffer_length);
		if (bytes_read >= 0) {
			buf->append_range(std::span(local_buffer.get(), bytes_read));
			return mint::create_number(0);
		}
		if (errno != EINTR) {
			return mint::create_number(errno);
		}
	}
}

mint::Reference mint_file_write(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& buffer) {

	const auto fd = mint::to_integer<int>(cursor, d_ptr);
	auto* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	const auto bytes_transferred = write(fd, buf->data(), buf->size());
	return mint::create_iterator_from(cursor,
	    (bytes_transferred < buf->size()) ? mint::create_number(errno) : mint::create_number(0),
	    mint::create_unsigned_number(bytes_transferred));
}

mint::Reference mint_file_flush(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	const auto fd = mint::to_integer<int>(cursor, d_ptr);
#ifdef MINT_OS_WINDOWS
	if (_commit(fd) != 0) {
		return mint::create_number(errno);
	}
#else
	if (fsync(fd) == -1) {
		return mint::create_number(errno);
	}
#endif
	return {};
}

mint::Reference mint_file_open_async(mint::Cursor& cursor, const mint::Reference& path, const mint::Reference& mode) {
	try {
#ifdef MINT_ASYNC_BACKEND_IOCP
		const auto createfile_mode = fopen_mode_to_createfile(mint::to_string(mode));
		auto* handle = CreateFileW(std::filesystem::path(mint::to_string(path)).generic_wstring().data(),
		    createfile_mode.access, FILE_SHARE_READ, nullptr, createfile_mode.disposition,
		    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
		if (handle != INVALID_HANDLE_VALUE) {
			return create_iterator_from(cursor, mint::create_handle(cursor.ast(), handle), mint::create_none());
		}
#elifdef MINT_OS_LINUX
		const auto fd = mint::open_file_descriptor(mint::to_string(path), mint::to_string(mode).data());
		if (fd != -1) {
			return create_iterator_from(cursor, mint::create_handle(cursor.ast(), fd), mint::create_none());
		}
#else
#error "This operation is not implemented for this platform"
#endif
		return create_iterator_from(cursor, mint::create_null(), mint::create_number(errno));
	}
	catch (const std::filesystem::filesystem_error& error) {
		return mint::create_number(mint::errno_from_error_code(error.code()));
	}
}

mint::Reference mint_file_close_async(mint::Cursor& /*cursor*/, mint::Reference& d_ptr) {
#ifdef MINT_ASYNC_BACKEND_IOCP
	if (auto* handle = mint::to_handle(d_ptr); handle != mint::invalid_handle) {
		if (!CloseHandle(handle)) {
			d_ptr.move_data(mint::create_null());
			return mint::create_number(mint::errno_from_last_error());
		}
		d_ptr.move_data(mint::create_null());
	}
#elifdef MINT_OS_LINUX
	if (auto fd = mint::to_handle(d_ptr); fd != mint::invalid_handle) {
		if (close(fd) != 0) {
			d_ptr.move_data(mint::create_null());
			return mint::create_number(mint::errno_from_last_error());
		}
		d_ptr.move_data(mint::create_null());
	}
#else
#error "This operation is not implemented for this platform"
#endif
	return {};
}

mint::Reference mint_file_tell_async(mint::Cursor& cursor, const mint::Reference& d_ptr) {
#ifdef MINT_ASYNC_BACKEND_IOCP
	auto pos = LARGE_INTEGER {};
	if (!::SetFilePointerEx(mint::to_handle(d_ptr), {}, &pos, FILE_CURRENT)) {
		return mint::create_iterator_from(cursor, mint::create_number(mint::errno_from_last_error()));
	}
	return mint::create_iterator_from(cursor, mint::create_number(0), mint::create_signed_number(pos.QuadPart));
#elifdef MINT_OS_LINUX
	const auto pos = lseek(mint::to_handle(d_ptr), 0, SEEK_CUR);
	return mint::create_iterator_from(cursor, (pos == -1L) ? mint::create_number(errno) : mint::create_number(0),
	    mint::create_number(pos));
#else
#error "This operation is not implemented for this platform"
#endif
}

mint::Reference mint_file_seek_async(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& pos) {
#ifdef MINT_ASYNC_BACKEND_IOCP
	const auto offset = mint::to_integer<LONGLONG>(cursor, pos);
	auto cursor_pos = LARGE_INTEGER {
	    .QuadPart = std::abs(offset),
	};
	if (!::SetFilePointerEx(mint::to_handle(d_ptr), cursor_pos, nullptr, (offset < 0) ? FILE_END : FILE_BEGIN)) {
		return mint::create_number(mint::errno_from_last_error());
	}
	return mint::create_number(0);
#elifdef MINT_OS_LINUX
	auto cursor_pos = mint::to_integer<long>(cursor, pos);
	if (lseek(mint::to_handle(d_ptr), cursor_pos, (cursor_pos < 0) ? SEEK_END : SEEK_SET) < 0) {
		return mint::create_number(errno);
	}
	return mint::create_number(0);
#else
#error "This operation is not implemented for this platform"
#endif
}

mint::Reference mint_file_at_end_async(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_ASYNC_BACKEND_IOCP
	auto pos = LARGE_INTEGER {};
	SetFilePointerEx(mint::to_handle(d_ptr), pos, &pos, FILE_CURRENT);
	auto size = LARGE_INTEGER {};
	GetFileSizeEx(mint::to_handle(d_ptr), &size);
	return mint::create_boolean(pos.QuadPart >= size.QuadPart);
#elifdef MINT_OS_LINUX
	const auto pos = lseek(mint::to_handle(d_ptr), 0, SEEK_CUR);
	struct stat file_info = {};
	fstat(mint::to_handle(d_ptr), &file_info);
	return mint::create_boolean(pos == file_info.st_size);
#else
#error "This operation is not implemented for this platform"
#endif
}

mint::Reference mint_file_read_async(mint::Cursor& cursor, mint::Reference& self, const mint::Reference& scheduler,
    const mint::Reference& d_ptr, const mint::Reference& buffer) {
	class AsyncReadOperation : public mint::MintAsyncOperation {
#ifdef MINT_ASYNC_BACKEND_EPOLL
		std::reference_wrapper<mint::AsyncRuntime> _scheduler;
		std::future<void> _future;
		std::error_code _error;
#endif
		std::vector<std::uint8_t>* _buf;
		std::array<char, BUFSIZ> _local_buffer {};
#ifdef MINT_ASYNC_BACKEND_IOCP
		LARGE_INTEGER _pos {};
#elifdef MINT_OS_LINUX
		__off_t _pos {};
#endif
	public:
		AsyncReadOperation(mint::Reference self, mint::AsyncRuntime& scheduler, mint::handle_t handle,
		    std::vector<std::uint8_t>* buf) :
		    mint::MintAsyncOperation(std::move(self), handle),
#ifdef MINT_ASYNC_BACKEND_EPOLL
		    _scheduler(scheduler),
#endif
		    _buf(buf) {
		}

		std::error_code start() override {

#ifdef MINT_ASYNC_BACKEND_IOCP
			if (!::SetFilePointerEx(get_handle(), {}, &_pos, FILE_CURRENT)) {
				return mint::last_error_code();
			}

			Offset = static_cast<DWORD>(_pos.LowPart);
			OffsetHigh = static_cast<DWORD>(_pos.HighPart);

			if (!::ReadFile(get_handle(), _local_buffer.data(), static_cast<DWORD>(_local_buffer.size()), nullptr,
			        this)) {
				switch (GetLastError()) {
				case ERROR_IO_PENDING:
					break;
				default:
					return mint::last_error_code();
				}
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      _pos = lseek(get_handle(), 0, SEEK_CUR);
				                      io_uring_prep_read(self.sqe, get_handle(), _local_buffer.data(),
				                          static_cast<unsigned>(_local_buffer.size()), static_cast<__u64>(_pos));
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      _future = std::async([&]() {
					                      const auto result = pread64(get_handle(), _local_buffer.data(),
					                          _local_buffer.size(), static_cast<__off64_t>(_pos));
					                      if (result != -1) {
						                      self.result = result;
					                      }
					                      else {
						                      _error = mint::last_error_code();
					                      }
					                      _scheduler.get().post_deferred_completion(*this);
				                      });
				                      return {};
			                      },
#endif
			                      [](std::monostate) -> std::error_code {
				                      return std::make_error_code(std::errc::not_supported);
			                      },
			                  },
			    *this);
#else
#error "This operation is not implemented for this platform"
#endif

			return {};
		}

		void complete(std::error_code error, std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_number(error.value()));
			}
#ifdef MINT_ASYNC_BACKEND_IOCP
			else {
				_pos.QuadPart += static_cast<LONGLONG>(bytes_transferred);
				if (!::SetFilePointerEx(get_handle(), _pos, nullptr, FILE_BEGIN)) {
					done(mint::create_number(error.value()));
				}
				else {
					_buf->append_range(std::span(_local_buffer.data(), bytes_transferred));
					done(mint::create_number(0));
				}
			}
#elifdef MINT_OS_LINUX
			_pos += static_cast<__off_t>(bytes_transferred);
			if (lseek(get_handle(), _pos, SEEK_SET) < 0) {
				done(mint::create_number(mint::errno_from_last_error()));
			}
			else {
				_buf->append_range(std::span(_local_buffer.data(), bytes_transferred));
				done(mint::create_number(0));
			}
#else
#error "This operation is not implemented for this platform"
#endif
		}
	};

	return mint::create_async_operation(cursor.ast(),
	    new AsyncReadOperation(std::move(self), *scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr,
	        mint::to_handle(d_ptr), buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr));
}

mint::Reference mint_file_read_some_async(mint::Cursor& cursor, mint::Reference& self, const mint::Reference& scheduler,
    const mint::Reference& d_ptr, const mint::Reference& buffer, const mint::Reference& count) {
	class AsyncReadSomeOperation : public mint::MintAsyncOperation {
#ifdef MINT_ASYNC_BACKEND_EPOLL
		std::reference_wrapper<mint::AsyncRuntime> _scheduler;
		std::future<void> _future;
		std::error_code _error;
#endif
		std::vector<std::uint8_t>* _buf;
		std::unique_ptr<char[]> _local_buffer;
		std::size_t _buffer_length;
#ifdef MINT_ASYNC_BACKEND_IOCP
		LARGE_INTEGER _pos {};
#elifdef MINT_OS_LINUX
		__off_t _pos {};
#endif
	public:
		AsyncReadSomeOperation(mint::Reference self, mint::AsyncRuntime& scheduler, mint::handle_t handle,
		    std::vector<std::uint8_t>* buf, std::size_t count) :
		    mint::MintAsyncOperation(std::move(self), handle),
#ifdef MINT_ASYNC_BACKEND_EPOLL
		    _scheduler(scheduler),
#endif
		    _buf(buf),
		    _local_buffer(std::make_unique<char[]>(count)),
		    _buffer_length(count) {
		}

		std::error_code start() override {

#ifdef MINT_ASYNC_BACKEND_IOCP
			if (!::SetFilePointerEx(get_handle(), {}, &_pos, FILE_CURRENT)) {
				return mint::last_error_code();
			}

			Offset = static_cast<DWORD>(_pos.LowPart);
			OffsetHigh = static_cast<DWORD>(_pos.HighPart);

			if (!::ReadFile(get_handle(), _local_buffer.get(), static_cast<DWORD>(_buffer_length), nullptr, this)) {
				switch (GetLastError()) {
				case ERROR_IO_PENDING:
					break;
				default:
					return mint::last_error_code();
				}
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      _pos = lseek(get_handle(), 0, SEEK_CUR);
				                      io_uring_prep_read(self.sqe, get_handle(), _local_buffer.get(),
				                          static_cast<unsigned>(_buffer_length), static_cast<__u64>(_pos));
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      _future = std::async([&]() {
					                      const auto result = pread64(get_handle(), _local_buffer.get(), _buffer_length,
					                          static_cast<__off64_t>(_pos));
					                      if (result != -1) {
						                      self.result = result;
					                      }
					                      else {
						                      _error = mint::last_error_code();
					                      }
					                      _scheduler.get().post_deferred_completion(*this);
				                      });
				                      return {};
			                      },
#endif
			                      [](std::monostate) -> std::error_code {
				                      return std::make_error_code(std::errc::not_supported);
			                      },
			                  },
			    *this);
#else
#error "This operation is not implemented for this platform"
#endif

			return {};
		}

		void complete(std::error_code error, std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_number(error.value()));
			}
#ifdef MINT_ASYNC_BACKEND_IOCP
			else {
				_pos.QuadPart += static_cast<LONGLONG>(bytes_transferred);
				if (!::SetFilePointerEx(get_handle(), _pos, nullptr, FILE_BEGIN)) {
					done(mint::create_number(mint::errno_from_last_error()));
				}
				else {
					_buf->append_range(std::span(_local_buffer.get(), bytes_transferred));
					done(mint::create_number(0));
				}
			}
#elifdef MINT_OS_LINUX
#ifdef MINT_ASYNC_BACKEND_EPOLL
			if (_error) {
				done(mint::create_number(_error.value()));
			}
#endif
			_pos += static_cast<__off_t>(bytes_transferred);
			if (lseek(get_handle(), _pos, SEEK_SET) < 0) {
				done(mint::create_number(mint::errno_from_last_error()));
			}
			else {
				_buf->append_range(std::span(_local_buffer.get(), bytes_transferred));
				done(mint::create_number(0));
			}
#else
#error "This operation is not implemented for this platform"
#endif
		}
	};

	return mint::create_async_operation(cursor.ast(),
	    new AsyncReadSomeOperation(std::move(self), *scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr,
	        mint::to_handle(d_ptr), buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr,
	        mint::to_integer<std::size_t>(cursor, count)));
}

mint::Reference mint_file_write_async(mint::Cursor& cursor, mint::Reference& self, const mint::Reference& scheduler,
    const mint::Reference& d_ptr, const mint::Reference& buffer) {
	class AsyncWriteOperation : public mint::MintAsyncOperation {
#ifdef MINT_ASYNC_BACKEND_EPOLL
		std::reference_wrapper<mint::AsyncRuntime> _scheduler;
		std::future<void> _future;
		std::error_code _error;
#endif
		std::reference_wrapper<mint::Cursor> _cursor;
		std::span<std::uint8_t> _buffer;
	public:
		AsyncWriteOperation(mint::Cursor& cursor, mint::Reference self, mint::AsyncRuntime& scheduler,
		    mint::handle_t handle, std::span<std::uint8_t> buffer) :
		    mint::MintAsyncOperation(std::move(self), handle),
#ifdef MINT_ASYNC_BACKEND_EPOLL
		    _scheduler(scheduler),
#endif
		    _cursor(cursor),
		    _buffer(buffer) {
		}

		std::error_code start() override {

#ifdef MINT_ASYNC_BACKEND_IOCP
			if (!WriteFile(get_handle(), _buffer.data(), static_cast<DWORD>(_buffer.size()), nullptr, this)) {
				switch (GetLastError()) {
				case ERROR_IO_PENDING:
					break;
				default:
					return mint::last_error_code();
				}
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      io_uring_prep_write(self.sqe, get_handle(), _buffer.data(),
				                          static_cast<unsigned>(_buffer.size()), -1);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      _future = std::async([&]() {
					                      const auto result = write(get_handle(), _buffer.data(), _buffer.size());
					                      if (result != -1) {
						                      self.result = result;
					                      }
					                      else {
						                      _error = mint::last_error_code();
					                      }
					                      _scheduler.get().post_deferred_completion(*this);
				                      });
				                      return {};
			                      },
#endif
			                      [](std::monostate) -> std::error_code {
				                      return std::make_error_code(std::errc::not_supported);
			                      },
			                  },
			    *this);
#else
#error "This operation is not implemented for this platform"
#endif

			return {};
		}

		void complete(std::error_code error, std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_iterator_from(_cursor, mint::create_number(error.value())));
			}
			else {
				done(mint::create_iterator_from(_cursor, mint::create_number(0),
				    mint::create_unsigned_number(bytes_transferred)));
			}
		}
	};

	return mint::create_async_operation(cursor.ast(),
	    new AsyncWriteOperation(cursor, std::move(self), *scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr,
	        mint::to_handle(d_ptr), *buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr));
}

mint::Reference mint_file_flush_async(mint::Cursor& cursor, mint::Reference& self, const mint::Reference& scheduler,
    const mint::Reference& d_ptr) {
	class AsyncFlushOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::AsyncRuntime> _scheduler;
		std::future<void> _future;
		std::error_code _error;
	public:
		AsyncFlushOperation(mint::Reference self, mint::AsyncRuntime& scheduler, mint::handle_t handle) :
		    mint::MintAsyncOperation(std::move(self), handle),
		    _scheduler(scheduler) {}

		std::error_code start() override {

#ifdef MINT_ASYNC_BACKEND_IOCP
			_future = std::async([this]() {
				if (!FlushFileBuffers(get_handle())) {
					_error = mint::last_error_code();
				}
				_scheduler.get().post_deferred_completion(*this);
			});
#elifdef MINT_OS_LINUX
			_future = std::async([this]() {
				if (fsync(get_handle()) == -1) {
					_error = mint::last_error_code();
				}
				_scheduler.get().post_deferred_completion(*this);
			});
#else
#error "This operation is not implemented for this platform"
#endif

			return {};
		}

		void complete(std::error_code error, std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_number(error.value()));
			}
#if defined(MINT_ASYNC_BACKEND_IOCP) || defined(MINT_ASYNC_BACKEND_IO_URING)
			else if (_error) {
				done(mint::create_number(_error.value()));
			}
#endif
			else {
				done(mint::create_none());
			}
		}
	};

	return mint::create_async_operation(cursor.ast(),
	    new AsyncFlushOperation(std::move(self), *scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr,
	        mint::to_handle(d_ptr)));
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

MINT_EXPORT_FUNCTION(mint_file_open, 2)
MINT_EXPORT_FUNCTION(mint_file_close, 1)
MINT_EXPORT_FUNCTION(mint_file_get_handle, 1)
MINT_EXPORT_FUNCTION(mint_file_tell, 1)
MINT_EXPORT_FUNCTION(mint_file_seek, 2)
MINT_EXPORT_FUNCTION(mint_file_at_end, 1)
MINT_EXPORT_FUNCTION(mint_file_read, 2)
MINT_EXPORT_FUNCTION(mint_file_read_some, 3)
MINT_EXPORT_FUNCTION(mint_file_write, 2)
MINT_EXPORT_FUNCTION(mint_file_flush, 1)

MINT_EXPORT_FUNCTION(mint_file_open_async, 2)
MINT_EXPORT_FUNCTION(mint_file_close_async, 1)
MINT_EXPORT_FUNCTION(mint_file_tell_async, 1)
MINT_EXPORT_FUNCTION(mint_file_seek_async, 2)
MINT_EXPORT_FUNCTION(mint_file_at_end_async, 1)
MINT_EXPORT_FUNCTION(mint_file_read_async, 4)
MINT_EXPORT_FUNCTION(mint_file_read_some_async, 5)
MINT_EXPORT_FUNCTION(mint_file_write_async, 4)
MINT_EXPORT_FUNCTION(mint_file_flush_async, 3)
