#include "system/filestream.h"
#include "system/filesystem.h"

using namespace std;
using namespace mint;

FileStream::FileStream(const string &name) : m_over(false) {

	m_path = name;
	m_file = open_file(name.c_str(), "r");
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
