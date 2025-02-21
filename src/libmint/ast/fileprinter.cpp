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

#include "mint/ast/fileprinter.h"
#include "mint/memory/reference.h"
#include "mint/memory/casttool.h"
#include "mint/system/filesystem.h"
#include "mint/system/terminal.h"
#include "mint/system/pipe.h"

#ifdef OS_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace mint;

struct File {
	static int print(FILE *stream, const char *str) {
		return fputs(str, stream);
	}
};

FilePrinter::FilePrinter(const char *path) :
	m_close(&fclose),
	m_stream(open_file(path, "w")) {
	if (is_term(m_stream)) {
		m_print = &Terminal::print;
	}
	else {
		m_print = &File::print;
	}
}

FilePrinter::FilePrinter(int fd) {
	switch (fd) {
	case STDIN_FILE_NO:
		if (is_pipe(fd)) {
			m_print = &Pipe::print;
			m_close = &fflush;
		}
		else {
			m_print = &File::print;
			m_close = &fflush;
		}
		m_stream = stdin;
		break;
	case STDOUT_FILE_NO:
		if (is_term(fd)) {
			m_print = &Terminal::print;
			m_close = &fflush;
		}
		else if (is_pipe(fd)) {
			m_print = &Pipe::print;
			m_close = &fflush;
		}
		else {
			m_print = &File::print;
			m_close = &fflush;
		}
		m_stream = stdout;
		break;
	case STDERR_FILE_NO:
		if (is_term(fd)) {
			m_print = &Terminal::print;
			m_close = &fflush;
		}
		else if (is_pipe(fd)) {
			m_print = &Pipe::print;
			m_close = &fflush;
		}
		else {
			m_print = &File::print;
			m_close = &fflush;
		}
		m_stream = stderr;
		break;
	default:
		m_print = &File::print;
		m_close = &fclose;
		m_stream = fdopen(dup(fd), "a");
		break;
	}
}

FilePrinter::~FilePrinter() {
	m_close(m_stream);
}

void FilePrinter::print(Reference &reference) {
	std::string buffer = to_string(reference);
	m_print(m_stream, buffer.c_str());
}

int FilePrinter::internal_print(const char *str) {
	return m_print(m_stream, str);
}

FILE *FilePrinter::file() const {
	return m_stream;
}
