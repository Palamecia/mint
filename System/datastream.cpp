#include "datastream.h"

using namespace std;

DataStream::DataStream() : m_shouldClearCache(false) {}

DataStream::~DataStream() {}

string DataStream::lineError() {

	string line = m_cachedLine;
	size_t err_pos = line.size();

	/// \todo dosen't work with inputstream and '{' token
	char c = getChar();
	while (c != '\n') {
		line += c;
		c = getChar();
	}
	line += '\n';

	line += string(err_pos - 1, ' ');
	line += '^';

	return line;
}

void DataStream::addToCache(char c) {

	if (m_shouldClearCache) {
		m_cachedLine.clear();
		m_shouldClearCache = false;
	}

	m_cachedLine += c;
}

void DataStream::clearCache() {
	m_shouldClearCache = true;
}
