#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "config.h"

#include <string>
#include <list>

namespace mint {

class MINT_EXPORT FileSystem {
public:
	static FileSystem &instance();

	std::string getModulePath(const std::string &module);
	std::string getPluginPath(const std::string &plugin);

protected:
	FileSystem();

private:
	std::list<std::string> m_libraryPath;
};

}

#endif // FILE_SYSTEM_H
