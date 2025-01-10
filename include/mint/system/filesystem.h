/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_FILESYSTEM_H
#define MINT_FILESYSTEM_H

#include "mint/config.h"
#include "mint/system/errno.h"

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
		EXISTS_FLAG = 0x00,
		READABLE_FLAG = 0x04,
		WRITABLE_FLAG = 0x02,
		EXECUTABLE_FLAG = 0x01
	};

	typedef int Permissions;
	enum Permission : Permissions {
		READ_OWNER_FLAG = 0x4000, ///< The file is readable by the owner of the file
		WRITE_OWNER_FLAG = 0x2000, ///< The file is writable by the owner of the file
		EXEC_OWNER_FLAG = 0x1000, ///< The file is executable by the owner of the file
		READ_USER_FLAG = 0x0400, ///< The file is readable by the user
		WRITE_USER_FLAG = 0x0200, ///< The file is writable by the user
		EXEC_USER_FLAG = 0x0100, ///< The file is executable by the user
		READ_GROUP_FLAG = 0x0040, ///< The file is readable by the group
		WRITE_GROUP_FLAG = 0x0020, ///< The file is writable by the group
		EXEC_GROUP_FLAG = 0x0010, ///< The file is executable by the group
		READ_OTHER_FLAG = 0x0004, ///< The file is readable by anyone
		WRITE_OTHER_FLAG = 0x0002, ///< The file is writable by anyone
		EXEC_OTHER_FLAG = 0x0001 ///< The file is executable by anyone
	};

#ifdef OS_UNIX
	static constexpr const char SEPARATOR = '/';
	static constexpr const size_t PATH_LENGTH = PATH_MAX;
#else
	static constexpr const char SEPARATOR = '\\';
	static constexpr const size_t PATH_LENGTH = _MAX_PATH;
#endif

	class MINT_EXPORT iterator {
	public:
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = std::string;
		using pointer = value_type *;
		using reference = value_type &;

		iterator &operator++();
		iterator operator++(int);
		bool operator ==(const iterator &other) const;
		bool operator !=(const iterator &other) const;
		value_type operator *() const;

	protected:
		iterator();
		explicit iterator(const std::string &path);
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
			explicit data(const std::string &path);
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

	struct path_less {
		bool operator ()(const std::string &path1, const std::string &path2) const;
	};

	static FileSystem &instance();

	std::string root_path() const;
	std::string home_path() const;

	std::string current_path() const;
	SystemError set_current_path(const std::string &path);

	std::string absolute_path(const std::string &path) const;
	std::string relative_path(const std::string &root, const std::string &path) const;

	SystemError copy(const std::string &source, const std::string &target);
	SystemError rename(const std::string &source, const std::string &target);
	SystemError remove(const std::string &source);
	SystemError create_link(const std::string &path, const std::string &target);
	SystemError create_directory(const std::string &path, bool recursive);
	SystemError remove_directory(const std::string &path, bool recursive);

	iterator browse(const std::string &path);
	iterator begin();
	iterator end();

	std::string get_module_path(const std::string &module) const;
	std::string get_plugin_path(const std::string &plugin) const;
	std::string get_script_path(const std::string &script) const;
	const std::list<std::string> &library_path() const;
	void add_to_path(const std::string &path);

	static std::string system_root();
	static std::string clean_path(const std::string &path);
	static std::string native_path(const std::string &path);

	static bool check_file_access(const std::string &path, AccessFlags flags);
	static bool check_file_permissions(const std::string &path, Permissions permissions);

	static bool is_absolute(const std::string &path);
	static bool is_clean(const std::string &path);

	static bool is_root(const std::string &path);
	static bool is_file(const std::string &path);
	static bool is_directory(const std::string &path);
	static bool is_symlink(const std::string &path);
	static bool is_bundle(const std::string &path);
	static bool is_hidden(const std::string &path);

	static size_t size_of(const std::string &path);
	static time_t birth_time(const std::string &path);
	static time_t last_read(const std::string &path);
	static time_t last_modified(const std::string &path);
	static std::string owner(const std::string &path);
	static std::string group(const std::string &path);
	static uid_t owner_id(const std::string &path);
	static gid_t group_id(const std::string &path);
	static std::string symlink_target(const std::string &path);

	static bool is_equal_path(const std::string& path1, const std::string& path2);
	static bool is_sub_path(const std::string& sub_path, const std::string& path);

protected:
	FileSystem();
	FileSystem(const FileSystem &other) = delete;
	FileSystem &operator =(const FileSystem &other) = delete;

private:
	mutable std::string m_root_path;
	mutable std::string m_home_path;
	mutable std::string m_current_path;
	std::list<std::string> m_library_path;
	std::string m_scripts_path;
};

#ifdef OS_WINDOWS
MINT_EXPORT std::wstring string_to_windows_path(const std::string &str);
MINT_EXPORT std::string windows_path_to_string(const std::wstring &path);
#endif

MINT_EXPORT FILE *open_file(const char *path, const char *mode);

}

#endif // MINT_FILESYSTEM_H
