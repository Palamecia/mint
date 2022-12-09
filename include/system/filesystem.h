#ifndef MINT_FILESYSTEM_H
#define MINT_FILESYSTEM_H

#include "config.h"

#ifdef OS_WINDOWS
#include <Windows.h>
typedef int uid_t; /// \todo Windows prefered type
typedef int gid_t; /// \todo Windows prefered type
#else
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

	class MINT_EXPORT Error {
	public:
		Error(bool status);
		Error(const Error &other) noexcept;

		Error &operator =(const Error &other) noexcept;

#ifdef OS_WINDOWS
		static Error fromWindowsLastError();
#endif

		operator bool() const;
		int getErrno() const;

	private:
		Error(bool _status, int _errno);

		bool m_status;
		int m_errno;
	};

#ifdef OS_UNIX
	static constexpr const char separator = '/';
	static constexpr const size_t path_length = PATH_MAX;
#else
	static const char separator = '\\';
	static constexpr const size_t path_length = _MAX_PATH;
#endif

	class MINT_EXPORT iterator : public std::iterator<std::input_iterator_tag, std::string> {
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
			typedef std::shared_ptr<WIN32_FIND_DATAW> entry_type;
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

	static FileSystem &instance();

	std::string rootPath() const;
	std::string homePath() const;

	std::string currentPath() const;
	Error setCurrentPath(const std::string &path);

	std::string absolutePath(const std::string &path) const;
	std::string relativePath(const std::string &root, const std::string &path) const;

	Error copy(const std::string &source, const std::string &target);
	Error rename(const std::string &source, const std::string &target);
	Error remove(const std::string &source);
	Error createLink(const std::string &path, const std::string &target);
	Error createDirectory(const std::string &path, bool recursive);
	Error removeDirectory(const std::string &path, bool recursive);

	iterator browse(const std::string &path);
	iterator begin();
	iterator end();

	std::string getModulePath(const std::string &module) const;
	std::string getPluginPath(const std::string &plugin) const;
	void addToPath(const std::string &path);

	static std::string systemRoot();
	static std::string cleanPath(const std::string &path);
	static std::string nativePath(const std::string &path);

	static bool checkFileAccess(const std::string &path, AccessFlags flags);
	static bool checkFilePermissions(const std::string &path, Permissions permissions);

	static bool isAbsolute(const std::string &path);
	static bool isClean(const std::string &path);

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
	mutable std::string m_rootPath;
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

#endif // MINT_FILESYSTEM_H
