#include "datastream.h"

using namespace std;

DataStream::DataStream() : m_shouldClearCache(false) {}

DataStream::~DataStream() {}

const char *DataStream::cachedLine() const {
	return m_cachedLine.c_str();
}

string DataStream::uncachedLine() {

	string line;

	char c = getChar();
	while (c != '\n') {
		line += c;
		c = getChar();
	}

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
