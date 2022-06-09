#include "ast/fileprinter.h"
#include "memory/reference.h"
#include "memory/casttool.h"
#include "system/filesystem.h"
#include "system/terminal.h"

#ifdef OS_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace mint;

static int file_print(FILE *stream, const char *str) {
	return fputs(str, stream);
}

FilePrinter::FilePrinter(const char *path) :
	m_close(&fclose),
	m_stream(open_file(path, "w")) {
	if (is_term(m_stream)) {
		m_print = &term_print;
	}
	else {
		m_print = &file_print;
	}
}

FilePrinter::FilePrinter(int fd) {
	switch (fd) {
	case stdin_fileno:
		m_print = &file_print;
		m_close = &fflush;
		m_stream = stdin;
		break;
	case stdout_fileno:
		m_print = &term_print;
		m_close = &fflush;
		m_stream = stdout;
		break;
	case stderr_fileno:
		m_print = &term_print;
		m_close = &fflush;
		m_stream = stderr;
		break;
	default:
		m_print = &file_print;
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

int FilePrinter::internalPrint(const char *str) {
	return m_print(m_stream, str);
}

FILE *FilePrinter::file() const {
	return m_stream;
}
