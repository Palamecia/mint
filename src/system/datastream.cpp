#include "system/datastream.h"

using namespace std;

DataStream::DataStream() : m_lineNumber(1), m_newLine(true) {}

DataStream::~DataStream() {}

int DataStream::getChar() {

	if (m_newLine) {
		m_lineEndCallback(m_lineNumber);
		m_cachedLine.clear();
		m_newLine = false;
	}

	int c = readChar();
	m_cachedLine += c;
	return c;
}

void DataStream::setLineEndCallback(function<void(size_t)> callback) {
	m_lineEndCallback = callback;
}

size_t DataStream::lineNumber() const {
	return m_lineNumber;
}

string DataStream::lineError() {

	string line = m_cachedLine;
	size_t err_pos = line.size();

	if (line.back() != '\n') {
		int c = nextBufferedChar();
		while ((c != '\n') && (c != '\0') && (c != EOF)) {
			line += c;
			c = nextBufferedChar();
		}
		line += '\n';
	}

	if (err_pos > 2) {
		for (size_t i = 0; i < err_pos - 2; ++i) {

			int c = m_cachedLine[i];

			if (c == '\t') {
				line += '\t';
			}
			else if (c & 0x80) {
				/// \todo handle utf-8 char (1 char != 1 byte)
			}
			else {
				line += ' ';
			}
		}
	}
	line += '^';

	return line;
}

void DataStream::nextLine() {
	m_newLine = true;
	m_lineNumber++;
}
