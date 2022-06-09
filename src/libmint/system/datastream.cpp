#include "system/datastream.h"

using namespace std;
using namespace mint;

DataStream::DataStream() :
	m_newLineCallback([](size_t){}),
	m_lineNumber(1),
	m_state(on_new_line) {

}

DataStream::~DataStream() {

}

int DataStream::getChar() {

	int c = readChar();

	switch (m_state) {
	case on_new_line:
		beginLine();
		break;
	case on_first_char:
		continueLine();
		break;
	case on_reading:
		break;
	}

	switch (c) {
	case EOF:
	case '\0':
		break;
	case '\n':
		endLine();
		break;
	default:
		m_cachedLine += static_cast<char>(c);
		break;
	}

	return c;
}

void DataStream::setNewLineCallback(function<void(size_t)> callback) {
	m_newLineCallback = callback;
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

	if (m_state != on_new_line) {
		endLine();
	}

	return line;
}


void DataStream::continueLine() {
	m_newLineCallback(m_lineNumber);
	m_state = on_reading;
}

void DataStream::beginLine() {
	m_state = on_first_char;
	m_cachedLine.clear();
}

void DataStream::endLine() {
	if (m_state == on_first_char) {
		continueLine();
	}
	m_state = on_new_line;
	m_lineNumber++;
}
