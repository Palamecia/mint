#include "filestream.h"

using namespace std;

FileStream::FileStream(const string &name) : m_lineNumber(1), m_over(false) {

	m_path = name;
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

	addToCache(c);
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

string FileStream::path() const {
	return m_path;
}

string FileStream::uncachedLine() {

	string line;
	char c = fgetc(m_file);

	while (c != '\n') {
		line += c;
		c = fgetc(m_file);
	}

	return line;
}
