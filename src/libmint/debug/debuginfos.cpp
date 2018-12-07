#include "debug/debuginfos.h"
#include "ast/module.h"

using namespace std;
using namespace mint;

void DebugInfos::newLine(Module *module, size_t lineNumber) {
	m_lines.emplace(module->nextNodeOffset(), lineNumber);
}

size_t DebugInfos::lineNumber(size_t offset) {

	if (m_lines.empty()) {
		return 1;
	}

	auto line = m_lines.lower_bound(offset);

	if (line == m_lines.end()) {
		return (--line)->second;
	}

	if ((line != m_lines.begin()) && (line->first > offset)) {
		line--;
	}

	if ((line != m_lines.begin()) && (line->first == offset)) {
		line--;
	}

	return line->second;
}
