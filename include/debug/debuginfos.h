#ifndef DEBUG_INFOS_H
#define DEBUG_INFOS_H

#include "config.h"

#include <stddef.h>
#include <map>

namespace mint {

class Module;

class MINT_EXPORT DebugInfos {
public:
	void newLine(Module *module, size_t lineNumber);
	size_t lineNumber(size_t offset);

private:
	std::map<size_t, size_t> m_lines;
};

}

#endif // DEBUG_INFOS_H
