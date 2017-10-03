#include "system/filestream.h"

using namespace std;

FileStream::FileStream(const string &name) : m_over(false) {

	m_path = name;
	m_file = fopen(name.c_str(), "r");
}

FileStream::~FileStream() {

	if (m_file) {
		fclose(m_file);
	}
}

bool FileStream::atEnd() const {
	return m_over;
}

bool FileStream::isValid() const {
	return m_file != nullptr;
}

string FileStream::path() const {
	return m_path;
}

int FileStream::readChar() {

	int c = nextBufferedChar();

	switch (c) {
	case '\n':
		nextLine();
		break;
	case EOF:
		m_over = true;
		break;
	default:
		break;
	}

	return c;
}

int FileStream::nextBufferedChar() {
	return fgetc(m_file);
}
