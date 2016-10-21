#include "system/datastream.h"

using namespace std;

DataStream::DataStream() : m_shouldClearCache(false) {}

DataStream::~DataStream() {}

int DataStream::getChar() {

	int c = getRawChar();

	if (m_shouldClearCache) {
		m_cachedLine.clear();
		m_shouldClearCache = false;
	}

	m_cachedLine += c;
	if (c == '\n') {
		m_shouldClearCache = true;
	}

	return c;
}

string DataStream::lineError() {

	string line = m_cachedLine;
	size_t err_pos = line.size();

	if (line.back() != '\n') {
		line += getLine();
		if (line.back() != '\n') {
			line += '\n';
		}
	}

	if (err_pos > 2) {
		for (size_t i = 0; i < err_pos - 2; ++i) {

			if (m_cachedLine[i] == '\t') {
				line += '\t';
			}
			else {
				line += ' ';
			}
		}
	}
	line += '^';

	return line;
}
