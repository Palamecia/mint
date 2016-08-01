#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <list>

class FileSystem {
public:
	static FileSystem &instance();

	std::string getModulePath(const std::string &module);

protected:
	FileSystem();

private:
	std::list<std::string> m_libraryPath;
};

#endif // FILE_SYSTEM_H
