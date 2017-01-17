#include "system/inputstream.h"
#include "system/terminal.h"

using namespace std;

InputStream::InputStream() :
	m_buffer(nullptr),
	m_cptr(nullptr),
	m_level(0),
	m_status(ready) {}

InputStream::~InputStream() {
	free(m_buffer);
}

InputStream &InputStream::instance() {

	static InputStream g_instance;

	return g_instance;
}

bool InputStream::atEnd() const {
	return m_status == over;
}

bool InputStream::isValid() const {
	return true;
}

string InputStream::path() const {
	return "stdin";
}

void InputStream::next() {
	fprintf(stdout, "\n");
	m_status = ready;
}

void InputStream::updateBuffer(const char *prompt) {

	free(m_buffer);
	m_buffer = readline(prompt);
	add_history(m_buffer);
	m_cptr = m_buffer;
}

int InputStream::readChar() {

	if (m_cptr == nullptr) {
		updateBuffer(">>> ");
	}
	else if ((m_status == ready) && (*m_cptr == '\0')) {
		updateBuffer(">>> ");
	}

	switch (m_status) {
	case ready:
		switch (*m_cptr) {
		case '\n':
			nextLine();
			if (m_level) {
				if (*(m_cptr + 1) == '\0') {
					updateBuffer("... ");
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

		return nextBufferedChar();

	case breaking:
		m_status = over;
		break;

	case over:
		m_status = ready;
		break;
	}

	return EOF;
}

int InputStream::nextBufferedChar() {
	return *m_cptr++;
}
