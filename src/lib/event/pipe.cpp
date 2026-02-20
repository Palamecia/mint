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

#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/casttool.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/system/terminal.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <memory>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <corecrt_io.h>
#include <fileapi.h>
#include <handleapi.h>
#include <io.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <processenv.h>
#include <rpcdce.h>
#include <synchapi.h>
#include <winbase.h>
#else
#include <sys/poll.h>
#include <sys/file.h>
#include <unistd.h>
#include <poll.h>
#endif

namespace {

mint::WeakReference mint_pipe_create(mint::Cursor& cursor) {
#ifdef MINT_OS_WINDOWS
	std::array<HANDLE, 2> pipe {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
	SECURITY_ATTRIBUTES pipe_attributes {
	    .nLength = sizeof(SECURITY_ATTRIBUTES),
	    .lpSecurityDescriptor = nullptr,
	    .bInheritHandle = true,
	};

	if (CreatePipe(std::next(pipe.data(), 0), std::next(pipe.data(), 1), &pipe_attributes, 0) != 0) {
		if ((pipe[0] != INVALID_HANDLE_VALUE) && (pipe[1] != INVALID_HANDLE_VALUE)) {
			return mint::create_iterator_from(cursor, mint::create_handle(cursor.ast(), pipe[0]),
			    mint::create_handle(cursor.ast(), pipe[1]));
		}
	}
#else
	std::array<int, 2> fd {-1, -1};

	if (pipe2(fd.data(), O_NONBLOCK) == 0) {
		if ((fd[0] != -1) && (fd[1] != -1)) {
			return mint::create_iterator_from(cursor, mint::create_handle(cursor.ast(), fd[0]),
			    mint::create_handle(cursor.ast(), fd[1]));
		}
	}
#endif
	return {};
}

mint::WeakReference mint_pipe_close(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(mint::to_handle(handle));
#else
	close(mint::to_handle(handle));
#endif
	return {};
}

mint::WeakReference mint_pipe_read(mint::Cursor& /*cursor*/, const mint::Reference& handle,
    const mint::Reference& stream) {
#ifdef MINT_OS_WINDOWS
	DWORD count = 0;
	std::array<std::uint8_t, BUFSIZ> read_buffer = {};
	mint::handle_t h = to_handle(handle);
	auto* stream_buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	while (ReadFile(h, read_buffer.data(), static_cast<DWORD>(read_buffer.size()), &count, nullptr)) {

		if (count < 0) {
			break;
		}

		std::copy_n(read_buffer.data(), count, std::back_inserter(*stream_buffer));
	}
#else
	std::array<std::uint8_t, BUFSIZ> read_buffer = {};
	mint::handle_t fd = to_handle(handle);
	auto* stream_buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	while (ssize_t count = read(fd, read_buffer.data(), read_buffer.size())) {

		if (count < 0) {
			break;
		}

		std::copy_n(read_buffer.data(), count, std::back_inserter(*stream_buffer));
	}
#endif
	return {};
}

mint::WeakReference mint_pipe_write(mint::Cursor& /*cursor*/, const mint::Reference& handle,
    const mint::Reference& stream) {

#ifdef MINT_OS_WINDOWS
	DWORD count = 0;
	mint::handle_t h = to_handle(handle);
	auto* buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	WriteFile(h, buffer->data(), static_cast<DWORD>(buffer->size()), &count, nullptr);
#else
	mint::handle_t fd = to_handle(handle);
	auto* buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	write(fd, buffer->data(), buffer->size());
#endif
	return {};
}

mint::WeakReference mint_pipe_wait(mint::Cursor& cursor, const mint::Reference& handle, const mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS
	mint::handle_t h = to_handle(handle);
	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	return mint::create_boolean(WaitForSingleObjectEx(h, time_ms, true) == WAIT_OBJECT_0);
#else
	pollfd fds {
	    .fd = to_handle(handle),
	    .events = POLLIN,
	};

	const int time_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	if (int ret = poll(&fds, 1, time_ms); (ret > 0) && (fds.revents & POLLIN)) {
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);
#endif
}

mint::WeakReference mint_system_pipe_create(mint::Cursor& cursor, const mint::Reference& fd_read,
    const mint::Reference& fd_write) {

	mint::WeakReference handles = mint::create_iterator(cursor.ast());

#ifdef MINT_OS_WINDOWS
	static const auto to_handle = [](int fd) {
		switch (fd) {
		case mint::stdin_file_no:
			return GetStdHandle(STD_INPUT_HANDLE);
		case mint::stdout_file_no:
			return GetStdHandle(STD_OUTPUT_HANDLE);
		case mint::stderr_file_no:
			return GetStdHandle(STD_ERROR_HANDLE);
		default:
			return std::bit_cast<handle_t>(_get_osfhandle(fd));
		}
		return INVALID_HANDLE_VALUE;
	};

	if (handle_t handle = to_handle(mint::to_integer<int>(cursor, fd_read)); handle != INVALID_HANDLE_VALUE) {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_handle(cursor.ast(), handle));
	}
	else {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_none());
	}
	if (handle_t handle = to_handle(mint::to_integer<int>(cursor, fd_write)); handle != INVALID_HANDLE_VALUE) {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_handle(cursor.ast(), handle));
	}
	else {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_none());
	}
#else
	if (auto handle = to_integer<mint::handle_t>(cursor, fd_read); handle != -1) {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_handle(cursor.ast(), handle));
	}
	else {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_none());
	}
	if (auto handle = to_integer<mint::handle_t>(cursor, fd_write); handle != -1) {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_handle(cursor.ast(), handle));
	}
	else {
		iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_none());
	}
#endif

	return handles;
}

mint::WeakReference mint_system_pipe_read(mint::Cursor& /*cursor*/, const mint::Reference& handle,
    const mint::Reference& stream) {

	std::vector<std::uint8_t>* stream_buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

#ifdef MINT_OS_WINDOWS

	DWORD count = 0;
	while (PeekNamedPipe(to_handle(handle), nullptr, 0, nullptr, &count, nullptr) && count) {
		auto read_buffer = std::make_unique<std::uint8_t[]>(count);
		if (ReadFile(to_handle(handle), read_buffer.get(), count, &count, nullptr)) {
			std::copy_n(read_buffer.get(), count, back_inserter(*stream_buffer));
		}
	}
#else
	pollfd rfds {
	    .fd = to_handle(handle),
	    .events = POLLIN,
	};

	const int flags = fcntl(rfds.fd, F_GETFL);
	fcntl(rfds.fd, F_SETFL, flags | O_NONBLOCK);

	while (::poll(&rfds, 1, 0) == 1) {
		std::array<std::uint8_t, BUFSIZ> read_buffer = {};
		if (auto count = ::read(rfds.fd, read_buffer.data(), read_buffer.size())) {
			std::copy_n(read_buffer.data(), count, back_inserter(*stream_buffer));
		}
	}

	fcntl(rfds.fd, F_SETFL, flags);
#endif
	return {};
}

mint::WeakReference mint_system_pipe_write(mint::Cursor& /*cursor*/, const mint::Reference& handle,
    const mint::Reference& stream) {

	std::vector<std::uint8_t>* buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

#ifdef MINT_OS_WINDOWS
	DWORD count = 0;
	WriteFile(to_handle(handle), buffer->data(), static_cast<DWORD>(buffer->size()), &count, nullptr);
#else
	write(to_handle(handle), buffer->data(), buffer->size());
#endif
	return {};
}

mint::WeakReference mint_system_pipe_wait(mint::Cursor& cursor, const mint::Reference& handle,
    mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS
	mint::handle_t h = to_handle(handle);
	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	return mint::create_boolean(WaitForSingleObjectEx(h, time_ms, true) == WAIT_OBJECT_0);
#else
	pollfd fds {
	    .fd = to_handle(handle),
	    .events = POLLIN,
	};

	const int time_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	if (int ret = poll(&fds, 1, time_ms); (ret > 0) && (fds.revents & POLLIN)) {
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);
#endif
}

}

MINT_EXPORT_FUNCTION(mint_pipe_create, 0)
MINT_EXPORT_FUNCTION(mint_pipe_close, 1)
MINT_EXPORT_FUNCTION(mint_pipe_read, 2)
MINT_EXPORT_FUNCTION(mint_pipe_write, 2)
MINT_EXPORT_FUNCTION(mint_pipe_wait, 2)
MINT_EXPORT_FUNCTION(mint_system_pipe_create, 2)
MINT_EXPORT_FUNCTION(mint_system_pipe_read, 2)
MINT_EXPORT_FUNCTION(mint_system_pipe_write, 2)
MINT_EXPORT_FUNCTION(mint_system_pipe_wait, 2)
