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
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/async_io.h"
#include "mint/system/terminal.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <corecrt_io.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <processenv.h>
#include <synchapi.h>
#include <winbase.h>
#include <bit>
#else
#include <array>
#include <cstdio>
#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
#endif

namespace {

mint::Reference mint_stdio_endpoint_to_handle(mint::Cursor& cursor, const mint::Reference& descriptor) {

	if (!mint::is_instance_of(descriptor, mint::Data::Format::number)) {
		return descriptor;
	}

#ifdef MINT_OS_WINDOWS
	switch (const auto fd = mint::to_integer<int>(cursor, descriptor)) {
	case mint::stdin_file_no:
		if (auto* handle = GetStdHandle(STD_INPUT_HANDLE); handle != mint::invalid_handle) {
			return mint::create_handle(cursor.ast(), handle);
		}
		break;
	case mint::stdout_file_no:
		if (auto* handle = GetStdHandle(STD_OUTPUT_HANDLE); handle != mint::invalid_handle) {
			return mint::create_handle(cursor.ast(), handle);
		}
		break;
	case mint::stderr_file_no:
		if (auto* handle = GetStdHandle(STD_ERROR_HANDLE); handle != mint::invalid_handle) {
			return mint::create_handle(cursor.ast(), handle);
		}
		break;
	default:
		if (auto* handle = std::bit_cast<mint::handle_t>(_get_osfhandle(fd)); handle != mint::invalid_handle) {
			return mint::create_handle(cursor.ast(), handle);
		}
		break;
	}
#else
	if (auto handle = mint::to_integer<int>(cursor, descriptor); handle != mint::invalid_handle) {
		return mint::create_handle(cursor.ast(), handle);
	}
#endif

	return {};
}

mint::Reference mint_stdio_socket_read_some(mint::Cursor& cursor, const mint::Reference& handle,
    const mint::Reference& stream, const mint::Reference& count) {

	auto* buf = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

#ifdef MINT_OS_WINDOWS
	auto bytes_read = DWORD();
	auto read_buffer_length = mint::to_integer<std::size_t>(cursor, count);
	auto read_buffer = std::make_unique<std::uint8_t[]>(read_buffer_length);
	if (ReadFile(to_handle(handle), read_buffer.get(), static_cast<DWORD>(read_buffer_length), &bytes_read, nullptr)) {
		buf->append_range(std::span(read_buffer.get(), bytes_read));
	}
#else
	const auto fd = to_handle(handle);

	const int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	auto read_buffer_length = mint::to_integer<std::size_t>(cursor, count);
	auto read_buffer = std::make_unique<std::uint8_t[]>(read_buffer_length);
	if (auto bytes_read = ::read(fd, read_buffer.get(), read_buffer_length)) {
		buf->append_range(std::span(read_buffer.get(), bytes_read));
	}

	fcntl(fd, F_SETFL, flags);
#endif
	return {};
}

mint::Reference mint_stdio_socket_read(mint::Cursor& /*cursor*/, const mint::Reference& handle,
    const mint::Reference& stream) {

	auto* buf = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

#ifdef MINT_OS_WINDOWS

	auto bytes_read = DWORD();
	while (PeekNamedPipe(to_handle(handle), nullptr, 0, nullptr, &bytes_read, nullptr) && bytes_read) {
		auto read_buffer = std::make_unique<std::uint8_t[]>(bytes_read);
		if (ReadFile(to_handle(handle), read_buffer.get(), bytes_read, &bytes_read, nullptr)) {
			buf->append_range(std::span(read_buffer.get(), bytes_read));
		}
	}
#else
	auto read_fd_set = pollfd {
	    .fd = to_handle(handle),
	    .events = POLLIN,
	};

	const int flags = fcntl(read_fd_set.fd, F_GETFL);
	fcntl(read_fd_set.fd, F_SETFL, flags | O_NONBLOCK);

	while (::poll(&read_fd_set, 1, 0) == 1) {
		auto read_buffer = std::array<std::uint8_t, BUFSIZ>();
		if (auto bytes_read = ::read(read_fd_set.fd, read_buffer.data(), read_buffer.size())) {
			buf->append_range(std::span(read_buffer.data(), bytes_read));
		}
	}

	fcntl(read_fd_set.fd, F_SETFL, flags);
#endif
	return {};
}

mint::Reference mint_stdio_socket_write(mint::Cursor& /*cursor*/, const mint::Reference& handle,
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

}

MINT_EXPORT_FUNCTION(mint_stdio_endpoint_to_handle, 1)

MINT_EXPORT_FUNCTION(mint_stdio_socket_read_some, 3)
MINT_EXPORT_FUNCTION(mint_stdio_socket_read, 2)
MINT_EXPORT_FUNCTION(mint_stdio_socket_write, 2)
