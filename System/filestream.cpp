#include "filestream.h"

using namespace std;

FileStream::FileStream(const string &name) : m_lineNumber(1), m_over(false) {

	m_file = fopen(name.c_str(), "r");
}

FileStream::~FileStream() {
	fclose(m_file);
}

int FileStream::getChar() {

	int c = fgetc(m_file);

	switch (c) {
	case '\n':
		m_lineNumber++;
		break;
	case EOF:
		m_over = true;
		break;
	default:
		break;
	}

	return c;
}

bool FileStream::atEnd() const {
	return m_over;
}

bool FileStream::isValid() const {
	return m_file != nullptr;
}

size_t FileStream::lineNumber() const {
	return m_lineNumber;
}
