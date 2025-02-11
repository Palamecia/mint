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

#include <filesystem>
#include <sys/stat.h>
#include <cstdint>
#include <chrono>
#include <string>
#include <list>

#ifdef OS_WINDOWS
using uid_t = std::intptr_t;
using gid_t = std::intptr_t;
#else
#include <linux/limits.h>
#endif

namespace mint {

class MINT_EXPORT FileSystem {
public:
	using AccessFlags = std::uint8_t;

	enum AccessRight : AccessFlags {
		EXECUTABLE_FLAG = 0x01,
		WRITABLE_FLAG = 0x02,
		READABLE_FLAG = 0x04
	};

	using Permissions = std::uint16_t;

	enum Permission : Permissions {
		READ_OWNER_FLAG = 0x4000,  ///< The file is readable by the owner of the file
		WRITE_OWNER_FLAG = 0x2000, ///< The file is writable by the owner of the file
		EXEC_OWNER_FLAG = 0x1000,  ///< The file is executable by the owner of the file
		READ_USER_FLAG = 0x0400,   ///< The file is readable by the user
		WRITE_USER_FLAG = 0x0200,  ///< The file is writable by the user
		EXEC_USER_FLAG = 0x0100,   ///< The file is executable by the user
		READ_GROUP_FLAG = 0x0040,  ///< The file is readable by the group
		WRITE_GROUP_FLAG = 0x0020, ///< The file is writable by the group
		EXEC_GROUP_FLAG = 0x0010,  ///< The file is executable by the group
		READ_OTHER_FLAG = 0x0004,  ///< The file is readable by anyone
		WRITE_OTHER_FLAG = 0x0002, ///< The file is writable by anyone
		EXEC_OTHER_FLAG = 0x0001   ///< The file is executable by anyone
	};

#ifdef OS_UNIX
	static constexpr const size_t PATH_LENGTH = PATH_MAX;
#else
	static constexpr const size_t PATH_LENGTH = _MAX_PATH;
#endif

	FileSystem(FileSystem &&) = delete;
	FileSystem(const FileSystem &) = delete;
	~FileSystem() = default;

	FileSystem &operator=(FileSystem &&) = delete;
	FileSystem &operator=(const FileSystem &) = delete;

	static FileSystem &instance();

	[[nodiscard]] std::filesystem::path get_main_module_path() const;
	void set_main_module_path(const std::filesystem::path &path);
	
	[[nodiscard]] std::filesystem::path get_module_path(const std::string &module) const;
	[[nodiscard]] std::filesystem::path get_plugin_path(const std::string &plugin) const;
	[[nodiscard]] std::filesystem::path get_script_path(const std::filesystem::path &script) const;
	[[nodiscard]] const std::list<std::filesystem::path> &library_path() const;
	void add_to_path(const std::filesystem::path &path);

	static std::string to_module_path(const std::filesystem::path &root_path, const std::filesystem::path &file_path);
	static std::filesystem::path to_system_path(const std::filesystem::path &root_path, const std::string &module_path);

	static std::filesystem::path system_root();
	static std::filesystem::path root_path();
	static std::filesystem::path home_path();

	static bool check_file_access(const std::filesystem::path &path, AccessFlags flags);
	static bool check_file_permissions(const std::filesystem::path &path, Permissions permissions);
	
	static bool is_root(const std::filesystem::path &path);
	static bool is_bundle(const std::filesystem::path &path);
	static bool is_hidden(const std::filesystem::path &path);
	static bool is_canonical(const std::filesystem::path &path);
	static bool is_normalized(const std::filesystem::path &path);

	static std::filesystem::path normalized(const std::filesystem::path &path);

	static std::filesystem::file_time_type from_system_time(const std::chrono::system_clock::time_point &time);
	static std::chrono::system_clock::time_point to_system_time(const std::filesystem::file_time_type &time);

	static std::filesystem::file_time_type birth_time(const std::filesystem::path &path);
	static std::filesystem::file_time_type last_read_time(const std::filesystem::path &path);
	static std::string owner(const std::filesystem::path &path);
	static std::string group(const std::filesystem::path &path);
	static uid_t owner_id(const std::filesystem::path &path);
	static gid_t group_id(const std::filesystem::path &path);

	static bool is_subpath(const std::filesystem::path &path,
						   const std::filesystem::path &base = std::filesystem::current_path());

protected:
	FileSystem();

private:
	std::list<std::filesystem::path> m_library_path;
	std::filesystem::path m_main_module_path;
	std::filesystem::path m_scripts_path;
};

MINT_EXPORT FILE *open_file(const std::filesystem::path &path, const char *mode);

}

#endif // MINT_FILESYSTEM_H
