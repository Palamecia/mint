/// \file

#include "debug/lineinfo.h"

using namespace std;
using namespace mint;

LineInfo::LineInfo(const string &module, size_t lineNumber) :
	m_moduleName(module),
	m_lineNumber(lineNumber) {

}

string LineInfo::moduleName() const {
	return m_moduleName;
}

size_t LineInfo::lineNumber() const {
	return m_lineNumber;
}

string LineInfo::toString() const {

	if (m_lineNumber) {
		return "Module '" + m_moduleName + "', line " + to_string(m_lineNumber);
	}

	return "Module '" + m_moduleName + "', line unknown";
}
