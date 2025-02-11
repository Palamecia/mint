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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/system/filesystem.h"
#include "mint/system/errno.h"

#ifdef OS_WINDOWS
#include <Windows.h>
#include <sstream>
#include <process.h>
#include <TlHelp32.h>
#include <corecrt_wstring.h>
#include "win32/NtProcessInfo.h"
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/resource.h>
#endif

#include <filesystem>
#include <string>

using namespace mint;

namespace {

#ifdef OS_WINDOWS
std::wstring utf8_to_windows(const std::string &str) {

	std::wstring buffer(MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0), L'\0');

	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.data(), buffer.length())) {
		return buffer;
	}

	return {};
}

std::string windows_to_utf8(const std::wstring &str) {

	std::string buffer(WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr), '\0');

	if (WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buffer.data(), buffer.length(), nullptr, nullptr)) {
		return buffer;
	}

	return {};
}
#endif

}

MINT_FUNCTION(mint_process_list, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_iterator();

#ifdef OS_WINDOWS
	PROCESSENTRY32 pe = {sizeof(PROCESSENTRY32)};
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnap != INVALID_HANDLE_VALUE) {

		for (BOOL found = Process32First(hSnap, &pe); found; found = Process32Next(hSnap, &pe)) {
			iterator_yield(result.data<Iterator>(), create_number(pe.th32ProcessID));
		}

		CloseHandle(hSnap);
	}
#else
	if (DIR *proc = opendir("/proc/")) {

		while (const dirent *process = readdir(proc)) {

			char *error = nullptr;
			auto pid = static_cast<pid_t>(strtol(process->d_name, &error, 10));

			if (!*error) {
				iterator_yield(result.data<Iterator>(), create_number(pid));
			}
		}

		closedir(proc);
	}
#endif

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_process_exec, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	std::string command = to_string(helper.pop_parameter());

	helper.return_value(create_number(system(command.data())));
}

MINT_FUNCTION(mint_process_get_handle, 1, cursor) {
#ifdef OS_WINDOWS

	FunctionHelper helper(cursor, 1);

	auto proc_id = static_cast<DWORD>(to_number(cursor, helper.pop_parameter()));
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, proc_id);

	if (handle == INVALID_HANDLE_VALUE) {
		handle = OpenProcess(STANDARD_RIGHTS_REQUIRED, TRUE, proc_id);
	}

	helper.return_value(create_object(handle));
#else
	((void)cursor);
#endif
}

MINT_FUNCTION(mint_process_get_pid, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference handle = std::move(helper.pop_parameter());

	if (handle.data()->format != Data::FMT_NONE) {
#ifdef OS_WINDOWS
		helper.return_value(create_number(GetProcessId(to_handle(helper.pop_parameter()))));
#else
		helper.return_value(create_number(static_cast<pid_t>(to_handle(handle))));
#endif
	}
}

MINT_FUNCTION(mint_process_close_handle, 1, cursor) {
#ifdef OS_WINDOWS

	FunctionHelper helper(cursor, 1);

	WeakReference handle = std::move(helper.pop_parameter());

	if (handle.data()->format != Data::FMT_NONE) {
		CloseHandle(to_handle(helper.pop_parameter()));
	}
#else
	((void)cursor);
#endif
}

MINT_FUNCTION(mint_process_start, 5, cursor) {

	FunctionHelper helper(cursor, 5);

	WeakReference pipes = std::move(helper.pop_parameter());
	WeakReference environment = std::move(helper.pop_parameter());
	WeakReference working_directory = std::move(helper.pop_parameter());
	WeakReference arguments = std::move(helper.pop_parameter());
	WeakReference process = std::move(helper.pop_parameter());
	WeakReference result = create_iterator();

#ifdef OS_WINDOWS

	std::wstringstream command;
	wchar_t *process_working_directory = nullptr;
	wchar_t **process_environment = nullptr;
	DWORD dwCreationFlags;
	STARTUPINFOW startup_info;
	PROCESS_INFORMATION process_info;

	dwCreationFlags = (GetConsoleWindow() ? 0 : CREATE_NO_WINDOW);
	ZeroMemory(&startup_info, sizeof(startup_info));
	startup_info.cb = sizeof(startup_info);

	auto escape = [](std::wstring &&arg) -> std::wstring && {
		if (arg.empty()) {
			arg = L"\"\"";
		}
		else if ((*arg.begin() != '"') && (*arg.rbegin() != '"') && (arg.find(' ') != std::wstring::npos)) {
			arg = L"\"" + arg + L"\"";
		}
		return std::move(arg);
	};

	command << escape(FileSystem::normalized(to_string(process)).generic_wstring());

	for (Array::values_type::value_type &argv : to_array(arguments)) {
		command << L" " << escape(utf8_to_windows(to_string(array_get_item(argv))));
	}

	if (working_directory.data()->format != Data::FMT_NONE) {
		std::wstring working_directory_str = utf8_to_windows(to_string(working_directory));
		process_working_directory = _wcsdup(working_directory_str.c_str());
	}

	if (environment.data()->format != Data::FMT_NONE) {
		process_environment = new wchar_t *[environment.data<Hash>()->values.size() + 1];
		dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
		size_t var_pos = 0;
		for (auto &var : environment.data<Hash>()->values) {
			std::wstring name = utf8_to_windows(to_string(hash_get_key(var)));
			std::wstring value = utf8_to_windows(to_string(hash_get_value(var)));
			auto *buffer = new wchar_t[name.size() + value.size() + 2];
			wsprintfW(buffer, L"%ls=%ls", name.c_str(), value.c_str());
			process_environment[var_pos++] = buffer;
		}
		process_environment[var_pos] = nullptr;
	}

	if (pipes.data()->format != Data::FMT_NONE) {

		auto get_pipe_handle = [](const Reference &pipes, intmax_t pipe, intmax_t handle) {
			return to_handle(array_get_item(array_get_item(pipes.data<Array>(), pipe).data<Array>(), handle));
		};

		if (SetHandleInformation(get_pipe_handle(pipes, 0, 0), HANDLE_FLAG_INHERIT, 0)) {
			startup_info.hStdInput = get_pipe_handle(pipes, 0, 0);
			startup_info.dwFlags |= STARTF_USESTDHANDLES;
		}

		if (SetHandleInformation(get_pipe_handle(pipes, 1, 0), HANDLE_FLAG_INHERIT, 0)) {
			startup_info.hStdOutput = get_pipe_handle(pipes, 1, 0);
			startup_info.dwFlags |= STARTF_USESTDHANDLES;
		}

		if (SetHandleInformation(get_pipe_handle(pipes, 2, 0), HANDLE_FLAG_INHERIT, 0)) {
			startup_info.hStdError = get_pipe_handle(pipes, 2, 0);
			startup_info.dwFlags |= STARTF_USESTDHANDLES;
		}
	}

	std::wstring command_line = command.str();

	if (CreateProcessW(nullptr, const_cast<wchar_t *>(command_line.data()), nullptr, nullptr, false, dwCreationFlags,
					   process_environment, process_working_directory, &startup_info, &process_info)) {
		iterator_yield(result.data<Iterator>(), WeakReference::create<None>());
		iterator_yield(result.data<Iterator>(), create_handle(process_info.hProcess));
		CloseHandle(process_info.hThread);
	}
	else {
		iterator_yield(result.data<Iterator>(), create_number(errno_from_error_code(last_error_code())));
	}
#else
	pid_t pid = fork();

	if (pid == 0) {

		std::vector<char *> args;

		std::string process_str = to_string(process);
		args.push_back(strdup(process_str.data()));

		for (Array::values_type::value_type &argv : to_array(arguments)) {
			std::string argv_str = to_string(array_get_item(argv));
			args.push_back(strdup(argv_str.data()));
		}

		args.push_back(nullptr);

		if (working_directory.data()->format != Data::FMT_NONE) {
			std::string working_directory_str = to_string(working_directory);
			chdir(working_directory_str.data());
		}

		if (pipes.data()->format != Data::FMT_NONE) {

			WeakReference stdin_pipe = array_get_item(pipes.data<Array>(), STDIN_FILENO);
			WeakReference stdout_pipe = array_get_item(pipes.data<Array>(), STDOUT_FILENO);
			WeakReference stderr_pipe = array_get_item(pipes.data<Array>(), STDERR_FILENO);

			dup2(static_cast<int>(to_handle(array_get_item(stdin_pipe.data<Array>(), 0))), STDIN_FILENO);
			dup2(static_cast<int>(to_handle(array_get_item(stdout_pipe.data<Array>(), 1))), STDOUT_FILENO);
			dup2(static_cast<int>(to_handle(array_get_item(stderr_pipe.data<Array>(), 1))), STDERR_FILENO);
		}
		else {
			struct rlimit limit;

			getrlimit(RLIMIT_NOFILE, &limit);

			for (int fd = 3; fd < static_cast<int>(limit.rlim_cur); ++fd) {
				close(fd);
			}
		}

		if (environment.data()->format != Data::FMT_NONE) {

			std::vector<char *> envp;

			for (auto &var : environment.data<Hash>()->values) {
				std::string name = to_string(hash_get_key(var));
				std::string value = to_string(hash_get_value(var));
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
		iterator_yield(result.data<Iterator>(), WeakReference::create<None>());
		iterator_yield(result.data<Iterator>(), create_handle(pid));
	}
	else {
		iterator_yield(result.data<Iterator>(), create_number(errno));
	}
#endif

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_process_getcmdline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = to_handle(helper.pop_parameter());

	if (LPWSTR szCmdLine = GetNtProcessCommandLine(handle)) {

		WeakReference results = create_iterator();
		WeakReference args = create_array();

		int argc = 0;
		wchar_t **argv = CommandLineToArgvW(szCmdLine, &argc);

		for (int argn = 0; argn < argc; ++argn) {
			if (results.data<Iterator>()->ctx.empty()) {
				iterator_yield(results.data<Iterator>(), create_string(windows_to_utf8(argv[argn])));
			}
			else {
				array_append(args.data<Array>(), create_string(windows_to_utf8(argv[argn])));
			}
		}

		iterator_yield(results.data<Iterator>(), std::move(args));

		helper.return_value(std::move(results));
	}
#else
	pid_t pid = static_cast<pid_t>(to_handle(helper.pop_parameter()));

	char cmdline_path[FileSystem::PATH_LENGTH];
	WeakReference results = create_iterator();
	WeakReference args = create_array();

	snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);
	FILE *cmdline = open_file(cmdline_path, "r");

	char *buffer = nullptr;
	size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, cmdline) != -1) {
		if (results.data<Iterator>()->ctx.empty()) {
			iterator_yield(results.data<Iterator>(), create_string(std::string(buffer, buffer_length)));
		}
		else {
			array_append(args.data<Array>(), create_string(std::string(buffer, buffer_length)));
		}
	}

	iterator_yield(results.data<Iterator>(), std::move(args));
	fclose(cmdline);
	free(buffer);

	helper.return_value(std::move(results));
#endif
}

MINT_FUNCTION(mint_process_getcwd, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = to_handle(helper.pop_parameter());
	DWORD dwLength = GetNtProcessCurrentDirectory(handle, NULL, 0);
	auto szCurrentDirectoryPath = static_cast<LPWSTR>(malloc(dwLength * sizeof(WCHAR)));

	if (GetNtProcessCurrentDirectory(handle, szCurrentDirectoryPath, dwLength)) {
		helper.return_value(create_string(std::filesystem::path(szCurrentDirectoryPath).generic_string()));
	}

	free(szCurrentDirectoryPath);
#else
	pid_t pid = static_cast<pid_t>(to_handle(helper.pop_parameter()));

	char exe_path[FileSystem::PATH_LENGTH];
	char proc_path[FileSystem::PATH_LENGTH];
	snprintf(exe_path, sizeof(exe_path), "/proc/%d/exe", pid);
	ssize_t count = readlink(exe_path, proc_path, sizeof(proc_path));

	if (count > 0) {
		helper.return_value(create_string(proc_path));
	}
#endif
}

MINT_FUNCTION(mint_process_getenv, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = to_handle(helper.pop_parameter());

	if (LPWCH szEnvironment = GetNtProcessEnvironmentStrings(handle)) {

		WeakReference results = create_hash();
		LPCWSTR buffer = szEnvironment;

		while (*buffer) {
			LPCWSTR cptr = wcschr(buffer, L'=');
			hash_insert(results.data<Hash>(), create_string(windows_to_utf8(std::wstring(buffer, cptr))),
						create_string(windows_to_utf8(std::wstring(cptr + 1))));
			buffer += lstrlenW(buffer) + 1;
		}

		FreeEnvironmentStringsW(szEnvironment);
		helper.return_value(std::move(results));
	}
#else
	pid_t pid = static_cast<pid_t>(to_handle(helper.pop_parameter()));

	char environ_path[FileSystem::PATH_LENGTH];
	WeakReference results = create_hash();

	snprintf(environ_path, sizeof(environ_path), "/proc/%d/environ", pid);
	FILE *environ = open_file(environ_path, "r");

	char *buffer = nullptr;
	size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, environ) != -1) {
		char *cptr = strchr(buffer, '=');
		hash_insert(results.data<Hash>(), create_string(std::string(buffer, cptr)), create_string(cptr + 1));
	}

	fclose(environ);
	free(buffer);

	helper.return_value(std::move(results));
#endif
}

MINT_FUNCTION(mint_process_getpid, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	helper.return_value(create_number(GetCurrentProcessId()));
#else
	helper.return_value(create_number(getpid()));
#endif
}

MINT_FUNCTION(mint_process_waitpid, 4, cursor) {

	FunctionHelper helper(cursor, 4);

	WeakReference exit_code = std::move(helper.pop_parameter());
	WeakReference exit_status = std::move(helper.pop_parameter());
	bool wait_for_finished = to_boolean(helper.pop_parameter());

#ifdef OS_WINDOWS
	HANDLE handle = to_handle(helper.pop_parameter());

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

	helper.return_value(create_boolean(finished));
#else
	int pid = static_cast<int>(to_handle(helper.pop_parameter()));

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

	helper.return_value(create_boolean(finished));
#endif
}

MINT_FUNCTION(mint_process_kill, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = to_handle(helper.pop_parameter());

	if (!TerminateProcess(handle, 0xDEAD)) {
		helper.return_value(create_number(errno_from_error_code(last_error_code())));
	}
#else
	pid_t pid = static_cast<pid_t>(to_handle(helper.pop_parameter()));

	if (kill(pid, SIGKILL)) {
		helper.return_value(create_number(errno));
	}
#endif
}

MINT_FUNCTION(mint_process_terminate, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = to_handle(helper.pop_parameter());

	if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, GetProcessId(handle))) {
		helper.return_value(create_number(errno_from_error_code(last_error_code())));
	}
#else
	pid_t pid = static_cast<pid_t>(to_handle(helper.pop_parameter()));

	if (kill(pid, SIGTERM)) {
		helper.return_value(create_number(errno));
	}
#endif
}
