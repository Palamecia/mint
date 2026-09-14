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
#include "mint/memory/cast_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <memory>
#include <span>
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
#include <winnt.h>
#elifdef MINT_OS_UNIX
#ifdef MINT_OS_LINUX
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

mint::Reference mint_pipe_create(mint::Cursor& cursor) {
#ifdef MINT_OS_WINDOWS

	auto pipe = std::to_array<HANDLE>({INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE});
	SECURITY_ATTRIBUTES pipe_attributes {
	    .nLength = sizeof(SECURITY_ATTRIBUTES),
	    .lpSecurityDescriptor = nullptr,
	    .bInheritHandle = true,
	};

	if (CreatePipe(std::next(pipe.data(), 0), std::next(pipe.data(), 1), &pipe_attributes, 0) != 0) {
		if ((pipe.at(0) != INVALID_HANDLE_VALUE) && (pipe.at(1) != INVALID_HANDLE_VALUE)) {
			if (DWORD mode = PIPE_NOWAIT; SetNamedPipeHandleState(pipe.at(0), &mode, nullptr, nullptr)) {
				return mint::create_iterator_from(cursor, mint::create_handle(cursor.ast(), pipe.at(0)),
				    mint::create_handle(cursor.ast(), pipe.at(1)));
			}
			CloseHandle(pipe.at(0));
			CloseHandle(pipe.at(1));
		}
	}

#elifdef MINT_OS_MAC

	auto fd = std::to_array<int>({-1, -1});

	if (pipe(fd.data()) == 0) {
		if ((fd.at(0) != -1) && (fd.at(1) != -1)) {
			fcntl(fd.at(0), F_SETFL, fcntl(fd.at(0), F_GETFL) | O_NONBLOCK);
			fcntl(fd.at(1), F_SETFL, fcntl(fd.at(1), F_GETFL) | O_NONBLOCK);
			return mint::create_iterator_from(cursor, mint::create_handle(cursor.ast(), fd.at(0)),
			    mint::create_handle(cursor.ast(), fd.at(1)));
		}
	}

#elifdef MINT_OS_UNIX

	auto fd = std::to_array<int>({-1, -1});

	if (pipe2(fd.data(), O_NONBLOCK) == 0) {
		if ((fd.at(0) != -1) && (fd.at(1) != -1)) {
			return mint::create_iterator_from(cursor, mint::create_handle(cursor.ast(), fd.at(0)),
			    mint::create_handle(cursor.ast(), fd.at(1)));
		}
	}

#endif
	return {};
}

mint::Reference mint_pipe_close(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(mint::to_handle(handle));
#else
	close(mint::to_handle(handle));
#endif
	return {};
}

mint::Reference mint_pipe_read_some(mint::Cursor& cursor, const mint::Reference& handle, const mint::Reference& stream,
    const mint::Reference& count) {

	const auto bytes_count = mint::to_integer<std::size_t>(cursor, count);
	auto read_buffer = std::make_unique<std::uint8_t[]>(bytes_count);
	auto* stream_buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

#ifdef MINT_OS_WINDOWS
	const auto h = to_handle(handle);
	if (auto byte_received = DWORD();
	    ReadFile(h, read_buffer.get(), static_cast<DWORD>(bytes_count), &byte_received, nullptr)) {
		if (byte_received > 0) {
			stream_buffer->append_range(std::span(read_buffer.get(), byte_received));
		}
	}
#else
	const auto fd = to_handle(handle);
	if (const auto byte_received = read(fd, read_buffer.get(), bytes_count)) {
		if (byte_received > 0) {
			stream_buffer->append_range(std::span(read_buffer.get(), byte_received));
		}
	}
#endif
	return {};
}

mint::Reference mint_pipe_read(mint::Cursor& /*cursor*/, const mint::Reference& handle, const mint::Reference& stream) {

	auto read_buffer = std::array<std::uint8_t, BUFSIZ>();
	auto* stream_buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

#ifdef MINT_OS_WINDOWS
	auto byte_received = DWORD();
	const auto h = to_handle(handle);
	while (ReadFile(h, read_buffer.data(), static_cast<DWORD>(read_buffer.size()), &byte_received, nullptr)) {

		if (byte_received < 0) {
			break;
		}

		stream_buffer->append_range(std::span(read_buffer.data(), byte_received));
	}
#else
	const auto fd = to_handle(handle);
	while (auto byte_received = read(fd, read_buffer.data(), read_buffer.size())) {

		if (byte_received < 0) {
			break;
		}

		stream_buffer->append_range(std::span(read_buffer.data(), byte_received));
	}
#endif
	return {};
}

mint::Reference mint_pipe_write(mint::Cursor& /*cursor*/, const mint::Reference& handle, const mint::Reference& stream) {

#ifdef MINT_OS_WINDOWS
	DWORD count = 0;
	const auto h = to_handle(handle);
	auto* buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	WriteFile(h, buffer->data(), static_cast<DWORD>(buffer->size()), &count, nullptr);
#else
	const auto fd = to_handle(handle);
	auto* buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	write(fd, buffer->data(), buffer->size());
#endif
	return {};
}

mint::Reference mint_pipe_wait(mint::Cursor& cursor, const mint::Reference& handle, const mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS

	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	return mint::create_boolean(WaitForSingleObjectEx(mint::to_handle(handle), time_ms, true) == WAIT_OBJECT_0);

#elifdef MINT_OS_LINUX

	const int timeout_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	const int context = epoll_create1(EPOLL_CLOEXEC);
	if (context == -1) {
		return mint::create_boolean(false);
	}

	auto changelist = epoll_event {
	    .events = EPOLLIN,
	    .data =
	        {
	            .fd = mint::to_handle(handle),
	        },
	};

	const auto ret = epoll_wait(context, &changelist, 1, timeout_ms);

	::close(context);
	return mint::create_boolean(ret > 0);

#elifdef MINT_OS_UNIX

	const auto timeout_ts = is_instance_of(timeout, mint::Data::Format::none)
	                            ? std::nullopt
	                            : std::optional<std::chrono::milliseconds>(to_integer<int>(cursor, timeout))
	                                  .transform([](std::chrono::milliseconds ms) {
		                                  const auto sec = std::chrono::floor<std::chrono::seconds>(ms);
		                                  const auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
		                                      ms - sec);
		                                  return timespec {
		                                      .tv_sec = sec.count(),
		                                      .tv_nsec = nsec.count(),
		                                  };
	                                  });

	const int context = kqueue();
	if (context == -1) {
		return mint::create_boolean(false);
	}

	struct kevent changelist {
	    .ident = static_cast<std::uintptr_t>(mint::to_handle(handle)),
	    .filter = EVFILT_READ,
	};

	struct kevent eventlist {};
	const auto ret = kevent(context, &changelist, 1, &eventlist, 1, timeout_ts ? std::to_address(timeout_ts) : nullptr);

	::close(context);
	return mint::create_boolean(ret > 0);

#endif
}

}

MINT_EXPORT_FUNCTION(mint_pipe_create, 0)
MINT_EXPORT_FUNCTION(mint_pipe_close, 1)
MINT_EXPORT_FUNCTION(mint_pipe_read_some, 3)
MINT_EXPORT_FUNCTION(mint_pipe_read, 2)
MINT_EXPORT_FUNCTION(mint_pipe_write, 2)
MINT_EXPORT_FUNCTION(mint_pipe_wait, 2)
