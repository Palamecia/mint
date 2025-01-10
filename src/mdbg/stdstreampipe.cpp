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

#include "stdstreampipe.h"
#include "assert.h"

using namespace mint;

#ifdef OS_UNIX
#include <poll.h>
#include <unistd.h>
#endif

StdStreamPipe::StdStreamPipe(StdStreamFileNo number) {
#ifdef OS_WINDOWS
	const std::wstring pipe_name = L"\\\\.\\pipe\\mdbg-std-" + std::to_wstring(number);

	m_handles[READ_INDEX] = INVALID_HANDLE_VALUE;
	m_handles[WRITE_INDEX] = INVALID_HANDLE_VALUE;

	HANDLE hRead = CreateNamedPipeW(pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
									1, BUFSIZ, BUFSIZ, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	HANDLE hWrite = CreateFile(pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (hRead != INVALID_HANDLE_VALUE && hWrite != INVALID_HANDLE_VALUE) {
		BOOL bConnected = ConnectNamedPipe(hRead, NULL);
		DWORD dwError = GetLastError();
		if (bConnected || dwError == ERROR_IO_PENDING || dwError == ERROR_PIPE_CONNECTED) {
			switch (number) {
			case STDIN_FILE_NO:
				if (SetStdHandle(STD_INPUT_HANDLE, hWrite)) {
					m_handles[READ_INDEX] = hRead;
					m_handles[WRITE_INDEX] = hWrite;
				}
				break;
			case STDOUT_FILE_NO:
				if (SetStdHandle(STD_OUTPUT_HANDLE, hWrite)) {
					m_handles[READ_INDEX] = hRead;
					m_handles[WRITE_INDEX] = hWrite;
				}
				break;
			case STDERR_FILE_NO:
				if (SetStdHandle(STD_ERROR_HANDLE, hWrite)) {
					m_handles[READ_INDEX] = hRead;
					m_handles[WRITE_INDEX] = hWrite;
				}
				break;
			}
		}
	}
#else
	if (pipe(m_handles)) {
		dup2(number, m_handles[WRITE_INDEX]);
	}
#endif
}

StdStreamPipe::~StdStreamPipe() {
#ifdef OS_WINDOWS
	CloseHandle(m_handles[WRITE_INDEX]);
	CloseHandle(m_handles[READ_INDEX]);
#else
	close(m_handles[WRITE_INDEX]);
	close(m_handles[READ_INDEX]);
#endif
}

bool StdStreamPipe::can_read() const {
#ifdef OS_WINDOWS
	DWORD dwCount = 0;

	if (PeekNamedPipe(m_handles[READ_INDEX], NULL, 0, NULL, &dwCount, NULL)) {
		return dwCount > 0;
	}
	return false;
#else
	pollfd rfds;
	rfds.fd = m_handles[READ_INDEX];
	rfds.events = POLLIN;

	return ::poll(&rfds, 1, 0) == 1;
#endif
}

std::string StdStreamPipe::read() {
	char buf[BUFSIZ];
#ifdef OS_WINDOWS
	DWORD dwCount = 0;

	if (ReadFile(m_handles[READ_INDEX], buf, BUFSIZ, &dwCount, NULL)) {
		return buf;
	}
#else
	if (::read(m_handles[READ_INDEX], buf, BUFSIZ)) {
		return buf;
	}
#endif

	return {};
}
