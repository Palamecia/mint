#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "config.h"

#ifdef OS_UNIX
#include <dirent.h>
#endif
#include <memory>
#include <string>
#include <list>

namespace mint {

class MINT_EXPORT FileSystem {
public:
	typedef int AccessFlags;
	enum AccessRight : AccessFlags {
		exists = 0x00,
		readable = 0x04,
		writable = 0x02,
		executable = 0x01
	};

	typedef int Permissions;
	enum Permission : Permissions {
		read_owner = 0x4000, ///< The file is readable by the owner of the file
		write_owner = 0x2000, ///< The file is writable by the owner of the file
		exec_owner = 0x1000, ///< The file is executable by the owner of the file
		read_user = 0x0400, ///< The file is readable by the user
		write_user = 0x0200, ///< The file is writable by the user
		exec_user = 0x0100, ///< The file is executable by the user
		read_group = 0x0040, ///< The file is readable by the group
		write_group = 0x0020, ///< The file is writable by the group
		exec_group = 0x0010, ///< The file is executable by the group
		read_other = 0x0004, ///< The file is readable by anyone
		write_other = 0x0002, ///< The file is writable by anyone
		exec_other = 0x0001 ///< The file is executable by anyone
	};

#ifdef OS_UNIX
	static constexpr const char separator = '/';
#else
	static const char separator = '\\';
#endif

	class iterator : public std::iterator<std::input_iterator_tag, std::string> {
	public:
		iterator &operator++();
		iterator operator++(int);
		bool operator ==(const iterator &other) const;
		bool operator !=(const iterator &other) const;
		value_type operator *() const;

	protected:
		iterator();
		iterator(const std::string &path);
		friend class FileSystem;

	private:
		class data {
		public:
#ifdef OS_WINDOWS
			typedef HANDLE context_type;
			typedef LPWIN32_FIND_DATA entry_type;
#else
			typedef DIR *context_type;
			typedef dirent *entry_type;
#endif
			data(const std::string &path);
			~data();

			entry_type first();
			entry_type next();

		private:
			context_type m_context;
			std::string m_path;
		};

		std::shared_ptr<data> m_data;
		data::entry_type m_entry;
	};

	~FileSystem() = default;

	static FileSystem &instance();

	std::string homePath() const;
	std::string currentPath() const;
	std::string absolutePath(const std::string &path) const;
	std::string relativePath(const std::string &root, const std::string &path) const;

	bool copy(const std::string &source, const std::string &target);
	bool rename(const std::string &source, const std::string &target);
	bool remove(const std::string &source);
	bool createLink(const std::string &path, const std::string &target);
	bool createDirectory(const std::string &path, bool recursive);
	bool removeDirectory(const std::string &path, bool recursive);

	iterator browse(const std::string &path);
	iterator begin();
	iterator end();

	std::string getModulePath(const std::string &module) const;
	std::string getPluginPath(const std::string &plugin) const;

	static std::string systemRoot();
	static std::string cleanPath(const std::string &path);
	static std::string nativePath(const std::string &path);

	static bool checkFileAccess(const std::string &path, AccessFlags flags);
	static bool checkFilePermissions(const std::string &path, Permissions permissions);

	static bool isRoot(const std::string &path);
	static bool isFile(const std::string &path);
	static bool isDirectory(const std::string &path);
	static bool isSymlink(const std::string &path);
	static bool isBundle(const std::string &path);
	static bool isHidden(const std::string &path);

	static size_t sizeOf(const std::string &path);
	static time_t birthTime(const std::string &path);
	static time_t lastRead(const std::string &path);
	static time_t lastModified(const std::string &path);
	static std::string owner(const std::string &path);
	static std::string group(const std::string &path);
	static uid_t ownerId(const std::string &path);
	static gid_t groupId(const std::string &path);
	static std::string symlinkTarget(const std::string &path);

protected:
	FileSystem();
	FileSystem(const FileSystem &other) = delete;
	FileSystem &operator =(const FileSystem &other) = delete;

private:
	mutable std::string m_homePath;
	mutable std::string m_currentPath;
	std::list<std::string> m_libraryPath;
};

#ifdef OS_WINDOWS
MINT_EXPORT std::wstring string_to_windows_path(const std::string &str);
MINT_EXPORT std::string windows_path_to_string(const std::wstring &path);
#endif

MINT_EXPORT FILE *open_file(const char *path, const char *mode);

}

#endif // FILE_SYSTEM_H
