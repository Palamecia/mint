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

#ifdef OS_WINDOWS
MINT_EXPORT std::wstring string_to_windows_path(const std::string &str);
MINT_EXPORT std::string windows_path_to_string(const std::wstring &path);
#endif

}

#endif // FILE_SYSTEM_H
