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

#include <mint/memory/functiontool.h>
#include <mint/memory/casttool.h>
#include <mint/system/terminal.h>

#ifdef OS_WINDOWS
#include <Windows.h>
#include <io.h>
#else
#include <sys/poll.h>
#include <sys/file.h>
#include <unistd.h>
#include <poll.h>
#endif

using namespace mint;

MINT_FUNCTION(mint_pipe_create, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	HANDLE hPipe[2];
	SECURITY_ATTRIBUTES pipeAttributes;

	pipeAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	pipeAttributes.bInheritHandle = true;
	pipeAttributes.lpSecurityDescriptor = nullptr;

	if (CreatePipe(hPipe + 0, hPipe + 1, &pipeAttributes, 0) != 0) {
		if ((hPipe[0] != INVALID_HANDLE_VALUE) && (hPipe[1] != INVALID_HANDLE_VALUE)) {
			helper.return_value(create_iterator(create_handle(hPipe[0]), create_handle(hPipe[1])));
		}
	}
#else
	int fd[2];

	if (pipe2(fd, O_NONBLOCK) == 0) {
		if ((fd[0] != -1) && (fd[1] != -1)) {
			helper.return_value(create_iterator(create_handle(fd[0]), create_handle(fd[1])));
		}
	}
#endif
}

MINT_FUNCTION(mint_pipe_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	mint::handle_t handle = to_handle(helper.pop_parameter());

#ifdef OS_WINDOWS
	CloseHandle(handle);
#else
	close(handle);
#endif
}

MINT_FUNCTION(mint_pipe_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &stream = helper.pop_parameter();

#ifdef OS_WINDOWS
	DWORD count;
	uint8_t read_buffer[1024];
	mint::handle_t h = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *stream_buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	while (ReadFile(h, read_buffer, sizeof(read_buffer), &count, nullptr)) {

		if (count < 0) {
			break;
		}

		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}
#else
	uint8_t read_buffer[1024];
	mint::handle_t fd = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *stream_buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	while (ssize_t count = read(fd, read_buffer, sizeof(read_buffer))) {

		if (count < 0) {
			break;
		}

		std::copy_n(read_buffer, count, std::back_inserter(*stream_buffer));
	}
#endif
}

MINT_FUNCTION(mint_pipe_write, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &stream = helper.pop_parameter();

#ifdef OS_WINDOWS
	DWORD count;
	mint::handle_t h = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	WriteFile(h, buffer->data(), buffer->size(), &count, nullptr);
#else
	mint::handle_t fd = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	write(fd, buffer->data(), buffer->size());
#endif
}

MINT_FUNCTION(mint_pipe_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timeout = helper.pop_parameter();

#ifdef OS_WINDOWS
	mint::handle_t h = to_handle(helper.pop_parameter());
	DWORD dwMilliseconds = INFINITE;

	if (timeout.data()->format != Data::FMT_NONE) {
		dwMilliseconds = static_cast<DWORD>(to_integer(cursor, timeout));
	}

	DWORD ret = WaitForSingleObjectEx(h, dwMilliseconds, true);
	helper.return_value(create_boolean(ret == WAIT_OBJECT_0));
#else
	pollfd fds;
	fds.events = POLLIN;
	fds.fd = to_handle(helper.pop_parameter());

	int time_ms = -1;

	if (timeout.data()->format != Data::FMT_NONE) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		result = true;
	}

	helper.return_value(create_boolean(result));
#endif
}

MINT_FUNCTION(mint_system_pipe_create, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &fd_write = helper.pop_parameter();
	Reference &fd_read = helper.pop_parameter();
	WeakReference handles = create_iterator();

#ifdef OS_WINDOWS
	static const auto to_handle = [](int fd) {
		switch (fd) {
		case STDIN_FILE_NO:
			return GetStdHandle(STD_INPUT_HANDLE);
		case STDOUT_FILE_NO:
			return GetStdHandle(STD_OUTPUT_HANDLE);
		case STDERR_FILE_NO:
			return GetStdHandle(STD_ERROR_HANDLE);
		default:
			return reinterpret_cast<handle_t>(_get_osfhandle(fd));
		}
		return INVALID_HANDLE_VALUE;
	};

	if (handle_t handle = to_handle(to_integer(cursor, fd_read)); handle != INVALID_HANDLE_VALUE) {
		iterator_yield(handles.data<Iterator>(), create_handle(handle));
	}
	else {
		iterator_yield(handles.data<Iterator>(), WeakReference::create<None>());
	}
	if (handle_t handle = to_handle(to_integer(cursor, fd_write)); handle != INVALID_HANDLE_VALUE) {
		iterator_yield(handles.data<Iterator>(), create_handle(handle));
	}
	else {
		iterator_yield(handles.data<Iterator>(), WeakReference::create<None>());
	}
#else
	if (handle_t handle = to_number(cursor, fd_read); handle != -1) {
		iterator_yield(handles.data<Iterator>(), create_handle(handle));
	}
	else {
		iterator_yield(handles.data<Iterator>(), WeakReference::create<None>());
	}
	if (handle_t handle = to_number(cursor, fd_write); handle != -1) {
		iterator_yield(handles.data<Iterator>(), create_handle(handle));
	}
	else {
		iterator_yield(handles.data<Iterator>(), WeakReference::create<None>());
	}
#endif

	helper.return_value(std::move(handles));
}

MINT_FUNCTION(mint_system_pipe_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &stream = helper.pop_parameter();
	mint::handle_t handle = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *stream_buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

#ifdef OS_WINDOWS

	DWORD dwCount;
	while (PeekNamedPipe(handle, NULL, 0, NULL, &dwCount, NULL) && dwCount) {
		auto *read_buffer = new uint8_t[dwCount];
		if (ReadFile(handle, read_buffer, dwCount, &dwCount, nullptr)) {
			copy_n(read_buffer, dwCount, back_inserter(*stream_buffer));
		}
		delete[] read_buffer;
	}
#else
	pollfd rfds;
	rfds.fd = handle;
	rfds.events = POLLIN;

	const int flags = fcntl(rfds.fd, F_GETFL);
	fcntl(rfds.fd, F_SETFL, flags | O_NONBLOCK);

	while (::poll(&rfds, 1, 0) == 1) {
		uint8_t read_buffer[BUFSIZ];
		if (size_t count = ::read(rfds.fd, read_buffer, BUFSIZ)) {
			copy_n(read_buffer, count, back_inserter(*stream_buffer));
		}
	}

	fcntl(rfds.fd, F_SETFL, flags);
#endif
}

MINT_FUNCTION(mint_system_pipe_write, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &stream = helper.pop_parameter();
	mint::handle_t handle = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

#ifdef OS_WINDOWS
	DWORD dwCount;
	WriteFile(handle, buffer->data(), buffer->size(), &dwCount, nullptr);
#else
	write(handle, buffer->data(), buffer->size());
#endif
}

MINT_FUNCTION(mint_system_pipe_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timeout = helper.pop_parameter();

#ifdef OS_WINDOWS
	mint::handle_t h = to_handle(helper.pop_parameter());
	DWORD dwMilliseconds = INFINITE;

	if (timeout.data()->format != Data::FMT_NONE) {
		dwMilliseconds = static_cast<DWORD>(to_integer(cursor, timeout));
	}

	DWORD ret = WaitForSingleObjectEx(h, dwMilliseconds, true);
	helper.return_value(create_boolean(ret == WAIT_OBJECT_0));
#else
	pollfd fds;
	fds.events = POLLIN;
	fds.fd = to_handle(helper.pop_parameter());

	int time_ms = -1;

	if (timeout.data()->format != Data::FMT_NONE) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		result = true;
	}

	helper.return_value(create_boolean(result));
#endif
}
