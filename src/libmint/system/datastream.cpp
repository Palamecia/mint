#include "system/datastream.h"

using namespace std;
using namespace mint;

DataStream::DataStream() :
	m_lineNumber(1),
	m_lineEndCallback([](size_t){}),
	m_newLine(true) {

}

DataStream::~DataStream() {

}

int DataStream::getChar() {

	int c = readChar();

	if (m_newLine && (c != '\0') && (c != EOF)) {
		startLine();
	}

	if ((c != '\n') && (c != EOF)) {
		m_cachedLine += static_cast<char>(c);
	}
	else if (c != EOF) {
		nextLine();
	}

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
	size_t err_pos = line.size() - 1;

	if (line.back() != '\n') {
		int c = nextBufferedChar();
		while ((c != '\n') && (c != '\0') && (c != EOF)) {
			line += static_cast<char>(c);
			c = nextBufferedChar();
		}
		line += '\n';
	}

	if (err_pos > 1) {
		for (size_t i = 0; i < err_pos - 1; ++i) {

			int c = m_cachedLine[i];

			if (c == '\t') {
				line += '\t';
			}
			else if (c & 0x80) {

				size_t size = 2;

				if (c & 0x04) {
					size++;
					if (c & 0x02) {
						size++;
					}
				}

				if (i + size < err_pos - 1) {
					line += ' ';
				}

				i += size - 1;
			}
			else {
				line += ' ';
			}
		}
	}
	line += '^';

	if (!m_newLine) {
		nextLine();
	}

	return line;
}

void DataStream::nextLine() {
	m_newLine = true;
	m_lineNumber++;
}

void DataStream::startLine() {
	m_lineEndCallback(m_lineNumber);
	m_cachedLine.clear();
	m_newLine = false;
}
