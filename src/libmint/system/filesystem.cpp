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

#include "mint/system/filesystem.h"
#include "mint/system/errno.h"
#include <cerrno>

#ifdef OS_WINDOWS
#include "win32/globalsid.h"
#include <Userenv.h>
#else
#include <pwd.h>
#include <grp.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <filesystem>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <string>
#include <chrono>
#include <memory>

#ifdef OS_WINDOWS
static constexpr const std::chrono::milliseconds FILE_TIME_DELTA {11644473600000};
static constexpr const char *LIBRARY_EXTENSION = ".dll";
static constexpr const char PATH_SEPARATOR = ';';
#else
static constexpr const std::chrono::milliseconds FILE_TIME_DELTA {-6437664000000};
static constexpr const char *LIBRARY_EXTENSION = ".so";
static constexpr const char PATH_SEPARATOR = ':';
#endif

static constexpr const char *LIBRARY_PATH_VAR = "MINT_LIBRARY_PATH";

using namespace mint;

extern "C" void find_mint(void) {}

namespace {

std::filesystem::path format_module_path(const std::string &mint_path) {
	std::string path = mint_path;
	std::replace(path.begin(), path.end(), '.', '/');
	return path;
}

#ifdef OS_WINDOWS
std::wstring wchar_from_multi_byte(const std::string &str) {
	const int length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (std::wstring buffer(length, L'\0'); MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.data(), length)) {
		return buffer;
	}
	return {};
}

std::string wchar_to_multi_byte(const std::wstring &str) {
	const int length = WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, nullptr, 0, nullptr, nullptr);
	if (std::string buffer(length, '\0');
		WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, buffer.data(), length, nullptr, nullptr)) {
		return buffer;
	}
	return {};
}
#endif

}

FileSystem::FileSystem() {

#ifdef OS_WINDOWS
	wchar_t dli_fname[PATH_LENGTH];
	GetModuleFileNameW(nullptr, dli_fname, PATH_LENGTH);
	const std::filesystem::path library_path = std::filesystem::path(dli_fname).parent_path().parent_path() / "lib";
	m_library_path.emplace_back(library_path / "mint");
	m_scripts_path = library_path / "mint-scripts";
#else
	Dl_info dl_info;
	dladdr(reinterpret_cast<void *>(find_mint), &dl_info);
	const std::filesystem::path library_path = std::filesystem::path(dl_info.dli_fname).parent_path();
	m_library_path.emplace_back(library_path / "mint");
	m_scripts_path = library_path / "mint-scripts";
#endif

	if (const char *var = getenv(LIBRARY_PATH_VAR)) {

		std::string path;
		std::istringstream stream(var);

		while (getline(stream, path, PATH_SEPARATOR)) {
			m_library_path.emplace_back(path);
		}
	}
}

FileSystem &FileSystem::instance() {
	static FileSystem g_instance;
	return g_instance;
}

std::filesystem::path FileSystem::get_main_module_path() const {
	return m_main_module_path;
}

void FileSystem::set_main_module_path(const std::filesystem::path &path) {

	m_main_module_path = std::filesystem::weakly_canonical(path);

	std::string load_path = m_main_module_path.generic_string();
	if (auto pos = load_path.rfind('/'); pos != std::string::npos) {
		add_to_path(std::filesystem::absolute(load_path.erase(pos)));
	}
}

std::filesystem::path FileSystem::get_module_path(const std::string &module) const {

	const std::filesystem::path module_path = format_module_path(module).replace_extension(".mn");

	if (const std::filesystem::path full_path = std::filesystem::absolute(module_path);
		std::filesystem::exists(full_path) && check_file_access(full_path, READABLE_FLAG)) {
		return full_path;
	}

	for (const std::filesystem::path &path : m_library_path) {
		if (const std::filesystem::path full_path = std::filesystem::absolute(path / module_path);
			std::filesystem::exists(full_path) && check_file_access(full_path, READABLE_FLAG)) {
			return full_path;
		}
	}

	return {};
}

std::filesystem::path FileSystem::get_plugin_path(const std::string &plugin) const {

	std::filesystem::path plugin_path = format_module_path(plugin).replace_extension(LIBRARY_EXTENSION);

	if (std::filesystem::exists(plugin_path) && check_file_access(plugin_path, READABLE_FLAG)) {
		return plugin_path;
	}

	for (const std::filesystem::path &path : m_library_path) {
		if (std::filesystem::path full_path = path / plugin_path;
			std::filesystem::exists(full_path) && check_file_access(full_path, READABLE_FLAG)) {
			return full_path;
		}
	}

	return {};
}

std::filesystem::path FileSystem::get_script_path(const std::filesystem::path &script) const {
	return std::filesystem::weakly_canonical(m_scripts_path / script / script).replace_extension(".mn");
}

const std::list<std::filesystem::path> &FileSystem::library_path() const {
	return m_library_path;
}

void FileSystem::add_to_path(const std::filesystem::path &path) {
	m_library_path.push_back(path);
}

std::string FileSystem::to_module_path(const std::filesystem::path &root_path, const std::filesystem::path &file_path) {
	std::string module_path = std::filesystem::relative(file_path, root_path).generic_string();
	module_path.resize(module_path.find('.'));
	std::replace(module_path.begin(), module_path.end(), '/', '.');
	return module_path;
}

std::filesystem::path FileSystem::to_system_path(const std::filesystem::path &root_path,
												 const std::string &module_path) {
	std::string file_path = module_path;
	std::replace(file_path.begin(), file_path.end(), '.', '/');
	return normalized(root_path / file_path);
}

std::filesystem::path FileSystem::system_root() {
#ifdef OS_WINDOWS
	wchar_t root_path[PATH_LENGTH];
	if (GetSystemDirectoryW(root_path, PATH_LENGTH)) {
		return root_path;
	}
	return {};
#else
	return std::string {std::filesystem::path::preferred_separator};
#endif
}

std::filesystem::path FileSystem::root_path() {
	return system_root().root_path();
}

std::filesystem::path FileSystem::home_path() {
#ifdef OS_WINDOWS
	HANDLE hnd = GetCurrentProcess();
	std::filesystem::path path;

	if (HANDLE token = nullptr; OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
		if (DWORD buffer_size = 0; !GetUserProfileDirectoryW(token, nullptr, &buffer_size) && buffer_size != 0) {
			auto user_directory = std::make_unique<wchar_t[]>(buffer_size);
			if (GetUserProfileDirectoryW(token, user_directory.get(), &buffer_size)) {
				user_directory[buffer_size - 1] = L'\0';
				path = user_directory.get();
			}
		}
		CloseHandle(token);
	}

	return path;
#else
	if (const struct passwd *pw = getpwuid(getuid())) {
		return pw->pw_dir;
	}
	return {};
#endif
}

bool FileSystem::check_file_access(const std::filesystem::path &path, AccessFlags flags) {
	int right = 0;
#ifdef OS_WINDOWS
	if (flags & READABLE_FLAG) {
		right |= 0x04;
	}
	if (flags & WRITABLE_FLAG) {
		right |= 0x02;
	}
	if (flags & EXECUTABLE_FLAG) {
		right |= 0x04;
	}
	const std::wstring generic_path = path.generic_wstring();
	if (_waccess(generic_path.c_str(), right) != 0) {
		if (errno == EACCES) {
			return false;
		}
		throw std::filesystem::filesystem_error("check_file_access", path, last_error_code());
	}
	return true;
#else
	if (flags & READABLE_FLAG) {
		right |= R_OK;
	}
	if (flags & WRITABLE_FLAG) {
		right |= W_OK;
	}
	if (flags & EXECUTABLE_FLAG) {
		right |= X_OK;
	}
	const std::string generic_path = path.generic_string();
	if (access(generic_path.c_str(), right) != 0) {
		if (errno == EACCES) {
			return false;
		}
		throw std::filesystem::filesystem_error("check_file_access", path, last_error_code());
	}
	return true;
#endif
}

bool FileSystem::check_file_permissions(const std::filesystem::path &path, Permissions permissions) {
#ifdef OS_WINDOWS

	PSID pOwner = 0;
	PSID pGroup = 0;
	PACL pDacl;
	Permissions data = 0;
	PSECURITY_DESCRIPTOR pSD;

	const std::wstring generic_path = path.generic_wstring();
	if (GetNamedSecurityInfoW(generic_path.c_str(), SE_FILE_OBJECT,
							  OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
							  &pOwner, &pGroup, &pDacl, NULL, &pSD)
		!= ERROR_SUCCESS) {
		throw std::filesystem::filesystem_error("check_file_permissions", path, last_error_code());
	}

	ACCESS_MASK access_mask;
	TRUSTEE_W trustee;

	enum {
		READ_MASK = 0x00000001,
		WRITE_MASK = 0x00000002,
		EXEC_MASK = 0x00000020
	};

	if (GlobalSid::g_instance.currentUserImpersonatedToken) {
		GENERIC_MAPPING mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS};
		PRIVILEGE_SET privileges;
		DWORD granted_access = 0;
		BOOL result = FALSE;
		DWORD generic_access_rights = GENERIC_READ;
		MapGenericMask(&generic_access_rights, &mapping);
		DWORD privileges_length = sizeof(privileges);
		if (AccessCheck(pSD, GlobalSid::g_instance.currentUserImpersonatedToken, generic_access_rights, &mapping, &privileges,
						&privileges_length, &granted_access, &result)
			&& result) {
			data |= READ_USER_FLAG;
		}
		privileges_length = sizeof(privileges);
		generic_access_rights = GENERIC_WRITE;
		MapGenericMask(&generic_access_rights, &mapping);
		if (AccessCheck(pSD, GlobalSid::g_instance.currentUserImpersonatedToken, generic_access_rights, &mapping, &privileges,
						&privileges_length, &granted_access, &result)
			&& result) {
			data |= WRITE_USER_FLAG;
		}
		privileges_length = sizeof(privileges);
		generic_access_rights = GENERIC_EXECUTE;
		MapGenericMask(&generic_access_rights, &mapping);
		if (AccessCheck(pSD, GlobalSid::g_instance.currentUserImpersonatedToken, generic_access_rights, &mapping, &privileges,
						&privileges_length, &granted_access, &result)
			&& result) {
			data |= EXEC_USER_FLAG;
		}
	}
	// fallback to GetEffectiveRightsFromAcl
	else if (GetEffectiveRightsFromAclW(pDacl, &GlobalSid::g_instance.currentUserTrusteeW, &access_mask) == ERROR_SUCCESS) {
		if (access_mask & READ_MASK) {
			data |= READ_USER_FLAG;
		}
		if (access_mask & WRITE_MASK) {
			data |= WRITE_USER_FLAG;
		}
		if (access_mask & EXEC_MASK) {
			data |= EXEC_USER_FLAG;
		}
	}

	BuildTrusteeWithSidW(&trustee, pOwner);
	if (GetEffectiveRightsFromAclW(pDacl, &trustee, &access_mask) == ERROR_SUCCESS) {
		if (access_mask & READ_MASK) {
			data |= READ_OWNER_FLAG;
		}
		if (access_mask & WRITE_MASK) {
			data |= WRITE_OWNER_FLAG;
		}
		if (access_mask & EXEC_MASK) {
			data |= EXEC_OWNER_FLAG;
		}
	}

	BuildTrusteeWithSidW(&trustee, pGroup);
	if (GetEffectiveRightsFromAclW(pDacl, &trustee, &access_mask) == ERROR_SUCCESS) {
		if (access_mask & READ_MASK) {
			data |= READ_GROUP_FLAG;
		}
		if (access_mask & WRITE_MASK) {
			data |= WRITE_GROUP_FLAG;
		}
		if (access_mask & EXEC_MASK) {
			data |= EXEC_GROUP_FLAG;
		}
	}

	if (GetEffectiveRightsFromAclW(pDacl, &GlobalSid::g_instance.worldTrusteeW, &access_mask) == ERROR_SUCCESS) {
		if (access_mask & READ_MASK) {
			data |= READ_OTHER_FLAG;
		}
		if (access_mask & WRITE_MASK) {
			data |= WRITE_OTHER_FLAG;
		}
		if (access_mask & EXEC_MASK) {
			data |= EXEC_OTHER_FLAG;
		}
	}

	LocalFree(pSD);
	return (data & permissions) == permissions;
#else
	mode_t mode = 0;

	if (permissions & READ_OWNER_FLAG) {
		mode |= S_IRUSR;
	}
	if (permissions & WRITE_OWNER_FLAG) {
		mode |= S_IWUSR;
	}
	if (permissions & EXEC_OWNER_FLAG) {
		mode |= S_IXUSR;
	}
	if (permissions & READ_USER_FLAG) {
		mode |= S_IRUSR;
	}
	if (permissions & WRITE_USER_FLAG) {
		mode |= S_IWUSR;
	}
	if (permissions & EXEC_USER_FLAG) {
		mode |= S_IXUSR;
	}
	if (permissions & READ_GROUP_FLAG) {
		mode |= S_IRGRP;
	}
	if (permissions & WRITE_GROUP_FLAG) {
		mode |= S_IWGRP;
	}
	if (permissions & EXEC_GROUP_FLAG) {
		mode |= S_IXGRP;
	}
	if (permissions & READ_OTHER_FLAG) {
		mode |= S_IROTH;
	}
	if (permissions & WRITE_OTHER_FLAG) {
		mode |= S_IWOTH;
	}
	if (permissions & EXEC_OTHER_FLAG) {
		mode |= S_IXOTH;
	}
	struct stat infos;
	const std::string generic_path = path.generic_string();
	if (stat(generic_path.c_str(), &infos) != 0) {
		throw std::filesystem::filesystem_error("check_file_permissions", path, last_error_code());
	}
	return (infos.st_mode & mode) == mode;
#endif
}

bool FileSystem::is_root(const std::filesystem::path &path) {
	return path == path.root_path();
}

bool FileSystem::is_bundle(const std::filesystem::path &path) {
#ifdef OS_MAC
	return /// \todo OSX
#else
	return false;
#endif
}

bool FileSystem::is_hidden(const std::filesystem::path &path) {
#ifdef OS_WINDOWS
	const std::wstring generic_path = path.generic_wstring();
	DWORD infos = GetFileAttributesW(generic_path.c_str());

	if (infos != INVALID_FILE_ATTRIBUTES) {
		return infos & FILE_ATTRIBUTE_HIDDEN;
	}

	return false;
#else
	const std::string file_name = path.filename().generic_string();
	return !file_name.empty() && file_name[0] == '.';
#endif
}

bool FileSystem::is_canonical(const std::filesystem::path &path) {
	return path == std::filesystem::weakly_canonical(path);
}

bool FileSystem::is_normalized(const std::filesystem::path &path) {
	return path.generic_string() == normalized(path).generic_string();
}

std::filesystem::path FileSystem::normalized(const std::filesystem::path &path) {
	return path.lexically_normal().make_preferred();
}

std::filesystem::file_time_type FileSystem::from_system_time(const std::chrono::system_clock::time_point &time) {
	return std::filesystem::file_time_type(time.time_since_epoch() + FILE_TIME_DELTA);
}

std::chrono::system_clock::time_point FileSystem::to_system_time(const std::filesystem::file_time_type &time) {
	return std::chrono::system_clock::time_point(time.time_since_epoch() - FILE_TIME_DELTA);
}

std::filesystem::file_time_type FileSystem::birth_time(const std::filesystem::path &path) {
#ifdef OS_WINDOWS
	const std::wstring generic_path = path.generic_wstring();
	HANDLE handle = CreateFileW(generic_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		throw std::filesystem::filesystem_error("birth_time", path, last_error_code());
	}
	FILETIME time;
	if (!GetFileTime(handle, &time, nullptr, nullptr)) {
		CloseHandle(handle);
		throw std::filesystem::filesystem_error("birth_time", path, last_error_code());
	}
	CloseHandle(handle);
	return std::filesystem::file_time_type(std::filesystem::file_time_type::duration(
		(std::filesystem::file_time_type::duration::rep(time.dwHighDateTime) << 32) + time.dwLowDateTime));
#else
	struct stat infos;
	const std::string generic_path = path.generic_string();
	if (stat(generic_path.c_str(), &infos) != 0) {
		throw std::filesystem::filesystem_error("birth_time", path, last_error_code());
	}
	return std::filesystem::file_time_type(std::chrono::seconds(infos.st_ctim.tv_sec)
										   + std::chrono::nanoseconds(infos.st_ctim.tv_nsec) + FILE_TIME_DELTA);
#endif
}

std::filesystem::file_time_type FileSystem::last_read_time(const std::filesystem::path &path) {
#ifdef OS_WINDOWS
	const std::wstring generic_path = path.generic_wstring();
	HANDLE handle = CreateFileW(generic_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE) {
		throw std::filesystem::filesystem_error("last_read_time", path, last_error_code());
	}
	FILETIME time;
	if (!GetFileTime(handle, nullptr, &time, nullptr)) {
		CloseHandle(handle);
		throw std::filesystem::filesystem_error("last_read_time", path, last_error_code());
	}
	CloseHandle(handle);
	return std::filesystem::file_time_type(std::filesystem::file_time_type::duration(
		(std::filesystem::file_time_type::duration::rep(time.dwHighDateTime) << 32) + time.dwLowDateTime));
#else
	struct stat infos;
	const std::string generic_path = path.generic_string();
	if (stat(generic_path.c_str(), &infos) != 0) {
		throw std::filesystem::filesystem_error("last_read_time", path, last_error_code());
	}
	return std::filesystem::file_time_type(std::chrono::seconds(infos.st_atim.tv_sec)
										   + std::chrono::nanoseconds(infos.st_atim.tv_nsec) + FILE_TIME_DELTA);
#endif
}

std::string FileSystem::owner(const std::filesystem::path &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	const std::wstring generic_path = path.generic_wstring();
	if (GetNamedSecurityInfoW(generic_path.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, NULL, NULL,
							  NULL, &pSD)
		!= ERROR_SUCCESS) {
		throw std::filesystem::filesystem_error("owner", path, last_error_code());
	}
	DWORD lowner = 0;
	DWORD ldomain = 0;
	SID_NAME_USE use = SidTypeUnknown;
	LookupAccountSidW(nullptr, pOwner, nullptr, &lowner, nullptr, &ldomain, &use);
	std::wstring owner(lowner, L'\0');
	std::wstring domain(ldomain, L'\0');
	if (!LookupAccountSidW(nullptr, pOwner, owner.data(), &lowner, domain.data(), &ldomain, &use)) {
		throw std::filesystem::filesystem_error("owner", path, last_error_code());
	}
	return wchar_to_multi_byte(owner);
#else
	if (struct passwd *pw = getpwuid(FileSystem::owner_id(path))) {
		return pw->pw_name;
	}
	throw std::filesystem::filesystem_error("owner", path, last_error_code());
#endif
}

std::string FileSystem::group(const std::filesystem::path &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	const std::wstring generic_path = path.generic_wstring();
	if (GetNamedSecurityInfoW(generic_path.c_str(), SE_FILE_OBJECT, GROUP_SECURITY_INFORMATION, 0, &pOwner, 0, 0, &pSD)
		!= ERROR_SUCCESS) {
		throw std::filesystem::filesystem_error("group", path, last_error_code());
	}
	DWORD lowner = 0;
	DWORD ldomain = 0;
	SID_NAME_USE use = SidTypeUnknown;
	LookupAccountSidW(nullptr, pOwner, nullptr, &lowner, nullptr, &ldomain, &use);
	std::wstring owner(lowner, L'\0');
	std::wstring domain(ldomain, L'\0');
	if (!LookupAccountSidW(nullptr, pOwner, owner.data(), &lowner, domain.data(), &ldomain, &use)) {
		throw std::filesystem::filesystem_error("group", path, last_error_code());
	}
	return wchar_to_multi_byte(owner);
#else
	if (struct group *gr = getgrgid(FileSystem::group_id(path))) {
		return gr->gr_name;
	}
	throw std::filesystem::filesystem_error("group", path, last_error_code());
#endif
}

uid_t FileSystem::owner_id(const std::filesystem::path &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	const std::wstring generic_path = path.generic_wstring();
	if (GetNamedSecurityInfoW(generic_path.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, 0, 0, 0, &pSD)
		!= ERROR_SUCCESS) {
		throw std::filesystem::filesystem_error("owner_id", path, last_error_code());
	}
	return reinterpret_cast<uid_t>(pOwner);
#else
	struct stat infos;
	const std::string generic_path = path.generic_string();
	if (stat(generic_path.c_str(), &infos) != 0) {
		throw std::filesystem::filesystem_error("owner_id", path, last_error_code());
	}
	return infos.st_uid;
#endif
}

gid_t FileSystem::group_id(const std::filesystem::path &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	const std::wstring generic_path = path.generic_wstring();
	if (GetNamedSecurityInfoW(generic_path.c_str(), SE_FILE_OBJECT, GROUP_SECURITY_INFORMATION, 0, &pOwner, 0, 0, &pSD)
		!= ERROR_SUCCESS) {
		throw std::filesystem::filesystem_error("group_id", path, last_error_code());
	}
	return reinterpret_cast<gid_t>(pOwner);
#else
	struct stat infos;
	const std::string generic_path = path.generic_string();
	if (stat(generic_path.c_str(), &infos) != 0) {
		throw std::filesystem::filesystem_error("group_id", path, last_error_code());
	}
	return infos.st_gid;
#endif
}

bool FileSystem::is_subpath(const std::filesystem::path &path, const std::filesystem::path &base) {
	auto relative_path = std::filesystem::relative(path, base);
	return !relative_path.empty() && relative_path.native()[0] != '.';
}

FILE *mint::open_file(const std::filesystem::path &path, const char *mode) {
#ifdef OS_WINDOWS
	const std::wstring generic_path = path.generic_wstring();
	const std::wstring mode_str = wchar_from_multi_byte(mode);
	return _wfopen(generic_path.c_str(), mode_str.c_str());
#else
	const std::string generic_path = path.generic_string();
	return fopen(generic_path.c_str(), mode);
#endif
}
