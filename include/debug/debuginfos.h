#ifndef MINT_DEBUGINFOS_H
#define MINT_DEBUGINFOS_H

#include "config.h"

#include <cstddef>
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

#endif // MINT_DEBUGINFOS_H
