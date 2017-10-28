#include "ast/debuginfos.h"
#include "ast/module.h"

using namespace std;

void DebugInfos::newLine(Module *module, size_t lineNumber) {
	m_lines.emplace(module->m_tree.size(), lineNumber);
}

size_t DebugInfos::lineNumber(size_t offset) {

	auto line = m_lines.lower_bound(offset);

	if (line != m_lines.begin()) {
		line--;
	}

	if (line->first == offset) {
		line--;
	}

	return line->second;
}
