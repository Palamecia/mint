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

#include "mint/system/pipe.h"
#include "mint/system/errno.h"
#include <cstddef>
#include <cstdio>
#include <stdio.h>
#include <string>
#include <system_error>

#ifdef MINT_OS_WINDOWS
#include "mint/system/terminal.h"
#include <bit>
#include <Windows.h>
#include <corecrt_io.h>
#include <fileapi.h>
#include <handleapi.h>
#include <io.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <processenv.h>
#include <winbase.h>
#include <winnt.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#endif

using namespace mint;

std::size_t Pipe::write(FILE* stream, const std::string& str) {
	const int fd = fileno(stream);
#ifdef MINT_OS_WINDOWS
	DWORD number_of_bytes_written = 0;
	HANDLE pipe = INVALID_HANDLE_VALUE;
	switch (fd) {
	case stdin_file_no:
		pipe = GetStdHandle(STD_INPUT_HANDLE);
		break;
	case stdout_file_no:
		pipe = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case stderr_file_no:
		pipe = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		pipe = std::bit_cast<HANDLE>(_get_osfhandle(fd));
		break;
	}
	if (!WriteFile(pipe, str.data(), static_cast<DWORD>(str.length()), &number_of_bytes_written, nullptr)) {
		throw std::system_error(last_error_code());
	}
	return number_of_bytes_written;
#else
	const auto amount = ::write(fd, str.data(), str.length());
	if (amount == EOF) {
		throw std::system_error(last_error_code());
	}
	return static_cast<std::size_t>(amount);
#endif
}

void Pipe::print(FILE* stream, const std::string& str) {
	const int fd = fileno(stream);
#ifdef MINT_OS_WINDOWS
	DWORD number_of_bytes_written = 0;
	HANDLE pipe = INVALID_HANDLE_VALUE;
	switch (fd) {
	case stdin_file_no:
		pipe = GetStdHandle(STD_INPUT_HANDLE);
		break;
	case stdout_file_no:
		pipe = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case stderr_file_no:
		pipe = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		pipe = std::bit_cast<HANDLE>(_get_osfhandle(fd));
		break;
	}
	if (!WriteFile(pipe, str.data(), static_cast<DWORD>(str.length()), &number_of_bytes_written, nullptr)) {
		throw std::system_error(last_error_code());
	}
#else
	if (::write(fd, str.data(), str.length()) == EOF) {
		throw std::system_error(last_error_code());
	}
#endif
}

bool mint::is_pipe(FILE* stream) {
	return is_pipe(fileno(stream));
}

bool mint::is_pipe(int fd) {
#ifdef MINT_OS_WINDOWS
	HANDLE pipe = INVALID_HANDLE_VALUE;
	switch (fd) {
	case stdin_file_no:
		pipe = GetStdHandle(STD_INPUT_HANDLE);
		break;
	case stdout_file_no:
		pipe = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case stderr_file_no:
		pipe = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		pipe = std::bit_cast<HANDLE>(_get_osfhandle(fd));
		break;
	}
	return GetNamedPipeInfo(pipe, nullptr, nullptr, nullptr, nullptr);
#else
	return S_ISFIFO(fd);
#endif
}
