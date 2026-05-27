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

#include "std_stream_pipe.h"
#include "mint/system/terminal.h"
#include <cassert>
#include <array>
#include <cstdio>
#include <string>

#ifdef MINT_OS_WINDOWS
#include <windows.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <processenv.h>
#include <winbase.h>
#include <winerror.h>
#include <winnt.h>
#else
#include <poll.h>
#include <sys/poll.h>
#include <unistd.h>
#endif

StdStreamPipe::StdStreamPipe(mint::StdStreamFileNo number) :
#ifdef MINT_OS_WINDOWS
    _handles({INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE}) {

	const std::wstring pipe_name = L"\\\\.\\pipe\\mdbg-std-" + std::to_wstring(number);

	HANDLE read_handle = CreateNamedPipeW(pipe_name.c_str(), PIPE_ACCESS_DUPLEX,
	    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, BUFSIZ, BUFSIZ, NMPWAIT_USE_DEFAULT_WAIT, nullptr);
	HANDLE write_handle = CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0,
	    nullptr);

	if (read_handle != INVALID_HANDLE_VALUE && write_handle != INVALID_HANDLE_VALUE) {
		const BOOL connected = ConnectNamedPipe(read_handle, nullptr);
		const DWORD error = GetLastError();
		if (connected || error == ERROR_IO_PENDING || error == ERROR_PIPE_CONNECTED) {
			switch (number) {
			case mint::stdin_file_no:
				if (SetStdHandle(STD_INPUT_HANDLE, write_handle)) {
					_handles[read_index] = read_handle;
					_handles[write_index] = write_handle;
				}
				break;
			case mint::stdout_file_no:
				if (SetStdHandle(STD_OUTPUT_HANDLE, write_handle)) {
					_handles[read_index] = read_handle;
					_handles[write_index] = write_handle;
				}
				break;
			case mint::stderr_file_no:
				if (SetStdHandle(STD_ERROR_HANDLE, write_handle)) {
					_handles[read_index] = read_handle;
					_handles[write_index] = write_handle;
				}
				break;
			}
		}
	}
#else
    _handles({-1, -1}) {
	if (pipe(_handles.data())) {
		dup2(number, _handles[write_index]);
	}
#endif
}

StdStreamPipe::~StdStreamPipe() {
#ifdef MINT_OS_WINDOWS
	CloseHandle(_handles[write_index]);
	CloseHandle(_handles[read_index]);
#else
	close(_handles[write_index]);
	close(_handles[read_index]);
#endif
}

bool StdStreamPipe::can_read() const {
#ifdef MINT_OS_WINDOWS
	DWORD count = 0;

	if (PeekNamedPipe(_handles[read_index], nullptr, 0, nullptr, &count, nullptr)) {
		return count > 0;
	}
	return false;
#else
	pollfd rfds {
	    .fd = _handles[read_index],
	    .events = POLLIN,
	};

	return ::poll(&rfds, 1, 0) == 1;
#endif
}

std::string StdStreamPipe::read() {

	auto buf = std::array<char, BUFSIZ>();

#ifdef MINT_OS_WINDOWS
	if (DWORD count = 0; ReadFile(_handles[read_index], buf.data(), static_cast<DWORD>(buf.size()), &count, nullptr)) {
		return buf.data();
	}
#else
	if (::read(_handles[read_index], buf.data(), buf.size())) {
		return buf.data();
	}
#endif

	return {};
}
