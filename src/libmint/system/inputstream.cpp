#include "system/inputstream.h"
#include "system/terminal.h"

#include <cstring>

#define MINT_NEW_LINE_PROMPT "\033[1;32m>>>\033[0m "
#define MINT_CONTINUE_PROMPT "\033[1;32m...\033[0m "

using namespace std;
using namespace mint;

int amount_of_digits(size_t value) {

	int amount = 1;

	while (value /= 10) {
		amount++;
	}

	return amount;
}

InputStream::InputStream() :
	m_buffer(nullptr),
	m_cptr(nullptr),
	m_level(0),
	m_status(ready) {
	term_init();
}

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
	m_level = 0;
	m_status = ready;
}

void InputStream::updateBuffer(const char *prompt) {

	size_t line_number = lineNumber();
	int line_number_digits = (amount_of_digits(line_number) / 4) + 3;
	size_t full_prompt_length = line_number_digits + strlen(prompt) + 3;
	char full_prompt[full_prompt_length];

	snprintf(full_prompt, sizeof(full_prompt), "% *zd %s", line_number_digits, line_number, prompt);
	free(m_buffer);
	m_buffer = nullptr;

	term_add_history(m_cptr = m_buffer = term_read_line(full_prompt));
}

int InputStream::readChar() {

	if (m_cptr == nullptr) {
		updateBuffer(MINT_NEW_LINE_PROMPT);
	}
	else if ((m_status == ready) && (*m_cptr == '\0')) {
		updateBuffer(MINT_NEW_LINE_PROMPT);
	}

	switch (m_status) {
	case continuing:
		updateBuffer(MINT_CONTINUE_PROMPT);
		m_status = ready;

	case ready:
		switch (*m_cptr) {
		case '\n':
			if (m_level) {
				if (*(m_cptr + 1) == '\0') {
					m_status = continuing;
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
