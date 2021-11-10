#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/filesystem.h"

#include <climits>
#include <string>
#include <thread>
#ifdef OS_WINDOWS
#include <sstream>
#include <process.h>
#include <TlHelp32.h>
#include <corecrt_wstring.h>
#include "NtProcessInfo.h"
using handle_data_t = std::remove_pointer<HANDLE>::type;
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/resource.h>
#endif

using namespace std;
using namespace mint;

#ifdef OS_WINDOWS
wstring utf8_to_windows(const string &str) {

	int length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	wchar_t *buffer = static_cast<wchar_t *>(alloca(length * sizeof(wchar_t)));

	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, length)) {
		return buffer;
	}

	return wstring(str.begin(), str.end());
}

string windows_to_utf8(const wstring &str) {

	int length = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
	char *buffer = static_cast<char *>(alloca(length * sizeof(char)));

	if (WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buffer, length, nullptr, nullptr)) {
		return buffer;
	}

	return string(str.begin(), str.end());
}
#endif

MINT_FUNCTION(mint_process_list, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_iterator();

#ifdef OS_WINDOWS
	PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnap != INVALID_HANDLE_VALUE) {

		for (BOOL found = Process32First(hSnap, &pe); found; found = Process32Next(hSnap, &pe)) {
			iterator_insert(result.data<Iterator>(), create_number(pe.th32ProcessID));
		}

		CloseHandle(hSnap);
	}
#else
	if (DIR *proc = opendir("/proc/")) {

		while (dirent *process = readdir(proc)) {

			char *error = nullptr;
			pid_t pid = static_cast<pid_t>(strtol(process->d_name, &error, 10));

			if (!*error) {
				iterator_insert(result.data<Iterator>(), create_number(pid));
			}
		}

		closedir(proc);
	}
#endif

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_process_exec, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	string command = to_string(helper.popParameter());

	helper.returnValue(create_number(system(command.data())));
}

MINT_FUNCTION(mint_process_get_handle, 1, cursor) {
#ifdef OS_WINDOWS

	FunctionHelper helper(cursor, 1);

	DWORD procId = static_cast<DWORD>(to_number(cursor, helper.popParameter()));
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, procId);

	if (handle == INVALID_HANDLE_VALUE) {
		handle = OpenProcess(STANDARD_RIGHTS_REQUIRED, TRUE, procId);
	}

	helper.returnValue(create_object(handle));
#else
	((void)cursor);
#endif
}

MINT_FUNCTION(mint_process_get_pid, 1, cursor) {
#ifdef OS_WINDOWS

	FunctionHelper helper(cursor, 1);

	WeakReference handle = move(helper.popParameter());

	if (handle.data()->format != Data::fmt_none) {
		helper.returnValue(create_number(GetProcessId(handle.data<LibObject<handle_data_t>>()->impl)));
	}
#else
	((void)cursor);
#endif
}

MINT_FUNCTION(mint_process_close_handle, 1, cursor) {
#ifdef OS_WINDOWS

	FunctionHelper helper(cursor, 1);

	WeakReference handle = move(helper.popParameter());

	if (handle.data()->format != Data::fmt_none) {
		CloseHandle(handle.data<LibObject<handle_data_t>>()->impl);
	}
#else
	((void)cursor);
#endif
}

MINT_FUNCTION(mint_process_start, 5, cursor) {

	FunctionHelper helper(cursor, 5);

	WeakReference pipes = move(helper.popParameter());
	WeakReference environement = move(helper.popParameter());
	WeakReference workingDirectory = move(helper.popParameter());
	WeakReference arguments = move(helper.popParameter());
	WeakReference process = move(helper.popParameter());

#ifdef OS_WINDOWS

	wstringstream command;
	wchar_t *working_directory = nullptr;
	wchar_t **process_environement = nullptr;
	DWORD dwCreationFlags;
	STARTUPINFOW startup_info;
	SECURITY_ATTRIBUTES attributes;
	PROCESS_INFORMATION process_info;

	dwCreationFlags = (GetConsoleWindow() ? 0 : CREATE_NO_WINDOW);
	ZeroMemory(&attributes, sizeof(attributes));
	attributes.nLength = sizeof(attributes);
	ZeroMemory(&startup_info, sizeof(startup_info));
	startup_info.cb = sizeof(startup_info);

	auto escape = [](wstring &&arg) -> wstring && {
		if (arg.empty()) {
			arg = L"\"\"";
		}
		else if ((*arg.begin() != '"') && (*arg.rbegin() != '"') && (arg.find(' ') != wstring::npos)) {
			arg = L"\"" + arg + L"\"";
		}
		return move(arg);
	};

	command << escape(string_to_windows_path(FileSystem::nativePath(to_string(process))));

	for (Array::values_type::value_type &argv : to_array(arguments)) {
		command << L" " << escape(utf8_to_windows(to_string(array_get_item(argv))));
	}

	if (workingDirectory.data()->format != Data::fmt_none) {
		working_directory = _wcsdup(utf8_to_windows(to_string(workingDirectory)).c_str());
	}

	if (environement.data()->format != Data::fmt_none) {
		process_environement = new wchar_t *[environement.data<Hash>()->values.size() + 1];
		dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
		size_t var_pos = 0;
		for (auto &var : environement.data<Hash>()->values) {
			wstring name = utf8_to_windows(to_string(hash_get_key(var)));
			wstring value = utf8_to_windows(to_string(hash_get_value(var)));
			wchar_t *buffer = new wchar_t[name.size() + value.size() + 2];
			wsprintfW(buffer, L"%ls=%ls", name.c_str(), value.c_str());
			process_environement[var_pos++] = buffer;
		}
		process_environement[var_pos] = nullptr;
	}

	if (pipes.data()->format != Data::fmt_none) {

		PHANDLE pipe_handles[3][2];

		attributes.bInheritHandle = true;

		for (int fd = 0; fd < 3; ++fd) {
			WeakReference pipe = array_get_item(pipes.data<Array>(), fd);
			for (int h = 0; h < 2; ++h) {
				PHANDLE handle = &array_get_item(pipe.data<Array>(), h).data<LibObject<handle_data_t>>()->impl;
				pipe_handles[fd][h] = handle;
				CloseHandle(*handle);
			}
			CreatePipe(pipe_handles[fd][0], pipe_handles[fd][1], &attributes, 0);
		}

		if (SetHandleInformation(*pipe_handles[0][0], HANDLE_FLAG_INHERIT, 0)) {
			startup_info.hStdInput = *pipe_handles[0][0];
			startup_info.dwFlags |= STARTF_USESTDHANDLES;
		}

		if (SetHandleInformation(*pipe_handles[1][0], HANDLE_FLAG_INHERIT, 0)) {
			startup_info.hStdOutput = *pipe_handles[1][0];
			startup_info.dwFlags |= STARTF_USESTDHANDLES;
		}

		if (SetHandleInformation(*pipe_handles[2][0], HANDLE_FLAG_INHERIT, 0)) {
			startup_info.hStdError = *pipe_handles[2][0];
			startup_info.dwFlags |= STARTF_USESTDHANDLES;
		}
	}

	wstring command_line = command.str();

	if (CreateProcessW(nullptr, const_cast<wchar_t *>(command_line.data()), nullptr, nullptr, false, dwCreationFlags, process_environement, working_directory, &startup_info, &process_info)) {
		helper.returnValue(create_object(process_info.hProcess));
		CloseHandle(process_info.hThread);
	}
#else
	pid_t pid = fork();

	if (pid == 0) {

		vector<char *> args;

		args.push_back(strdup(to_string(process).data()));

		for (Array::values_type::value_type &argv : to_array(arguments)) {
			args.push_back(strdup(to_string(array_get_item(argv)).data()));
		}

		args.push_back(nullptr);

		if (workingDirectory.data()->format != Data::fmt_none) {
			chdir(to_string(workingDirectory).data());
		}

		if (pipes.data()->format != Data::fmt_none) {

			WeakReference stdin_pipe = array_get_item(pipes.data<Array>(), STDIN_FILENO);
			WeakReference stdout_pipe = array_get_item(pipes.data<Array>(), STDOUT_FILENO);
			WeakReference stderr_pipe = array_get_item(pipes.data<Array>(), STDERR_FILENO);

			dup2(static_cast<int>(to_integer(cursor, array_get_item(stdin_pipe.data<Array>(), 0))), STDIN_FILENO);
			dup2(static_cast<int>(to_integer(cursor, array_get_item(stdout_pipe.data<Array>(), 1))), STDOUT_FILENO);
			dup2(static_cast<int>(to_integer(cursor, array_get_item(stderr_pipe.data<Array>(), 1))), STDERR_FILENO);
		}
		else {
			struct rlimit limit;

			getrlimit(RLIMIT_NOFILE, &limit);

			for (int fd = 3; fd < static_cast<int>(limit.rlim_cur); ++fd) {
				close(fd);
			}
		}

		if (environement.data()->format != Data::fmt_none) {

			vector<char *> envp;

			for (auto &var : environement.data<Hash>()->values) {
				string name = to_string(hash_get_key(var));
				string value = to_string(hash_get_value(var));
				char *buffer = new char[name.size() + value.size() + 2];
				sprintf(buffer, "%s=%s", name.c_str(), value.c_str());
				envp.push_back(buffer);
			}

			envp.push_back(nullptr);

			execve(args.front(), args.data(), envp.data());
		}
		else {
			execve(args.front(), args.data(), environ);
		}

		exit(EXIT_FAILURE);
	}

	if (pid != -1) {
		helper.returnValue(create_number(pid));
	}
#endif
}

MINT_FUNCTION(mint_process_getcmdline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;

	if (LPWSTR szCmdLine = GetNtProcessCommandLine(handle)) {

		WeakReference results = create_iterator();
		WeakReference args = create_array();

		int argc = 0;
		wchar_t **argv = CommandLineToArgvW(szCmdLine, &argc);

		for (int argn = 0; argn < argc; ++argn) {
			if (results.data<Iterator>()->ctx.empty()) {
				iterator_insert(results.data<Iterator>(), create_string(windows_to_utf8(argv[argn])));
			}
			else {
				array_append(args.data<Array>(), create_string(windows_to_utf8(argv[argn])));
			}
		}

		iterator_insert(results.data<Iterator>(), move(args));

		helper.returnValue(move(results));
	}
#else
	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	char cmdline_path[FileSystem::path_length];
	WeakReference results = create_iterator();
	WeakReference args = create_array();

	snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);
	FILE *cmdline = open_file(cmdline_path, "r");

	char *buffer = nullptr;
	size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, cmdline) != -1) {
		if (results.data<Iterator>()->ctx.empty()) {
			iterator_insert(results.data<Iterator>(), create_string(string(buffer, buffer_length)));
		}
		else {
			array_append(args.data<Array>(), create_string(string(buffer, buffer_length)));
		}
	}

	iterator_insert(results.data<Iterator>(), move(args));
	fclose(cmdline);
	free(buffer);

	helper.returnValue(move(results));
#endif
}

MINT_FUNCTION(mint_process_getcwd, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;
	DWORD dwLength = GetNtProcessCurrentDirectory(handle, NULL, 0);
	LPWSTR szCurrentDirectoryPath = static_cast<LPWSTR>(alloca(dwLength * sizeof (WCHAR)));

	if (GetNtProcessCurrentDirectory(handle, szCurrentDirectoryPath, dwLength)) {
		helper.returnValue(create_string(windows_path_to_string(szCurrentDirectoryPath)));
	}
#else
	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	char exe_path[FileSystem::path_length];
	char proc_path[FileSystem::path_length];
	snprintf(exe_path, sizeof(exe_path), "/proc/%d/exe", pid);
	ssize_t count = readlink(exe_path, proc_path, sizeof(proc_path));

	if (count > 0) {
		helper.returnValue(create_string(proc_path));
	}
#endif
}

MINT_FUNCTION(mint_process_getenv, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;

	if (LPWCH szEnvironment = GetNtProcessEnvironmentStrings(handle)) {
		
		WeakReference results = create_hash();
		LPCWSTR buffer = szEnvironment;

		while (*buffer) {
			LPCWSTR cptr = wcschr(buffer, L'=');
			hash_insert(results.data<Hash>(), create_string(windows_to_utf8(wstring(buffer, cptr))), create_string(windows_to_utf8(wstring(cptr + 1))));
			buffer += lstrlenW(buffer) + 1;
		}

		FreeEnvironmentStringsW(szEnvironment);
		helper.returnValue(move(results));
	}
#else
	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	char environ_path[FileSystem::path_length];
	WeakReference results = create_hash();

	snprintf(environ_path, sizeof(environ_path), "/proc/%d/environ", pid);
	FILE *environ = open_file(environ_path, "r");

	char *buffer = nullptr;
	size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, environ) != -1) {
		char *cptr = strchr(buffer, '=');
		hash_insert(results.data<Hash>(), create_string(string(buffer, cptr)), create_string(cptr + 1));
	}

	fclose(environ);
	free(buffer);

	helper.returnValue(move(results));
#endif
}

MINT_FUNCTION(mint_process_getpid, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	helper.returnValue(create_number(GetCurrentProcessId()));
#else
	helper.returnValue(create_number(getpid()));
#endif
}

MINT_FUNCTION(mint_process_waitpid, 4, cursor) {

	FunctionHelper helper(cursor, 4);

	WeakReference exit_code = move(helper.popParameter());
	WeakReference exit_status = move(helper.popParameter());
	bool wait_for_finished = to_boolean(cursor, helper.popParameter());

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;

	bool finished = false;

	if (WaitForSingleObject(handle, wait_for_finished ? INFINITE : 0) == WAIT_OBJECT_0) {

		DWORD value = 0;

		if (GetExitCodeProcess(handle, &value)) {
			exit_status.data<Boolean>()->value = (value == 0xDEAD || (value >= 0x80000000 && value < 0xD0000000));
			exit_code.data<Number>()->value = value;
		}

		CloseHandle(handle);
		finished = true;
	}

	helper.returnValue(create_boolean(finished));
#else
	int pid = static_cast<int>(to_integer(cursor, helper.popParameter()));

	int status = 0;
	int options = 0;
	bool finished = false;

	if (!wait_for_finished) {
		options |= WNOHANG;
	}

	do {
		if (waitpid(pid, &status, options) == pid) {
			exit_status.data<Boolean>()->value = WIFEXITED(status);
			exit_code.data<Number>()->value = WEXITSTATUS(status);
			finished = true;
		}
	}
	while (!finished && wait_for_finished);

	helper.returnValue(create_boolean(finished));
#endif
}

MINT_FUNCTION(mint_process_kill, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;

	helper.returnValue(create_boolean(TerminateProcess(handle, 0xDEAD)));
#else
	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	helper.returnValue(create_boolean(!kill(pid, SIGKILL)));
#endif
}

MINT_FUNCTION(mint_process_treminate, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;

	helper.returnValue(create_boolean(GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, GetProcessId(handle))));
#else
	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	helper.returnValue(create_boolean(!kill(pid, SIGTERM)));
#endif
}
