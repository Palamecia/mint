#include "system/printer.h"

Printer::Printer(int fd) : m_closable(false) {
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

Printer::Printer(const char *path) : m_closable(true) {
	m_output = fopen(path, "w");
}

Printer::~Printer() {
	if (m_closable) {
		fclose(m_output);
	}
}

void Printer::print(const void *value) {
	fprintf(m_output, "%p", value);
}

void Printer::print(double value) {
	fprintf(m_output, "%g", value);
}

void Printer::print(const char *value) {
	fprintf(m_output, "%s", value);
}

void Printer::printNone() {
	fprintf(m_output, "(none)");
}

void Printer::printNull() {
	fprintf(m_output, "(null)");
}

void Printer::printFunction() {
	fprintf(m_output, "(function)");
}
