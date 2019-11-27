#include "system/fileprinter.h"
#include "system/filesystem.h"

using namespace mint;

FilePrinter::FilePrinter(int fd) :
	m_closable(false) {
	switch (fd) {
	case 0:
		m_output = stdin;
		break;
	case 1:
		m_output = stdout;
		break;
	case 2:
		m_output = stderr;
		break;
	default:
		m_output = fdopen(fd, "w");
		m_closable = true;
		break;
	}
}

FilePrinter::FilePrinter(const char *path) :
	m_output(open_file(path, "w")),
	m_closable(true) {

}

FilePrinter::~FilePrinter() {
	if (m_closable) {
		fclose(m_output);
	}
	else {
		fflush(m_output);
	}
}

bool FilePrinter::print(DataType type, void *data) {

	switch (type) {
	case none:
		break;
	case null:
		fprintf(m_output, "(null)");
		break;
	case object:
		fprintf(m_output, "%p", data);
		break;
	case package:
		fprintf(m_output, "(package)");
		break;
	case function:
		fprintf(m_output, "(function)");
		break;

	default:
		return false;
	}

	return true;
}

void FilePrinter::print(const char *value) {
	fprintf(m_output, "%s", value);
}

void FilePrinter::print(double value) {
	fprintf(m_output, "%g", value);
}

void FilePrinter::print(bool value) {
	fprintf(m_output, "%s", value ? "true" : "false");
}

FILE *FilePrinter::file() const {
	return m_output;
}
