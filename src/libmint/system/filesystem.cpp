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
#include "mint/system/utf8.h"

#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef OS_WINDOWS
#include <windows.h>
#include <AclAPI.h>
#include <Userenv.h>
#else
#include <pwd.h>
#include <grp.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#endif

#ifdef OS_WINDOWS
#define PATH_SEPARATOR ';'

typedef struct _REPARSE_DATA_BUFFER {
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;

	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;

		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;

		struct {
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	};
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

time_t FileTimeToTimestamp(const FILETIME &time) {

	LARGE_INTEGER date, epoch;

	date.HighPart = time.dwHighDateTime;
	date.LowPart = time.dwLowDateTime;

	epoch.QuadPart = 11644473600000 * 10000;
	date.QuadPart -= epoch.QuadPart;

	return date.QuadPart / 10000000;
}

struct GlobalSid {
	TRUSTEE_W currentUserTrusteeW;
	TRUSTEE_W worldTrusteeW;
	PSID currentUserSID = 0;
	PSID worldSID = 0;
	HANDLE currentUserImpersonatedToken = nullptr;

	GlobalSid() {
		{
			{
				// Create TRUSTEE for current user
				HANDLE hnd = GetCurrentProcess();
				HANDLE token = 0;
				if (OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
					DWORD retsize = 0;
					// GetTokenInformation requires a buffer big enough for the TOKEN_USER struct and
					// the SID struct. Since the SID struct can have variable number of subauthorities
					// tacked at the end, its size is variable. Obtain the required size by first
					// doing a dummy GetTokenInformation call.
					GetTokenInformation(token, TokenUser, 0, 0, &retsize);
					if (retsize) {
						void *tokenBuffer = malloc(retsize);
						if (GetTokenInformation(token, TokenUser, tokenBuffer, retsize, &retsize)) {
							PSID tokenSid = reinterpret_cast<PTOKEN_USER>(tokenBuffer)->User.Sid;
							DWORD sidLen = ::GetLengthSid(tokenSid);
							currentUserSID = reinterpret_cast<PSID>(malloc(sidLen));
							if (CopySid(sidLen, currentUserSID, tokenSid)) {
								BuildTrusteeWithSidW(&currentUserTrusteeW, currentUserSID);
							}
						}
						free(tokenBuffer);
					}
					CloseHandle(token);
				}
				token = nullptr;
				if (OpenProcessToken(hnd, TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ,
									 &token)) {
					DuplicateToken(token, SecurityImpersonation, &currentUserImpersonatedToken);
					CloseHandle(token);
				}
				{
					// Create TRUSTEE for Everyone (World)
					SID_IDENTIFIER_AUTHORITY worldAuth = {SECURITY_WORLD_SID_AUTHORITY};
					if (AllocateAndInitializeSid(&worldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &worldSID)) {
						BuildTrusteeWithSidW(&worldTrusteeW, worldSID);
					}
				}
			}
		}
	}

	~GlobalSid() {
		free(currentUserSID);
		currentUserSID = 0;
		// worldSID was allocated with AllocateAndInitializeSid so it needs to be freed with FreeSid
		if (worldSID) {
			FreeSid(worldSID);
			worldSID = 0;
		}
		if (currentUserImpersonatedToken) {
			CloseHandle(currentUserImpersonatedToken);
			currentUserImpersonatedToken = nullptr;
		}
	}

	GlobalSid(const GlobalSid &other) = delete;
	GlobalSid &operator=(const GlobalSid &other) = delete;
};

static GlobalSid g_globalSid;
#else
#define PATH_SEPARATOR ':'
#endif

#define LIBRARY_PATH_VAR "MINT_LIBRARY_PATH"

using namespace mint;

#ifdef OS_UNIX
constexpr const char FileSystem::SEPARATOR;
#endif

extern "C" void find_mint(void) {}

std::string format_module_path(const std::string &mint_path) {

	std::string path = mint_path;
	replace(path.begin(), path.end(), '.', FileSystem::SEPARATOR);
	return path;
}

std::string get_parent_dir(const std::string &path) {
	return path.substr(0, path.find_last_of(FileSystem::SEPARATOR));
}

FileSystem::iterator::iterator(const std::string &path) :
	m_data(new data(path)) {
	m_entry = m_data->first();
}

FileSystem::iterator::iterator() :
	m_entry(nullptr) {}

FileSystem::iterator &FileSystem::iterator::operator++() {
	m_entry = m_data->next();
	return *this;
}

FileSystem::iterator FileSystem::iterator::operator++(int) {
	iterator other = *this;
	m_entry = m_data->next();
	return other;
}

bool FileSystem::iterator::operator==(const iterator &other) const {
	return m_entry == other.m_entry;
}

bool FileSystem::iterator::operator!=(const iterator &other) const {
	return m_entry != other.m_entry;
}

FileSystem::iterator::value_type FileSystem::iterator::operator*() const {
	if (m_entry) {
#ifdef OS_WINDOWS
		return windows_path_to_string(m_entry->cFileName);
#else
		return m_entry->d_name;
#endif
	}
	return value_type();
}

FileSystem::iterator::data::data(const std::string &path) :
	m_context(nullptr),
	m_path(path) {}

FileSystem::iterator::data::~data() {
	if (m_context) {
#ifdef OS_WINDOWS
		FindClose(m_context);
#else
		closedir(m_context);
#endif
	}
}

FileSystem::iterator::data::entry_type FileSystem::iterator::data::first() {
#ifdef OS_WINDOWS
	entry_type entry(new entry_type::element_type);
	std::wstring pattern = string_to_windows_path("\\\\?\\" + m_path + FileSystem::SEPARATOR + '*');
	if ((m_context = FindFirstFileW(pattern.c_str(), entry.get()))) {
		return entry;
	}
#else
	if ((m_context = opendir(m_path.c_str()))) {
		return readdir(m_context);
	}
#endif
	return nullptr;
}

FileSystem::iterator::data::entry_type FileSystem::iterator::data::next() {
#ifdef OS_WINDOWS
	entry_type entry(new entry_type::element_type);
	if (FindNextFileW(m_context, entry.get())) {
		return entry;
	}
	return nullptr;
#else
	return readdir(m_context);
#endif
}

FileSystem::FileSystem() {

	m_home_path = home_path();
	m_current_path = current_path();

#ifdef OS_WINDOWS
	wchar_t dli_fname[PATH_LENGTH];
	GetModuleFileNameW(nullptr, dli_fname, sizeof dli_fname / sizeof(wchar_t));
	const std::string library_path = get_parent_dir(get_parent_dir(windows_path_to_string(dli_fname))) + "\\lib";
	m_library_path.push_back(library_path + "\\mint");
	m_scripts_path = library_path + "\\mint-scripts";
#else
	Dl_info dl_info;
	dladdr(reinterpret_cast<void *>(find_mint), &dl_info);
	const std::string library_path = get_parent_dir(dl_info.dli_fname);
	m_library_path.push_back(library_path + "/mint");
	m_scripts_path = library_path + "/mint-scripts";
#endif

	if (const char *var = getenv(LIBRARY_PATH_VAR)) {

		std::string path;
		std::istringstream stream(var);

		while (getline(stream, path, PATH_SEPARATOR)) {
			m_library_path.push_back(path);
		}
	}
}

bool FileSystem::path_less::operator()(const std::string &path1, const std::string &path2) const {
#ifdef OS_WINDOWS
	return utf8_compare_case_insensitive(path1, path2) < 0;
#else
	return path1 < path2;
#endif
}

FileSystem &FileSystem::instance() {
	static FileSystem g_instance;
	return g_instance;
}

std::string FileSystem::root_path() const {

#ifdef OS_WINDOWS
	wchar_t lpBuffer[PATH_LENGTH];
	if (GetSystemDirectoryW(lpBuffer, sizeof(lpBuffer)) >= 2) {
		m_root_path = windows_path_to_string(lpBuffer).substr(0, 2);
	}
#else
	m_root_path = "/";
#endif

	return m_root_path;
}

std::string FileSystem::home_path() const {

#ifdef OS_WINDOWS
	HANDLE hnd = GetCurrentProcess();
	HANDLE token = 0;

	if (OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
		DWORD dwBufferSize = 0;
		if (!GetUserProfileDirectoryW(token, NULL, &dwBufferSize) && dwBufferSize != 0) {
			std::wstring userDirectory(dwBufferSize, L'\0');
			if (GetUserProfileDirectoryW(token, userDirectory.data(), &dwBufferSize)) {
				m_home_path = windows_path_to_string(userDirectory);
			}
		}

		CloseHandle(token);
	}
#else
	if (const struct passwd *pw = getpwuid(getuid())) {
		m_home_path = pw->pw_dir;
	}
#endif

	return m_home_path;
}

std::string FileSystem::current_path() const {

#ifdef OS_WINDOWS
	wchar_t currentName[PATH_LENGTH];
	DWORD size = GetCurrentDirectoryW(PATH_LENGTH, currentName);

	if (size != 0) {
		if (size > PATH_LENGTH) {
			std::wstring newCurrentName(size, L'\0');
			if (GetCurrentDirectoryW(PATH_LENGTH, newCurrentName.data()) != 0) {
				m_current_path = windows_path_to_string(newCurrentName);
			}
		}
		else {
			m_current_path = windows_path_to_string(currentName);
		}
	}
#else
	char path[PATH_LENGTH];

	if (getcwd(path, sizeof(path))) {
		m_current_path = path;
	}
#endif

	return m_current_path;
}

SystemError FileSystem::set_current_path(const std::string &path) {

#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	if (_wchdir(windows_path.c_str())) {
#else
	if (chdir(path.c_str())) {
#endif
		return false;
	}

	m_current_path = path;
	return true;
}

std::string FileSystem::absolute_path(const std::string &path) const {

	if (!path.empty() && path.front() == '~') {
		return absolute_path(home_path() + (path.c_str() + 1));
	}

#ifdef OS_WINDOWS
	wchar_t resolved_path[PATH_LENGTH];
	std::wstring windows_path = string_to_windows_path(path);
	if (GetFullPathNameW(windows_path.c_str(), PATH_LENGTH, resolved_path, nullptr)) {
		return windows_path_to_string(resolved_path);
	}
#else
	char resolved_path[PATH_LENGTH];
	if (realpath(path.c_str(), resolved_path)) {
		return resolved_path;
	}
#endif

	if (is_absolute(path)) {
		return clean_path(path);
	}

	return clean_path(current_path() + FileSystem::SEPARATOR + path);
}

std::string FileSystem::relative_path(const std::string &root, const std::string &path) const {

	std::string root_path = absolute_path(root);
	auto root_start = root_path.find(FileSystem::SEPARATOR);
	std::string root_directory = root_path.substr(0, root_start);

	std::string other_path = absolute_path(path);
	auto other_start = other_path.find(FileSystem::SEPARATOR);
	std::string other_directory = other_path.substr(0, other_start);

	while (is_equal_path(root_directory, other_directory) && (root_start != std::string::npos)
		   && (other_start != std::string::npos)) {

		auto root_stop = root_path.find(FileSystem::SEPARATOR, root_start + 1);
		root_directory = root_path.substr(root_start, root_stop - root_start);

		auto other_stop = other_path.find(FileSystem::SEPARATOR, other_start + 1);
		other_directory = other_path.substr(other_start, other_stop - other_start);

		if (is_equal_path(root_directory, other_directory)) {
			root_start = root_stop;
			other_start = other_stop;
		}
	}

	std::string relative_path;

	while (root_start != std::string::npos) {
		relative_path += "..";
		relative_path += FileSystem::SEPARATOR;
		root_start = root_path.find(FileSystem::SEPARATOR, root_start + 1);
	}

	if (other_start != std::string::npos) {
		relative_path += other_path.substr(other_start + 1);
	}

	return relative_path;
}

SystemError FileSystem::copy(const std::string &source, const std::string &target) {

	if (is_directory(source)) {
		if (!check_file_access(target, FileSystem::EXISTS_FLAG)) {
			if (SystemError error = create_directory(target, true)) {
				return error;
			}
		}
		for (auto it = browse(source); it != end(); ++it) {
			if (SystemError error = copy(source + FileSystem::SEPARATOR + *it, target + FileSystem::SEPARATOR + *it)) {
				return error;
			}
		}
	}

	if (FILE *input = open_file(source.c_str(), "rb")) {
		if (FILE *output = open_file(target.c_str(), "wb")) {
			SystemError status = true;
			byte_t block[4096];
			while (auto bytes = fread(block, sizeof(byte_t), sizeof(block), input)) {
				if (bytes != fwrite(block, sizeof(byte_t), bytes, output)) {
					status = false;
					break;
				}
			}
			fclose(output);
			fclose(input);
			return status;
		}
		fclose(input);
	}

	return false;
}

SystemError FileSystem::rename(const std::string &source, const std::string &target) {

	if (SystemError error = copy(source, target)) {
		return error;
	}

	return remove(source);
}

SystemError FileSystem::remove(const std::string &source) {

	if (is_directory(source)) {
		for (auto it = browse(source); it != end(); ++it) {
			if (SystemError error = remove(source + FileSystem::SEPARATOR + *it)) {
				return error;
			}
		}
		return remove_directory(source, false);
	}

#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(source);
	if (_wunlink(windows_path.c_str())) {
#else
	if (unlink(source.c_str())) {
#endif
		return false;
	}

	return true;
}

SystemError FileSystem::create_link(const std::string &path, const std::string &target) {
#ifdef OS_WINDOWS
	DWORD flags = 0;
	if (is_directory(path)) {
		flags = SYMBOLIC_LINK_FLAG_DIRECTORY;
	}
	std::wstring windows_path = string_to_windows_path(path);
	std::wstring windows_target_path = string_to_windows_path(path);
	if (!CreateSymbolicLinkW(windows_path.c_str(), windows_target_path.c_str(), flags)) {
		return SystemError::from_windows_last_error();
	}
#else
	if (symlink(path.c_str(), target.c_str())) {
		return false;
	}
#endif

	return true;
}

SystemError FileSystem::create_directory(const std::string &path, bool recursive) {

#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	if (_wmkdir(windows_path.c_str()) == 0) {
#else
	if (mkdir(path.c_str(), 0777) == 0) {
#endif
		return true;
	}

	if (recursive) {
		std::string absolute_path = FileSystem::absolute_path(path);
		std::string parent = absolute_path.substr(0, absolute_path.rfind(FileSystem::SEPARATOR));
		if ((parent != absolute_path) && !check_file_access(parent, EXISTS_FLAG)) {
			if (SystemError error = create_directory(parent, recursive)) {
				return error;
			}
			return create_directory(path, false);
		}
	}

	return false;
}

SystemError FileSystem::remove_directory(const std::string &path, bool recursive) {

#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	if (_wrmdir(windows_path.c_str()) == 0) {
#else
	if (rmdir(path.c_str()) == 0) {
#endif
		if (recursive) {
			std::string absolute_path = FileSystem::absolute_path(path);
			std::string parent = absolute_path.substr(0, absolute_path.rfind(FileSystem::SEPARATOR));
			if (parent != absolute_path) {
				return remove_directory(parent, recursive);
			}
		}
		return true;
	}

	return false;
}

FileSystem::iterator FileSystem::browse(const std::string &path) {
	return iterator(absolute_path(path));
}

FileSystem::iterator FileSystem::begin() {
	return browse(system_root());
}

FileSystem::iterator FileSystem::end() {
	static iterator g_end;
	return g_end;
}

std::string FileSystem::get_module_path(const std::string &module) const {

	const std::string module_path = format_module_path(module) + ".mn";

	if (const std::string full_path = FileSystem::instance().absolute_path(module_path);
		check_file_access(full_path, READABLE_FLAG)) {
		return full_path;
	}

	for (const std::string &path : m_library_path) {
		const std::string full_path = FileSystem::instance().absolute_path(path + FileSystem::SEPARATOR + module_path);
		if (check_file_access(full_path, READABLE_FLAG)) {
			return full_path;
		}
	}

	return {};
}

std::string FileSystem::get_plugin_path(const std::string &plugin) const {

	std::string plugin_path = format_module_path(plugin);
#ifdef OS_WINDOWS
	plugin_path += ".dll";
#else
	plugin_path += ".so";
#endif

	if (check_file_access(plugin_path, READABLE_FLAG)) {
		return plugin_path;
	}

	for (const std::string &path : m_library_path) {
		std::string full_path = path + FileSystem::SEPARATOR + plugin_path;
		if (check_file_access(full_path, READABLE_FLAG)) {
			return full_path;
		}
	}

	return {};
}

std::string FileSystem::get_script_path(const std::string &script) const {
	return clean_path(m_scripts_path + "/" + script + "/" + script + ".mn");
}

const std::list<std::string> &FileSystem::library_path() const {
	return m_library_path;
}

void FileSystem::add_to_path(const std::string &path) {
	m_library_path.push_back(path);
}

std::string FileSystem::system_root() {
#ifdef OS_WINDOWS
	wchar_t root_path[PATH_LENGTH];
	if (GetSystemDirectoryW(root_path, PATH_LENGTH)) {
		return windows_path_to_string(root_path);
	}
	return {};
#else
	return {FileSystem::SEPARATOR};
#endif
}

std::string FileSystem::clean_path(const std::string &path) {

	std::string clean_path = native_path(path);

	auto start = clean_path.find(FileSystem::SEPARATOR);
	while (start != std::string::npos) {
		auto stop = clean_path.find(FileSystem::SEPARATOR, start + 1);
		auto len = (stop == std::string::npos) ? clean_path.size() - start : stop - start - 1;
		std::string directory = clean_path.substr(start + 1, (stop == std::string::npos) ? stop : len);
		if (directory == ".") {
			clean_path.replace(start, len + 1, "");
			start = clean_path.find(FileSystem::SEPARATOR, start);
		}
		else if (directory == "..") {
			auto before = clean_path.rfind(FileSystem::SEPARATOR, start - 1);
			if (before != std::string::npos) {
				len = (stop == std::string::npos) ? clean_path.size() - before : stop - before - 1;
				clean_path.replace(before, len + 1, "");
				start = clean_path.find(FileSystem::SEPARATOR, before);
			}
			else {
				directory = clean_path.substr(0, start);
				if ((directory != ".") && (directory != "..")) {
					clean_path.replace(0, stop + 1, "");
					start = clean_path.find(FileSystem::SEPARATOR);
				}
				else {
					start = stop;
				}
			}
		}
		else {
			start = stop;
		}
	}

	return clean_path;
}

std::string FileSystem::native_path(const std::string &path) {

	std::string native_path = path;

	for (const char sep : {'/', '\\'}) {
		if (sep != FileSystem::SEPARATOR) {
			replace(native_path.begin(), native_path.end(), sep, FileSystem::SEPARATOR);
		}
	}

#ifdef OS_WINDOWS
	const char *native_path_ptr = native_path.c_str();

	if ((native_path_ptr[0] == FileSystem::SEPARATOR) && (native_path_ptr[1] != FileSystem::SEPARATOR)) {
		native_path = system_root() + (native_path_ptr + 1);
	}
#endif

	return native_path;
}

bool FileSystem::check_file_access(const std::string &path, AccessFlags flags) {
	int right = 0;
#ifdef OS_WINDOWS
	if (flags & EXISTS_FLAG) {
		right |= 0x00;
	}
	if (flags & READABLE_FLAG) {
		right |= 0x04;
	}
	if (flags & WRITABLE_FLAG) {
		right |= 0x02;
	}
	if (flags & EXECUTABLE_FLAG) {
		right |= 0x04;
	}
	std::wstring windows_path = string_to_windows_path(path);
	return _waccess(windows_path.c_str(), right) == 0;
#else
	if (flags & EXISTS_FLAG) {
		right |= F_OK;
	}
	if (flags & READABLE_FLAG) {
		right |= R_OK;
	}
	if (flags & WRITABLE_FLAG) {
		right |= W_OK;
	}
	if (flags & EXECUTABLE_FLAG) {
		right |= X_OK;
	}
	return access(path.c_str(), right) == 0;
#endif
}

bool FileSystem::check_file_permissions(const std::string &path, Permissions permissions) {
#ifdef OS_WINDOWS

	PSID pOwner = 0;
	PSID pGroup = 0;
	PACL pDacl;
	Permissions data = 0;
	PSECURITY_DESCRIPTOR pSD;

	std::wstring windows_path = string_to_windows_path(path);
	if (GetNamedSecurityInfoW(windows_path.c_str(), SE_FILE_OBJECT,
							  OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
							  &pOwner, &pGroup, &pDacl, 0, &pSD)
		== ERROR_SUCCESS) {

		ACCESS_MASK access_mask;
		TRUSTEE_W trustee;

		enum {
			READ_MASK = 0x00000001,
			WRITE_MASK = 0x00000002,
			EXEC_MASK = 0x00000020
		};

		if (g_globalSid.currentUserImpersonatedToken) {
			GENERIC_MAPPING mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS};
			PRIVILEGE_SET privileges;
			DWORD grantedAccess;
			BOOL result;
			DWORD genericAccessRights = GENERIC_READ;
			MapGenericMask(&genericAccessRights, &mapping);
			DWORD privilegesLength = sizeof(privileges);
			if (AccessCheck(pSD, g_globalSid.currentUserImpersonatedToken, genericAccessRights, &mapping, &privileges,
							&privilegesLength, &grantedAccess, &result)
				&& result) {
				data |= READ_USER_FLAG;
			}
			privilegesLength = sizeof(privileges);
			genericAccessRights = GENERIC_WRITE;
			MapGenericMask(&genericAccessRights, &mapping);
			if (AccessCheck(pSD, g_globalSid.currentUserImpersonatedToken, genericAccessRights, &mapping, &privileges,
							&privilegesLength, &grantedAccess, &result)
				&& result) {
				data |= WRITE_USER_FLAG;
			}
			privilegesLength = sizeof(privileges);
			genericAccessRights = GENERIC_EXECUTE;
			MapGenericMask(&genericAccessRights, &mapping);
			if (AccessCheck(pSD, g_globalSid.currentUserImpersonatedToken, genericAccessRights, &mapping, &privileges,
							&privilegesLength, &grantedAccess, &result)
				&& result) {
				data |= EXEC_USER_FLAG;
			}
		}
		// fallback to GetEffectiveRightsFromAcl
		else if (GetEffectiveRightsFromAclW(pDacl, &g_globalSid.currentUserTrusteeW, &access_mask) == ERROR_SUCCESS) {
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

		if (GetEffectiveRightsFromAclW(pDacl, &g_globalSid.worldTrusteeW, &access_mask) == ERROR_SUCCESS) {
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
	}
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
	if (stat(path.c_str(), &infos) == 0) {
		return (infos.st_mode & mode) == mode;
	}
#endif
	return false;
}

bool FileSystem::is_absolute(const std::string &path) {

	if (path.empty()) {
		return false;
	}

#ifdef OS_WINDOWS
	std::string native_path = FileSystem::native_path(path);

	if (native_path.size() >= 3) {
		if (isalpha(native_path[0]) && native_path[1] == ':' && native_path[2] == FileSystem::SEPARATOR) {
			return true;
		}
	}

	for (size_t i = 0; i < std::min(size_t(3), path.size()); ++i) {
		if (native_path[i] != FileSystem::SEPARATOR) {
			return false;
		}
	}

	return true;
#else
	return path[0] == FileSystem::SEPARATOR;
#endif
}

bool FileSystem::is_clean(const std::string &path) {

	int dots = 0;
	bool dotok = true;
	bool slashok = true;

	for (std::string::const_iterator iter = path.cbegin(); iter != path.cend(); ++iter) {
		if (*iter == FileSystem::SEPARATOR) {
			if (dots == 1 || dots == 2) {
				return false;
			}
			if (!slashok) {
				return false;
			}
			dots = 0;
			dotok = true;
			slashok = false;
		}
		else if (dotok) {
			slashok = true;
			if (*iter == '.') {
				dots++;
				if (dots > 2) {
					dotok = false;
				}
			}
			else {
				dots = 0;
				dotok = false;
			}
		}
	}

	return (dots != 1 && dots != 2);
}

bool FileSystem::is_root(const std::string &path) {
#ifdef OS_WINDOWS
	std::string native_path = FileSystem::native_path(path);

	if (native_path.size() == 3 && isalpha(native_path[0]) && native_path[1] == ':'
		&& native_path[2] == FileSystem::SEPARATOR) {
		return true;
	}

	if (native_path == std::string(1, FileSystem::SEPARATOR) || native_path == std::string(2, FileSystem::SEPARATOR)
		|| native_path == std::string(3, FileSystem::SEPARATOR)) {
		return true;
	}

	return false;
#else
	return path == std::string({FileSystem::SEPARATOR});
#endif
}

bool FileSystem::is_file(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	DWORD infos = GetFileAttributesW(windows_path.c_str());
	if (infos != INVALID_FILE_ATTRIBUTES) {
		return infos & FILE_ATTRIBUTE_NORMAL;
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return S_ISREG(infos.st_mode);
	}
#endif
	return false;
}

bool FileSystem::is_directory(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	DWORD infos = GetFileAttributesW(windows_path.c_str());
	if (infos != INVALID_FILE_ATTRIBUTES) {
		return infos & FILE_ATTRIBUTE_DIRECTORY;
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return S_ISDIR(infos.st_mode);
	}
#endif
	return false;
}

bool FileSystem::is_symlink(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	DWORD infos = GetFileAttributesW(windows_path.c_str());
	if (infos != INVALID_FILE_ATTRIBUTES) {
		return infos & FILE_ATTRIBUTE_REPARSE_POINT;
	}
#else
	struct stat infos;
	if (lstat(path.c_str(), &infos) == 0) {
		return S_ISLNK(infos.st_mode);
	}
#endif
	return false;
}

bool FileSystem::is_bundle(const std::string &path) {
#ifdef OS_MAC
	return /// \todo OSX
#else
	return false;
#endif
}

bool FileSystem::is_hidden(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	DWORD infos = GetFileAttributesW(windows_path.c_str());

	if (infos != INVALID_FILE_ATTRIBUTES) {
		return infos & FILE_ATTRIBUTE_HIDDEN;
	}

	return false;
#else
	std::string file_name = clean_path(path);
	auto pos = file_name.rfind(FileSystem::SEPARATOR);

	if (pos != std::string::npos) {
		file_name = file_name.substr(pos + 1);
	}

	return file_name[0] == '.';
#endif
}

size_t FileSystem::size_of(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	HANDLE hFile = CreateFileW(windows_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD size = GetFileSize(hFile, nullptr);
		CloseHandle(hFile);
		return static_cast<size_t>(size);
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return static_cast<size_t>(infos.st_size);
	}
#endif
	return 0;
}

time_t FileSystem::birth_time(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	HANDLE hFile = CreateFileW(windows_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		FILETIME time;
		if (GetFileTime(hFile, &time, nullptr, nullptr)) {
			CloseHandle(hFile);
			return FileTimeToTimestamp(time);
		}
		CloseHandle(hFile);
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return infos.st_ctime;
	}
#endif
	return 0;
}

time_t FileSystem::last_read(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	HANDLE hFile = CreateFileW(windows_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		FILETIME time;
		if (GetFileTime(hFile, nullptr, &time, nullptr)) {
			CloseHandle(hFile);
			return FileTimeToTimestamp(time);
		}
		CloseHandle(hFile);
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return infos.st_atime;
	}
#endif
	return 0;
}

time_t FileSystem::last_modified(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	HANDLE hFile = CreateFileW(windows_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		FILETIME time;
		if (GetFileTime(hFile, nullptr, nullptr, &time)) {
			CloseHandle(hFile);
			return FileTimeToTimestamp(time);
		}
		CloseHandle(hFile);
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return infos.st_mtime;
	}
#endif
	return 0;
}

std::string FileSystem::owner(const std::string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	std::wstring windows_path = string_to_windows_path(path);
	if (GetNamedSecurityInfoW(windows_path.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, 0, 0, 0, &pSD)
		== ERROR_SUCCESS) {
		DWORD lowner = 0;
		DWORD ldomain = 0;
		SID_NAME_USE use = SidTypeUnknown;
		LookupAccountSidW(nullptr, pOwner, nullptr, &lowner, nullptr, &ldomain, &use);
		std::wstring owner(lowner, L'\0');
		std::wstring domain(ldomain, L'\0');
		if (LookupAccountSidW(nullptr, pOwner, owner.data(), &lowner, domain.data(), &ldomain, &use)) {
			return windows_path_to_string(owner);
		}
	}
#else
	if (struct passwd *pw = getpwuid(FileSystem::owner_id(path))) {
		return pw->pw_name;
	}
#endif
	return {};
}

std::string FileSystem::group(const std::string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	std::wstring windows_path = string_to_windows_path(path);
	if (GetNamedSecurityInfoW(windows_path.c_str(), SE_FILE_OBJECT, GROUP_SECURITY_INFORMATION, 0, &pOwner, 0, 0, &pSD)
		== ERROR_SUCCESS) {
		DWORD lowner = 0;
		DWORD ldomain = 0;
		SID_NAME_USE use = SidTypeUnknown;
		LookupAccountSidW(nullptr, pOwner, nullptr, &lowner, nullptr, &ldomain, &use);
		std::wstring owner(lowner, L'\0');
		std::wstring domain(ldomain, L'\0');
		if (LookupAccountSidW(nullptr, pOwner, owner.data(), &lowner, domain.data(), &ldomain, &use)) {
			return windows_path_to_string(owner);
		}
	}
#else
	if (struct group *gr = getgrgid(FileSystem::group_id(path))) {
		return gr->gr_name;
	}
#endif
	return {};
}

uid_t FileSystem::owner_id(const std::string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	std::wstring windows_path = string_to_windows_path(path);
	if (GetNamedSecurityInfoW(windows_path.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, 0, 0, 0, &pSD)
		== ERROR_SUCCESS) {
		return reinterpret_cast<uid_t>(pOwner);
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return infos.st_uid;
	}
#endif
	return 0;
}

gid_t FileSystem::group_id(const std::string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	std::wstring windows_path = string_to_windows_path(path);
	if (GetNamedSecurityInfoW(windows_path.c_str(), SE_FILE_OBJECT, GROUP_SECURITY_INFORMATION, 0, &pOwner, 0, 0, &pSD)
		== ERROR_SUCCESS) {
		return reinterpret_cast<gid_t>(pOwner);
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return infos.st_gid;
	}
#endif
	return 0;
}

std::string FileSystem::symlink_target(const std::string &path) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	HANDLE hPath = CreateFileW(windows_path.c_str(), FILE_READ_EA,
							   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
							   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
	if (hPath != INVALID_HANDLE_VALUE) {

		DWORD retsize = 0;
		DWORD bufsize = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
		PREPARSE_DATA_BUFFER rdb = static_cast<PREPARSE_DATA_BUFFER>(malloc(bufsize));

		BOOL bSuccess = DeviceIoControl(hPath, FSCTL_GET_REPARSE_POINT, nullptr, 0, rdb, bufsize, &retsize, nullptr);
		CloseHandle(hPath);

		if (bSuccess) {
			char target_path[PATH_LENGTH * 4];
			if (rdb->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
				size_t length = rdb->MountPointReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
				size_t offset = rdb->MountPointReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
				const PWCHAR PathBuffer = &rdb->MountPointReparseBuffer.PathBuffer[offset];
				if (auto len = WideCharToMultiByte(CP_UTF8, 0, PathBuffer, length, target_path, sizeof(target_path),
												   nullptr, nullptr)) {
					free(rdb);
					return std::string(target_path, static_cast<size_t>(len));
				}
			}
			else if (rdb->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
				int length = rdb->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
				int offset = rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
				const PWCHAR PathBuffer = &rdb->SymbolicLinkReparseBuffer.PathBuffer[offset];
				if (auto len = WideCharToMultiByte(CP_UTF8, 0, PathBuffer, length, target_path, sizeof(target_path),
												   nullptr, nullptr)) {
					free(rdb);
					return std::string(target_path, static_cast<size_t>(len));
				}
			}
		}

		free(rdb);
	}
#else
	char target_path[PATH_LENGTH];
	if (ssize_t len = readlink(path.c_str(), target_path, sizeof(target_path))) {
		return std::string(target_path, static_cast<size_t>(len));
	}
#endif
	return path;
}

bool FileSystem::is_equal_path(const std::string &path1, const std::string &path2) {
#ifdef OS_WINDOWS
	return !utf8_compare_case_insensitive(path1, path2);
#else
	return path1 == path2;
#endif
}

bool FileSystem::is_sub_path(const std::string &sub_path, const std::string &path) {
	if (sub_path.size() < path.size()) {
		return false;
	}
#ifdef OS_WINDOWS
	if (utf8_compare_substring_case_insensitive(sub_path, path, utf8_code_point_count(path))) {
		return false;
	}
#else
	if (utf8_compare_substring(sub_path, path, utf8_code_point_count(path))) {
		return false;
	}
#endif
	return path.size() == sub_path.size() || path.back() == SEPARATOR || sub_path[path.size()] == SEPARATOR;
}

#ifdef OS_WINDOWS
std::wstring mint::string_to_windows_path(const std::string &str) {

	wchar_t buffer[FileSystem::PATH_LENGTH];

	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, static_cast<int>(std::size(buffer)))) {
		return buffer;
	}

	return {};
}
#endif

#ifdef OS_WINDOWS
std::string mint::windows_path_to_string(const std::wstring &path) {

	char buffer[FileSystem::PATH_LENGTH * 4];

	if (WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, buffer, static_cast<int>(sizeof buffer), nullptr, nullptr)) {
		return buffer;
	}

	return {};
}
#endif

FILE *mint::open_file(const char *path, const char *mode) {
#ifdef OS_WINDOWS
	std::wstring windows_path = string_to_windows_path(path);
	std::wstring mode_str = string_to_windows_path(mode);
	return _wfopen(windows_path.c_str(), mode_str.c_str());
#else
	return fopen(path, mode);
#endif
}
