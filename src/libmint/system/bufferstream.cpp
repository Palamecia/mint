#include "system/bufferstream.h"

#include <cstring>

using namespace std;
using namespace mint;

BufferStream::BufferStream(const string &buffer) :
	m_buffer(strdup(buffer.c_str())),
	m_status(ready) {
	m_cptr = m_buffer;
}

BufferStream::~BufferStream() {
	delete m_buffer;
}

bool BufferStream::atEnd() const {
	return m_status == over;
}

bool BufferStream::isValid() const {
	return true;
}

string BufferStream::path() const {
	return "buffer";
}

int BufferStream::readChar() {

	switch (m_status) {
	case ready:
		if (*m_cptr == '\0') {
			m_status = over;
			return '\n';
		}
		break;

	case over:
		return EOF;
	}


	return nextBufferedChar();
}

int BufferStream::nextBufferedChar() {
	return *m_cptr++;
}
