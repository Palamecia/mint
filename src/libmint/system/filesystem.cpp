#include "system/filesystem.h"

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
							if (CopySid(sidLen, currentUserSID, tokenSid))
								BuildTrusteeWithSidW(&currentUserTrusteeW, currentUserSID);
						}
						free(tokenBuffer);
					}
					CloseHandle(token);
				}
				token = nullptr;
				if (OpenProcessToken(hnd, TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ, &token)) {
					DuplicateToken(token, SecurityImpersonation, &currentUserImpersonatedToken);
					CloseHandle(token);
				}
				{
					// Create TRUSTEE for Everyone (World)
					SID_IDENTIFIER_AUTHORITY worldAuth = { SECURITY_WORLD_SID_AUTHORITY };
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
};

static GlobalSid g_globalSid;
#else
#define PATH_SEPARATOR ':'
#endif

#define LIBRARY_PATH_VAR "MINT_LIBRARY_PATH"

using namespace std;
using namespace mint;

#ifdef OS_UNIX
constexpr const char FileSystem::separator;
#endif

extern "C" void find_mint(void) {}

string format_module_path(const string &mint_path) {

	string path = mint_path;
	replace(path.begin(), path.end(), '.', FileSystem::separator);
	return path;
}

string get_parent_dir(const string &path) {
	return path.substr(0, path.find_last_of(FileSystem::separator));
}

FileSystem::iterator::iterator(const string &path) :
	m_data(new data(path)) {
	m_entry = m_data->first();
}

FileSystem::iterator::iterator() : m_entry(nullptr) {

}

FileSystem::iterator &FileSystem::iterator::operator++() {
	m_entry = m_data->next();
	return *this;
}

FileSystem::iterator FileSystem::iterator::operator++(int) {
	iterator other = *this;
	m_entry = m_data->next();
	return other;
}

bool FileSystem::iterator::operator ==(const iterator &other) const {
	return m_entry == other.m_entry;
}

bool FileSystem::iterator::operator !=(const iterator &other) const {
	return m_entry != other.m_entry;
}

FileSystem::iterator::value_type FileSystem::iterator::operator *() const {
	if (m_entry) {
#ifdef OS_WINDOWS
		return windows_path_to_string(m_entry->cFileName);
#else
		return m_entry->d_name;
#endif
	}
	return value_type();
}

FileSystem::iterator::data::data(const string &path) :
	m_context(nullptr),
	m_path(path) {

}

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
	if ((m_context = FindFirstFileW(string_to_windows_path(m_path + FileSystem::separator + '*').c_str(), entry.get()))) {
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

	m_homePath = homePath();
	m_currentPath = currentPath();

#ifdef OS_WINDOWS
	wchar_t dli_fname[path_length];
	GetModuleFileNameW(nullptr, dli_fname, sizeof dli_fname / sizeof(wchar_t));
	string libraryPath = get_parent_dir(get_parent_dir(windows_path_to_string(dli_fname))) + "/lib";
#else
	Dl_info dl_info;
	dladdr(reinterpret_cast<void *>(find_mint), &dl_info);
	string libraryPath = get_parent_dir(dl_info.dli_fname);
#endif
	m_libraryPath.push_back(nativePath(libraryPath));
	m_libraryPath.push_back(nativePath(libraryPath + "/mint"));

	if (const char *var = getenv(LIBRARY_PATH_VAR)) {

		string path;
		istringstream stream(var);

		while (getline(stream, path, PATH_SEPARATOR)) {
			m_libraryPath.push_back(path);
		}
	}
}

FileSystem &FileSystem::instance() {
	static FileSystem g_instance;
	return g_instance;
}

string FileSystem::homePath() const {

#ifdef OS_WINDOWS
	HANDLE hnd = GetCurrentProcess();
	HANDLE token = 0;

	if (OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
		DWORD dwBufferSize = 0;
		if (!GetUserProfileDirectoryW(token, NULL, &dwBufferSize) && dwBufferSize != 0) {
			wchar_t *userDirectory = static_cast<wchar_t *>(alloca(dwBufferSize * sizeof(wchar_t)));
			if (GetUserProfileDirectoryW(token, userDirectory, &dwBufferSize)) {
				m_homePath = windows_path_to_string(userDirectory);
			}
		}

		CloseHandle(token);
	}
#else
	if (struct passwd *pw = getpwuid(getuid())) {
		m_homePath = pw->pw_dir;
	}
#endif

	return m_homePath;
}

string FileSystem::currentPath() const {

#ifdef OS_WINDOWS
	wchar_t currentName[path_length];
	DWORD size = GetCurrentDirectoryW(path_length, currentName);

	if (size != 0) {
		if (size > path_length) {
			wchar_t *newCurrentName = static_cast<wchar_t *>(alloca(size * sizeof(wchar_t)));
			if (GetCurrentDirectoryW(path_length, newCurrentName) != 0) {
				m_currentPath = windows_path_to_string(newCurrentName);
			}
		}
		else {
			m_currentPath = windows_path_to_string(currentName);
		}
	}
#else
	char path[path_length];

	if (getcwd(path, sizeof(path))) {
		m_currentPath = path;
	}
#endif

	return m_currentPath;
}

string FileSystem::absolutePath(const string &path) const {

#ifdef OS_WINDOWS
	wchar_t resolved_path[path_length];
	if (GetFullPathNameW(string_to_windows_path(path).c_str(), path_length, resolved_path, nullptr)) {
		return windows_path_to_string(resolved_path);
	}
#else
	char resolved_path[path_length];
	if (realpath(path.c_str(), resolved_path)) {
		return resolved_path;
	}
#endif

	if (isRoot(path)) {
		return cleanPath(path);
	}

	if (path[0] == '~') {
		return cleanPath(homePath() + (path.c_str() + 1));
	}

	return cleanPath(currentPath() + FileSystem::separator + path);
}

string FileSystem::relativePath(const string &root, const string &path) const {

	string root_path = absolutePath(root);
	auto root_start = root_path.find(FileSystem::separator);
	string root_directory = root_path.substr(0, root_start);

	string other_path = absolutePath(path);
	auto other_start = other_path.find(FileSystem::separator);
	string other_directory = other_path.substr(0, other_start);

	while ((root_directory == other_directory) && (root_start != string::npos) && (other_start != string::npos)) {

		auto root_stop = root_path.find(FileSystem::separator, root_start + 1);
		root_directory = root_path.substr(root_start, root_stop - root_start);

		auto other_stop = other_path.find(FileSystem::separator, other_start + 1);
		other_directory = other_path.substr(other_start, other_stop - other_start);

		if ((root_directory == other_directory)) {
			root_start = root_stop;
			other_start = other_stop;
		}
	}

	string relative_path;

	while (root_start != string::npos) {
		relative_path += "..";
		relative_path += FileSystem::separator;
		root_start = root_path.find(FileSystem::separator, root_start + 1);
	}

	if (other_start != string::npos) {
		relative_path += other_path.substr(other_start + 1);
	}

	return relative_path;
}

bool FileSystem::copy(const string &source, const string &target) {

	if (isDirectory(source)) {
		if (!checkFileAccess(target, FileSystem::exists)) {
			if (!createDirectory(target, true)) {
				return false;
			}
		}
		for (auto it = browse(source); it != end(); ++it) {
			if (!copy(source + FileSystem::separator + *it, target + FileSystem::separator + *it)) {
				return false;
			}
		}
	}

	if (FILE *input = open_file(source.c_str(), "rb")) {
		if (FILE *output = open_file(target.c_str(), "wb")) {
			bool success = true;
			byte block[4096];
			while (auto bytes = fread(block, sizeof(byte), sizeof(block), input)) {
				if (bytes != fwrite(block, sizeof(byte), bytes, output)) {
					success = false;
					break;
				}
			}
			fclose(output);
			fclose(input);
			return success;
		}
		fclose(input);
	}

	return false;
}

bool FileSystem::rename(const string &source, const string &target) {

	if (!copy(source, target)) {
		return false;
	}

	return remove(source);
}

bool FileSystem::remove(const string &source) {

	if (isDirectory(source)) {
		for (auto it = browse(source); it != end(); ++it) {
			if (!remove(source + FileSystem::separator + *it)) {
				return false;
			}
		}
		return removeDirectory(source, false);
	}

#ifdef OS_WINDOWS
	return _wunlink(string_to_windows_path(source).c_str()) == 0;
#else
	return unlink(source.c_str()) == 0;
#endif
}

bool FileSystem::createLink(const string &path, const string &target) {
#ifdef OS_WINDOWS
	DWORD falgs = 0;
	if (isDirectory(path)) {
		falgs = SYMBOLIC_LINK_FLAG_DIRECTORY;
	}
	return CreateSymbolicLinkW(string_to_windows_path(path).c_str(),
							   string_to_windows_path(target).c_str(),
							   falgs);
#else
	return symlink(path.c_str(), target.c_str()) == 0;
#endif
}

bool FileSystem::createDirectory(const string &path, bool recursive) {

#ifdef OS_WINDOWS
	if (_wmkdir(string_to_windows_path(path).c_str()) == 0) {
#else
	if (mkdir(path.c_str(), 0777) == 0) {
#endif
		return true;
	}

	if (recursive) {
		string absolute_path = absolutePath(path);
		string parent = absolute_path.substr(0, absolute_path.rfind(FileSystem::separator));
		if ((parent != absolute_path) && !checkFileAccess(parent, exists)) {
			if (createDirectory(parent, recursive)) {
				return createDirectory(path, false);
			}
		}
	}

	return false;
}

bool FileSystem::removeDirectory(const string &path, bool recursive) {

#ifdef OS_WINDOWS
	if (_wrmdir(string_to_windows_path(path).c_str()) == 0) {
#else
	if (rmdir(path.c_str()) == 0) {
#endif
		if (recursive) {
			string absolute_path = absolutePath(path);
			string parent = absolute_path.substr(0, absolute_path.rfind(FileSystem::separator));
			if (parent != absolute_path) {
				removeDirectory(parent, recursive);
			}
		}
		return true;
	}
	return false;
}

FileSystem::iterator FileSystem::browse(const string &path) {
	return iterator(absolutePath(path));
}

FileSystem::iterator FileSystem::begin() {
	return browse(systemRoot());
}

FileSystem::iterator FileSystem::end() {

	static iterator g_end;

	return g_end;
}

string FileSystem::getModulePath(const string &module) const {

	string modulePath = format_module_path(module) + ".mn";

	if (checkFileAccess(modulePath, readable)) {
		return modulePath;
	}

	for (string path : m_libraryPath) {
		string fullPath = path + FileSystem::separator + modulePath;
		if (checkFileAccess(fullPath, readable)) {
			return fullPath;
		}
	}

	return string();
}

string FileSystem::getPluginPath(const string &plugin) const {

	string pluginPath = format_module_path(plugin);
#ifdef OS_WINDOWS
	pluginPath += ".dll";
#else
	pluginPath += ".so";
#endif

	if (checkFileAccess(pluginPath, readable)) {
		return pluginPath;
	}

	for (string path : m_libraryPath) {
		string fullPath = path + FileSystem::separator + pluginPath;
		if (checkFileAccess(fullPath, readable)) {
			return fullPath;
		}
	}

	return string();
}

void FileSystem::addToPath(const string &path) {
	m_libraryPath.push_back(path);
}

string FileSystem::systemRoot() {
#ifdef OS_WINDOWS
	wchar_t root_path[path_length];
	if (GetSystemDirectoryW(root_path, path_length)) {
		return windows_path_to_string(root_path);
	}
	return string();
#else
	return {FileSystem::separator};
#endif
}

string FileSystem::cleanPath(const string &path) {

	string clean_path = nativePath(path);

	auto start = clean_path.find(FileSystem::separator);
	while (start != string::npos) {
		auto stop = clean_path.find(FileSystem::separator, start + 1);
		auto len = (stop == string::npos) ? clean_path.size() - start : stop - start - 1;
		string directory = clean_path.substr(start + 1, (stop == string::npos) ? stop : len);
		if (directory == ".") {
			clean_path.replace(start, len + 1, "");
			start = clean_path.find(FileSystem::separator, start);
		}
		else if (directory == "..") {
			auto before = clean_path.rfind(FileSystem::separator, start - 1);
			if (before != string::npos) {
				len = (stop == string::npos) ? clean_path.size() - before : stop - before - 1;
				clean_path.replace(before, len + 1, "");
				start = clean_path.find(FileSystem::separator, before);
			}
			else {
				directory = clean_path.substr(0, start);
				if ((directory != ".") && (directory != "..")) {
					clean_path.replace(0, stop + 1, "");
					start = clean_path.find(FileSystem::separator);
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

string FileSystem::nativePath(const string &path) {

	string native_path = path;

	for (const char sep : {'/', '\\'}) {
		if (sep != FileSystem::separator) {
			replace(native_path.begin(), native_path.end(), sep, FileSystem::separator);
		}
	}

#ifdef OS_WINDOWS
	if ((native_path.c_str()[0] == FileSystem::separator) && (native_path.c_str()[1] != FileSystem::separator)) {
		native_path = systemRoot() +( native_path.c_str() + 1);
	}
#endif

	return native_path;
}

bool FileSystem::checkFileAccess(const string &path, AccessFlags flags) {
	int right = 0;
#ifdef OS_WINDOWS
	if (flags & exists) {
		right |= 0x00;
	}
	if (flags & readable) {
		right |= 0x04;
	}
	if (flags & writable) {
		right |= 0x02;
	}
	if (flags & executable) {
		right |= 0x04;
	}
	return _waccess(string_to_windows_path(path).c_str(), right) == 0;
#else
	if (flags & exists) {
		right |= F_OK;
	}
	if (flags & readable) {
		right |= R_OK;
	}
	if (flags & writable) {
		right |= W_OK;
	}
	if (flags & executable) {
		right |= X_OK;
	}
	return access(path.c_str(), right) == 0;
#endif
}

bool FileSystem::checkFilePermissions(const string &path, Permissions permissions) {
#ifdef OS_WINDOWS

	PSID pOwner = 0;
	PSID pGroup = 0;
	PACL pDacl;
	Permissions data = 0;
	PSECURITY_DESCRIPTOR pSD;

	if (GetNamedSecurityInfoW(string_to_windows_path(path).c_str(), SE_FILE_OBJECT,
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
		&pOwner, &pGroup, &pDacl, 0, &pSD) == ERROR_SUCCESS) {

		ACCESS_MASK access_mask;
		TRUSTEE_W trustee;
		enum { ReadMask = 0x00000001, WriteMask = 0x00000002, ExecMask = 0x00000020 };

		if (g_globalSid.currentUserImpersonatedToken) {
			GENERIC_MAPPING mapping = { FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS };
			PRIVILEGE_SET privileges;
			DWORD grantedAccess;
			BOOL result;
			DWORD genericAccessRights = GENERIC_READ;
			MapGenericMask(&genericAccessRights, &mapping);
			DWORD privilegesLength = sizeof(privileges);
			if (AccessCheck(pSD, g_globalSid.currentUserImpersonatedToken, genericAccessRights,
				&mapping, &privileges, &privilegesLength, &grantedAccess, &result) && result) {
				data |= read_user;
			}
			privilegesLength = sizeof(privileges);
			genericAccessRights = GENERIC_WRITE;
			MapGenericMask(&genericAccessRights, &mapping);
			if (AccessCheck(pSD, g_globalSid.currentUserImpersonatedToken, genericAccessRights,
				&mapping, &privileges, &privilegesLength, &grantedAccess, &result) && result) {
				data |= write_user;
			}
			privilegesLength = sizeof(privileges);
			genericAccessRights = GENERIC_EXECUTE;
			MapGenericMask(&genericAccessRights, &mapping);
			if (AccessCheck(pSD, g_globalSid.currentUserImpersonatedToken, genericAccessRights,
				&mapping, &privileges, &privilegesLength, &grantedAccess, &result) && result) {
				data |= exec_user;
			}
		}
		// fallback to GetEffectiveRightsFromAcl
		else if (GetEffectiveRightsFromAclW(pDacl, &g_globalSid.currentUserTrusteeW, &access_mask) == ERROR_SUCCESS) {
			if (access_mask & ReadMask)
				data |= read_user;
			if (access_mask & WriteMask)
				data |= write_user;
			if (access_mask & ExecMask)
				data |= exec_user;
		}

		BuildTrusteeWithSidW(&trustee, pOwner);
		if (GetEffectiveRightsFromAclW(pDacl, &trustee, &access_mask) == ERROR_SUCCESS) {
			if (access_mask & ReadMask)
				data |= read_owner;
			if (access_mask & WriteMask)
				data |= write_owner;
			if (access_mask & ExecMask)
				data |= exec_owner;
		}

		BuildTrusteeWithSidW(&trustee, pGroup);
		if (GetEffectiveRightsFromAclW(pDacl, &trustee, &access_mask) == ERROR_SUCCESS) {
			if (access_mask & ReadMask)
				data |= read_group;
			if (access_mask & WriteMask)
				data |= write_group;
			if (access_mask & ExecMask)
				data |= exec_group;
		}

		if (GetEffectiveRightsFromAclW(pDacl, &g_globalSid.worldTrusteeW, &access_mask) == ERROR_SUCCESS) {
			if (access_mask & ReadMask)
				data |= read_other;
			if (access_mask & WriteMask)
				data |= write_other;
			if (access_mask & ExecMask)
				data |= exec_other;
		}

		LocalFree(pSD);
		return (data & permissions) == permissions;
	}
#else
	mode_t mode = 0;

	if (permissions & read_owner) {
		mode |= S_IRUSR;
	}
	if (permissions & write_owner) {
		mode |= S_IWUSR;
	}
	if (permissions & exec_owner) {
		mode |= S_IXUSR;
	}
	if (permissions & read_user) {
		mode |= S_IRUSR;
	}
	if (permissions & write_user) {
		mode |= S_IWUSR;
	}
	if (permissions & exec_user) {
		mode |= S_IXUSR;
	}
	if (permissions & read_group) {
		mode |= S_IRGRP;
	}
	if (permissions & write_group) {
		mode |= S_IWGRP;
	}
	if (permissions & exec_group) {
		mode |= S_IXGRP;
	}
	if (permissions & read_other) {
		mode |= S_IROTH;
	}
	if (permissions & write_other) {
		mode |= S_IWOTH;
	}
	if (permissions & exec_other) {
		mode |= S_IXOTH;
	}
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return (infos.st_mode & mode) == mode;
	}
#endif
	return false;
}

bool FileSystem::isRoot(const string &path) {
#ifdef OS_WINDOWS
	string native_path = nativePath(path);
	if (native_path.size() == 3
		&& isalpha(native_path[0])
		&& native_path[1] == ':'
		&& native_path[2] == FileSystem::separator) {
		return true;
	}
	if (native_path == string(1, FileSystem::separator)
		|| native_path == string(2, FileSystem::separator)
		|| native_path == string(3, FileSystem::separator)) {
		return true;
	}
#else
	return path == string({FileSystem::separator});
#endif
	return false;
}

bool FileSystem::isFile(const string &path) {
#ifdef OS_WINDOWS
	DWORD infos = GetFileAttributesW(string_to_windows_path(path).c_str());
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

bool FileSystem::isDirectory(const string &path) {
#ifdef OS_WINDOWS
	DWORD infos = GetFileAttributesW(string_to_windows_path(path).c_str());
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

bool FileSystem::isSymlink(const string &path) {
#ifdef OS_WINDOWS
	DWORD infos = GetFileAttributesW(string_to_windows_path(path).c_str());
	if (infos != INVALID_FILE_ATTRIBUTES) {
		return infos & FILE_ATTRIBUTE_REPARSE_POINT;
	}
#else
	struct stat infos;
	if (stat(path.c_str(), &infos) == 0) {
		return S_ISLNK(infos.st_mode);
	}
#endif
	return false;
}

bool FileSystem::isBundle(const string &path) {
#ifdef OS_OSX
	return /// \todo OSX
#else
	return false;
#endif
}

bool FileSystem::isHidden(const string &path) {
#ifdef OS_WINDOWS
	DWORD infos = GetFileAttributesW(string_to_windows_path(path).c_str());

	if (infos != INVALID_FILE_ATTRIBUTES) {
		return infos & FILE_ATTRIBUTE_HIDDEN;
	}

	return false;
#else
	string file_name = cleanPath(path);
	auto pos = file_name.rfind(FileSystem::separator);

	if (pos != string::npos) {
		file_name = file_name.substr(pos + 1);
	}

	return file_name[0] == '.';
#endif
}

size_t FileSystem::sizeOf(const string &path) {
#ifdef OS_WINDOWS
	HANDLE hFile = CreateFileW(string_to_windows_path(path).c_str(),
							   GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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

time_t FileSystem::birthTime(const string &path) {
#ifdef OS_WINDOWS
	HANDLE hFile = CreateFileW(string_to_windows_path(path).c_str(),
							   GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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

time_t FileSystem::lastRead(const string &path) {
#ifdef OS_WINDOWS
	HANDLE hFile = CreateFileW(string_to_windows_path(path).c_str(),
							   GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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

time_t FileSystem::lastModified(const string &path) {
#ifdef OS_WINDOWS
	HANDLE hFile = CreateFileW(string_to_windows_path(path).c_str(),
							   GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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

string FileSystem::owner(const string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	if (GetNamedSecurityInfoW(string_to_windows_path(path).c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, 0, 0, 0, &pSD) == ERROR_SUCCESS) {
		DWORD lowner = 0;
		DWORD ldomain = 0;
		SID_NAME_USE use = SidTypeUnknown;
		LookupAccountSidW(nullptr, pOwner, nullptr, &lowner, nullptr, &ldomain, &use);
		wchar_t *owner = (wchar_t *)alloca(lowner * sizeof(wchar_t));
		wchar_t *domain = (wchar_t *)alloca(ldomain * sizeof(wchar_t));
		if (LookupAccountSidW(nullptr, pOwner, owner, &lowner, domain, &ldomain, &use)) {
			return windows_path_to_string(owner);
		}
	}
#else
	if (struct passwd *pw = getpwuid(FileSystem::ownerId(path))) {
		return pw->pw_name;
	}
#endif
	return string();
}

string FileSystem::group(const string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	if (GetNamedSecurityInfoW(string_to_windows_path(path).c_str(), SE_FILE_OBJECT, GROUP_SECURITY_INFORMATION, 0, &pOwner, 0, 0, &pSD) == ERROR_SUCCESS) {
		DWORD lowner = 0;
		DWORD ldomain = 0;
		SID_NAME_USE use = SidTypeUnknown;
		LookupAccountSidW(nullptr, pOwner, nullptr, &lowner, nullptr, &ldomain, &use);
		wchar_t *owner = (wchar_t *)alloca(lowner * sizeof(wchar_t));
		wchar_t *domain = (wchar_t *)alloca(ldomain * sizeof(wchar_t));
		if (LookupAccountSidW(nullptr, pOwner, owner, &lowner, domain, &ldomain, &use)) {
			return windows_path_to_string(owner);
		}
	}
#else
	if (struct group *gr = getgrgid(FileSystem::groupId(path))) {
		return gr->gr_name;
	}
#endif
	return string();
}

uid_t FileSystem::ownerId(const string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	if (GetNamedSecurityInfoW(string_to_windows_path(path).c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pOwner, 0, 0, 0, &pSD) == ERROR_SUCCESS) {
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

gid_t FileSystem::groupId(const string &path) {
#ifdef OS_WINDOWS
	PSID pOwner = 0;
	PSECURITY_DESCRIPTOR pSD;
	if (GetNamedSecurityInfoW(string_to_windows_path(path).c_str(), SE_FILE_OBJECT, GROUP_SECURITY_INFORMATION, 0, &pOwner, 0, 0, &pSD) == ERROR_SUCCESS) {
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

string FileSystem::symlinkTarget(const string &path) {
#ifdef OS_WINDOWS
	HANDLE hPath = CreateFileW(string_to_windows_path(path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (hPath != INVALID_HANDLE_VALUE) {
		DWORD len = GetFinalPathNameByHandleW(hPath, nullptr, 0, FILE_NAME_OPENED);
		wchar_t *target_path = (wchar_t *)alloca(len * sizeof(wchar_t));
		GetFinalPathNameByHandleW(hPath, target_path, len, FILE_NAME_OPENED);
		CloseHandle(hPath);
		return windows_path_to_string(target_path);
	}
#else
	char target_path[path_length];
	if (ssize_t len = readlink(path.c_str(), target_path, sizeof(target_path))) {
		return string(target_path, static_cast<size_t>(len));
	}
#endif
	return path;
}

#ifdef OS_WINDOWS
wstring mint::string_to_windows_path(const string &str) {

	wchar_t buffer[FileSystem::path_length];

	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1,
							buffer, static_cast<int>(extent<decltype(buffer)>::value))) {
		return buffer;
	}

	return wstring(str.begin(), str.end());
}
#endif

#ifdef OS_WINDOWS
string mint::windows_path_to_string(const wstring &path) {

	char buffer[FileSystem::path_length * 4];

	if (WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1,
							buffer, static_cast<int>(sizeof buffer), nullptr, nullptr)) {
		return buffer;
	}

	return string(path.begin(), path.end());
}
#endif

FILE *mint::open_file(const char *path, const char *mode) {
#ifdef OS_WINDOWS
	return _wfopen(string_to_windows_path(path).c_str(), string_to_windows_path(mode).c_str());
#else
	return fopen(path, mode);
#endif
}
