#ifndef DEBUG_INFOS_H
#define DEBUG_INFOS_H

#include <map>

class Module;

class DebugInfos {
public:
	void newLine(Module *module, size_t lineNumber);
	size_t lineNumber(size_t offset);

private:
	std::map<size_t, size_t> m_lines;
};

#endif // DEBUG_INFOS_H
