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

#include "mint/ast/file_printer.h"
#include "mint/memory/reference.h"
#include "mint/memory/cast_tools.h"
#include "mint/system/errno.h"
#include "mint/system/filesystem.h"
#include "mint/system/terminal.h"
#include "mint/system/pipe.h"
#include <cstdio>
#include <filesystem>
#include <stdio.h>
#include <string>
#include <system_error>

#ifdef MINT_OS_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace mint;

struct File {
	static void print(FILE* stream, const std::string& str) {
		if (std::fputs(str.data(), stream) == EOF) {
			throw std::system_error(last_error_code());
		}
	}
};

FilePrinter::FilePrinter(const std::filesystem::path& path) :
    _stream(open_file(path, "w")),
    _close(&fclose) {
	if (is_term(_stream)) {
		_print = &Terminal::print;
	}
	else if (is_pipe(_stream)) {
		_print = &Pipe::print;
	}
	else {
		_print = &File::print;
	}
}

FilePrinter::FilePrinter(int fd) {
	switch (fd) {
	case stdin_file_no:
		_stream = stdin;
		if (is_pipe(fd)) {
			_print = &Pipe::print;
			_close = &fflush;
		}
		else {
			_print = &File::print;
			_close = &fflush;
		}
		break;
	case stdout_file_no:
		_stream = stdout;
		if (is_term(fd)) {
			_print = &Terminal::print;
			_close = &fflush;
		}
		else if (is_pipe(fd)) {
			_print = &Pipe::print;
			_close = &fflush;
		}
		else {
			_print = &File::print;
			_close = &fflush;
		}
		break;
	case stderr_file_no:
		_stream = stderr;
		if (is_term(fd)) {
			_print = &Terminal::print;
			_close = &fflush;
		}
		else if (is_pipe(fd)) {
			_print = &Pipe::print;
			_close = &fflush;
		}
		else {
			_print = &File::print;
			_close = &fflush;
		}
		break;
	default:
		_stream = fdopen(dup(fd), "a");
		_print = &File::print;
		_close = &fclose;
		break;
	}
}

FilePrinter::~FilePrinter() {
	_close(_stream);
}

void FilePrinter::print(const Reference& reference) {
	_print(_stream, to_string(reference));
}

FILE* FilePrinter::stream() const {
	return _stream;
}
