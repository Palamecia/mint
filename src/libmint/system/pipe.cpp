/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "mint/system/terminal.h"
#include "mint/system/errno.h"

#ifdef OS_WINDOWS
#include "win32/pipe.h"
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <cstring>
#endif

using namespace mint;

int Pipe::printf(FILE *stream, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int written = Pipe::vprintf(stream, format, args);
	va_end(args);
	return written;
}

int Pipe::vprintf(FILE *stream, const char *format, va_list args) {

#ifdef OS_UNIX
	return vfprintf(stream, format, args);
#else
	HANDLE hPipe = INVALID_HANDLE_VALUE;

	switch (int fd = fileno(stream)) {
	case STDIN_FILE_NO:
		hPipe = GetStdHandle(STD_INPUT_HANDLE);
		break;
	case STDOUT_FILE_NO:
		hPipe = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case STDERR_FILE_NO:
		hPipe = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		hPipe = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
		break;
	}

	int written_all = 0;

	while (const char *cptr = strstr(format, "%")) {

		if (int prefix_length = static_cast<int>(cptr - format)) {
			int written = WriteMultiByteToFile(hPipe, format, prefix_length);
			if (written == EOF) {
				errno = errno_from_error_code(last_error_code());
				return written;
			}
			written_all += written;
		}

		format = cptr + 1;
		int written = pipe_handle_format_flags(hPipe, &format, &args);
		if (written == EOF) {
			errno = errno_from_error_code(last_error_code());
			return written;
		}
		written_all += written;
	}

	if (*format) {
		int written = WriteMultiByteToFile(hPipe, format);
		if (written == EOF) {
			errno = errno_from_error_code(last_error_code());
			return written;
		}
		written_all += written;
	}

	return written_all;
#endif
}

int Pipe::print(FILE *stream, const char *str) {
	const int fd = fileno(stream);
#ifdef OS_WINDOWS
	DWORD dwNumberOfBytesWritten = 0;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	switch (fd) {
	case STDIN_FILE_NO:
		hPipe = GetStdHandle(STD_INPUT_HANDLE);
		break;
	case STDOUT_FILE_NO:
		hPipe = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case STDERR_FILE_NO:
		hPipe = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		hPipe = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
		break;
	}
	if (WriteFile(hPipe, str, static_cast<DWORD>(strlen(str)) + 1, &dwNumberOfBytesWritten, NULL)) {
		return static_cast<int>(dwNumberOfBytesWritten);
	}
	return EOF;
#else
	return write(fd, str, strlen(str));
#endif
}

bool mint::is_pipe(FILE *stream) {
	return is_pipe(fileno(stream));
}

bool mint::is_pipe(int fd) {
#ifdef OS_WINDOWS
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	switch (fd) {
	case STDIN_FILE_NO:
		hPipe = GetStdHandle(STD_INPUT_HANDLE);
		break;
	case STDOUT_FILE_NO:
		hPipe = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case STDERR_FILE_NO:
		hPipe = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		hPipe = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
		break;
	}
	return GetNamedPipeInfo(hPipe, NULL, NULL, NULL, NULL);
#else
	return S_ISFIFO(fd);
#endif
}
