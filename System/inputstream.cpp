#include "System/inputstream.h"

#include <cstdio>

using namespace std;

// #ifdef _WIN32
char *readline(const char *prompt) {

	size_t size = 0;
	char *buffer = nullptr;

	fprintf(stdout, prompt);
	fflush(stdout);

	getline(&buffer, &size, stdin);

	return buffer;
}
/* #else
#include <readline/readline.h>
#include <readline/history.h>
#endif*/

InputStream::InputStream() :
	m_buffer(nullptr),
	m_cptr(nullptr),
	m_level(0),
	m_status(ready),
	m_lineNumber(1) {}

InputStream::~InputStream() {}

InputStream &InputStream::instance() {

	static InputStream g_instance;

	return g_instance;
}

int InputStream::getChar() {

	if (m_cptr == nullptr) {
		m_buffer = readline(">>> ");
		// add_history(m_buffer);
		m_cptr = m_buffer;
	}
	else if ((m_status == ready) && (*m_cptr == 0)) {
		delete [] m_buffer;
		m_buffer = readline(">>> ");
		// add_history(m_buffer);
		m_cptr = m_buffer;
	}

	switch (m_status) {
	case ready:
		switch (*m_cptr) {
		case '\n':
			m_lineNumber++;
			if (m_level) {
				if (*(m_cptr + 1) == 0) {
					delete [] m_buffer;
					m_buffer = readline("... ");
					// add_history(m_buffer);
					m_cptr = m_buffer;
					addToCache('\n');
					return '\n';
				}
			}
			else {
				m_status = breaking;
			}
			break;
		case '{':
			m_level++;
			break;
		case '}':
			m_level--;
			break;
		default:
			break;
		}

		addToCache(*m_cptr);
		return *m_cptr++;

	case breaking:
		m_status = over;
		break;

	case over:
		m_status = ready;
		break;
	}

	return EOF;
}

bool InputStream::atEnd() const {
	return m_status == over;
}

bool InputStream::isValid() const {
	return true;
}

size_t InputStream::lineNumber() const {
	return m_lineNumber;
}

string InputStream::path() const {
	return "stdin";
}

void InputStream::next() {
	fprintf(stdout, "\n");
	m_status = ready;
}

string InputStream::uncachedLine() {

	string line;
	char c = *m_cptr++;

	while (c != '\n') {
		line += c;
		c = *m_cptr++;
	}

	return line;
}
