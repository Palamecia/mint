/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/system/filesystem.h"
#include "mint/system/errno.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <gsl/pointers>
#include <filesystem>
#include <string>

#ifdef MINT_OS_WINDOWS
#include "win32/NtProcessInfo.h"
#include <cstdint>
#include <memory>
#include <Windows.h>
#include <consoleapi.h>
#include <consoleapi2.h>
#include <consoleapi3.h>
#include <corecrt_wstring.h>
#include <cwchar>
#include <handleapi.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <process.h>
#include <processenv.h>
#include <processthreadsapi.h>
#include <shellapi.h>
#include <sstream>
#include <stringapiset.h>
#include <synchapi.h>
#include <TlHelp32.h>
#include <utility>
#include <winbase.h>
#include <winnls.h>
#include <winnt.h>
#include <winuser.h>
#else
#include <array>
#include <csignal>
#include <cstdio>
#include <dirent.h>
#include <format>
#include <ranges>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#endif

namespace {

#ifdef MINT_OS_WINDOWS
std::wstring utf8_to_windows(const std::string& str) {
	const int length = MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, nullptr, 0);
	if (std::wstring buffer(length, L'\0'); MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, buffer.data(), length)) {
		return buffer;
	}
	return {};
}

std::string windows_to_utf8(const std::wstring& str) {
	const int length = WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, nullptr, 0, nullptr, nullptr);
	if (std::string buffer(length, '\0');
	    WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, buffer.data(), length, nullptr, nullptr)) {
		buffer.resize(length - 1);
		return buffer;
	}
	return {};
}

constexpr inline DWORD internal_kill_code = 0xDEAD;

#endif

mint::WeakReference mint_process_list(mint::Cursor& cursor) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

#ifdef MINT_OS_WINDOWS
	PROCESSENTRY32 pe = {sizeof(PROCESSENTRY32)};

	if (HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); snap != INVALID_HANDLE_VALUE) {
		for (BOOL found = Process32First(snap, &pe); found; found = Process32Next(snap, &pe)) {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(pe.th32ProcessID));
		}
		CloseHandle(snap);
	}
#else
	if (DIR* proc = opendir("/proc/")) {

		while (const dirent* process = readdir(proc)) {

			char* error = nullptr;
			auto pid = static_cast<pid_t>(strtol(process->d_name, &error, 10));

			if (!*error) {
				mint::iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(pid));
			}
		}

		closedir(proc);
	}
#endif

	return result;
}

mint::WeakReference mint_process_exec(mint::Cursor& /*cursor*/, const mint::Reference& command) {
	return mint::create_signed_number(system(to_string(command).data()));
}

mint::WeakReference mint_process_get_handle(mint::Cursor& cursor, const mint::Reference& pid) {
#ifdef MINT_OS_WINDOWS
	const auto proc_id = mint::to_integer<DWORD>(cursor, pid);
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, proc_id);

	if (handle == INVALID_HANDLE_VALUE) {
		handle = OpenProcess(STANDARD_RIGHTS_REQUIRED, TRUE, proc_id);
	}

	return mint::create_c_object(cursor.ast(), handle);
#else
	((void)cursor);
	return {};
#endif
}

mint::WeakReference mint_process_get_pid(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
	if (handle.data().format() != mint::Data::Format::none) {
#ifdef MINT_OS_WINDOWS
		return mint::create_number(GetProcessId(to_handle(handle)));
#else
		return mint::create_number(static_cast<pid_t>(to_handle(handle)));
#endif
	}
	return {};
}

mint::WeakReference mint_process_close_handle(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	if (handle.data().format() != mint::Data::Format::none) {
		CloseHandle(to_handle(handle));
	}
#else
	((void)handle);
#endif
	return {};
}

mint::WeakReference mint_process_start(mint::Cursor& cursor, const mint::Reference& process,
    const mint::Reference& arguments, mint::Reference& working_directory, const mint::Reference& environment,
    const mint::Reference& pipes) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

#ifdef MINT_OS_WINDOWS

	std::wstringstream command;
	wchar_t* process_working_directory = nullptr;
	wchar_t** process_environment = nullptr;
	DWORD creation_flags = (GetConsoleWindow() ? 0 : CREATE_NO_WINDOW);
	STARTUPINFOW startup_info;
	PROCESS_INFORMATION process_info;

	ZeroMemory(&startup_info, sizeof(startup_info));
	startup_info.cb = sizeof(startup_info);

	auto escape = [](std::wstring&& arg) -> std::wstring&& {
		if (arg.empty()) {
			arg = L"\"\"";
		}
		else if ((*arg.begin() != '"') && (*arg.rbegin() != '"') && (arg.find(' ') != std::wstring::npos)) {
			arg = L"\"" + arg + L"\"";
		}
		return std::move(arg);
	};

	command << escape(mint::FileSystem::normalized(to_string(process)).generic_wstring());

	for (auto& argv : to_array(arguments)) {
		command << L" " << escape(utf8_to_windows(to_string(array_get_item(argv))));
	}

	if (working_directory.data().format() != mint::Data::Format::none) {
		const auto working_directory_str = utf8_to_windows(to_string(working_directory));
		process_working_directory = _wcsdup(working_directory_str.c_str());
	}

	if (environment.data().format() != mint::Data::Format::none) {
		process_environment = new wchar_t*[environment.data<mint::Hash>().values.size() + 1];
		creation_flags |= CREATE_UNICODE_ENVIRONMENT;
		std::size_t var_pos = 0;
		for (auto& var : environment.data<mint::Hash>().values) {
			const auto name = utf8_to_windows(to_string(hash_get_key(var)));
			const auto value = utf8_to_windows(to_string(hash_get_value(var)));
			auto* buffer = new wchar_t[name.size() + value.size() + 2];
			wsprintfW(buffer, L"%ls=%ls", name.c_str(), value.c_str());
			process_environment[var_pos++] = buffer;
		}
		process_environment[var_pos] = nullptr;
	}

	if (pipes.data().format() != mint::Data::Format::none) {

		auto get_pipe_handle = [](const mint::Reference& pipes, intmax_t pipe, intmax_t handle) {
			return to_handle(
			    array_get_item(array_get_item(pipes.data<mint::Array>(), pipe).data<mint::Array>(), handle));
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

	if (CreateProcessW(nullptr, const_cast<wchar_t*>(command_line.data()), nullptr, nullptr, false, creation_flags,
	        process_environment, process_working_directory, &startup_info, &process_info)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_handle(cursor.ast(), process_info.hProcess));
		CloseHandle(process_info.hThread);
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_number(mint::errno_from_error_code(mint::last_error_code())));
	}
#else
	const auto pid = fork();

	if (pid == 0) {

		std::vector<char*> args;

		std::string process_str = to_string(process);
		args.push_back(strdup(process_str.data()));

		for (auto& argv : to_array(arguments)) {
			std::string argv_str = to_string(array_get_item(argv));
			args.push_back(strdup(argv_str.data()));
		}

		args.push_back(nullptr);

		if (working_directory.data().format() != mint::Data::Format::none) {
			std::string working_directory_str = to_string(working_directory);
			chdir(working_directory_str.data());
		}

		if (pipes.data().format() != mint::Data::Format::none) {

			const auto stdin_pipe = array_get_item(pipes.data<mint::Array>(), STDIN_FILENO);
			const auto stdout_pipe = array_get_item(pipes.data<mint::Array>(), STDOUT_FILENO);
			const auto stderr_pipe = array_get_item(pipes.data<mint::Array>(), STDERR_FILENO);

			dup2(static_cast<int>(to_handle(array_get_item(stdin_pipe.data<mint::Array>(), 0))), STDIN_FILENO);
			dup2(static_cast<int>(to_handle(array_get_item(stdout_pipe.data<mint::Array>(), 1))), STDOUT_FILENO);
			dup2(static_cast<int>(to_handle(array_get_item(stderr_pipe.data<mint::Array>(), 1))), STDERR_FILENO);
		}
		else {
			struct rlimit limit;

			getrlimit(RLIMIT_NOFILE, &limit);

			for (int fd = 3; fd < static_cast<int>(limit.rlim_cur); ++fd) {
				close(fd);
			}
		}

		if (environment.data().format() != mint::Data::Format::none) {

			auto envp = std::vector<char*>(std::from_range,
			    std::views::transform(environment.data<mint::Hash>().values, [](auto& var) -> char* {
				    return strdup(
				        std::format("{}={}", to_string(hash_get_key(var)), to_string(hash_get_value(var))).data());
			    }));

			envp.push_back(nullptr);

			execve(args.front(), args.data(), envp.data());
		}
		else {
			execve(args.front(), args.data(), environ);
		}

		exit(EXIT_FAILURE);
	}

	if (pid != -1) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_handle(cursor.ast(), pid));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno));
	}
#endif

	return result;
}

mint::WeakReference mint_process_getcmdline(mint::Cursor& cursor, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	mint::WeakReference results = mint::create_iterator(cursor.ast());

	if (LPWSTR cmd_line = mint::GetNtProcessCommandLine(to_handle(handle))) {

		mint::WeakReference args = mint::create_array(cursor.ast());

		int argc = 0;
		wchar_t** argv = CommandLineToArgvW(cmd_line, &argc);

		for (int argn = 0; argn < argc; ++argn) {
			if (results.data<mint::Iterator>().ctx.empty()) {
				iterator_yield(cursor, results.data<mint::Iterator>(),
				    mint::create_string(cursor.ast(), windows_to_utf8(argv[argn])));
			}
			else {
				array_append(args.data<mint::Array>(), mint::create_string(cursor.ast(), windows_to_utf8(argv[argn])));
			}
		}

		iterator_yield(cursor, results.data<mint::Iterator>(), std::move(args));
	}

	return results;
#else
	const auto pid = static_cast<pid_t>(to_handle(handle));

	mint::WeakReference results = create_iterator(cursor.ast());
	mint::WeakReference args = create_array(cursor.ast());

	auto cmdline_path = std::format("/proc/{}/cmdline", pid);
	gsl::owner<FILE*> cmdline = mint::open_file(cmdline_path, "r");

	gsl::owner<char*> buffer = nullptr;
	std::size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, cmdline) != -1) {
		if (results.data<mint::Iterator>().ctx.empty()) {
			iterator_yield(cursor, results.data<mint::Iterator>(),
			    create_string(cursor.ast(), std::string(buffer, buffer_length)));
		}
		else {
			array_append(args.data<mint::Array>(),
			    mint::create_string(cursor.ast(), std::string(buffer, buffer_length)));
		}
	}

	iterator_yield(cursor, results.data<mint::Iterator>(), std::move(args));
	std::fclose(cmdline);
	std::free(buffer);

	return results;
#endif
}

mint::WeakReference mint_process_getcwd(mint::Cursor& cursor, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	const auto length = mint::GetNtProcessCurrentDirectory(to_handle(handle), nullptr, 0);
	auto current_directory_path = std::make_unique<WCHAR[]>(length);

	if (mint::GetNtProcessCurrentDirectory(to_handle(handle), current_directory_path.get(), length)) {
		return mint::create_string(cursor.ast(), std::filesystem::path(current_directory_path.get()).generic_string());
	}
#else
	const auto pid = static_cast<pid_t>(to_handle(handle));

	std::array<char, mint::FileSystem::path_length> proc_path {};
	const auto exe_path = std::format("/proc/{}/exe", pid);
	const auto count = readlink(exe_path.data(), proc_path.data(), proc_path.size());

	if (count > 0) {
		return mint::create_string(cursor.ast(), std::string(proc_path.data(), static_cast<std::size_t>(count)));
	}
#endif
	return {};
}

mint::WeakReference mint_process_getenv(mint::Cursor& cursor, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	mint::WeakReference results = mint::create_hash(cursor.ast());

	if (LPWCH environment = mint::GetNtProcessEnvironmentStrings(to_handle(handle))) {
		for (LPCWSTR buffer = environment; *buffer; buffer += lstrlenW(buffer) + 1) {
			LPCWSTR cptr = wcschr(buffer, L'=');
			mint::hash_insert(results.data<mint::Hash>(),
			    mint::create_string(cursor.ast(), windows_to_utf8(std::wstring(buffer, cptr))),
			    mint::create_string(cursor.ast(), windows_to_utf8(std::wstring(cptr + 1))));
		}
		FreeEnvironmentStringsW(environment);
	}

	return results;
#else
	const auto pid = static_cast<pid_t>(to_handle(handle));

	mint::WeakReference results = mint::create_hash(cursor.ast());

	const auto environ_path = std::format("/proc/{}/environ", pid);
	gsl::owner<FILE*> environ = mint::open_file(environ_path, "r");

	gsl::owner<char*> buffer = nullptr;
	std::size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, environ) != -1) {
		char* cptr = strchr(buffer, '=');
		hash_insert(results.data<mint::Hash>(), mint::create_string(cursor.ast(), std::string(buffer, cptr)),
		    mint::create_string(cursor.ast(), cptr + 1));
	}

	std::fclose(environ);
	std::free(buffer);

	return results;
#endif
}

mint::WeakReference mint_process_getpid(mint::Cursor& /*cursor*/) {
#ifdef MINT_OS_WINDOWS
	return mint::create_number(GetCurrentProcessId());
#else
	return mint::create_number(getpid());
#endif
}

mint::WeakReference mint_process_waitpid(mint::Cursor& /*cursor*/, const mint::Reference& handle,
    const mint::Reference& wait_for_finished, const mint::Reference& exit_status, const mint::Reference& exit_code) {

#ifdef MINT_OS_WINDOWS
	bool finished = false;

	if (WaitForSingleObject(to_handle(handle), to_boolean(wait_for_finished) ? INFINITE : 0) == WAIT_OBJECT_0) {

		DWORD value = 0;

		if (GetExitCodeProcess(to_handle(handle), &value)) {
			exit_status.data<mint::Boolean>().value = (value == internal_kill_code
			                                           || (value >= 0x80000000 && value < 0xD0000000));
			exit_code.data<mint::Number>().value = value;
		}

		CloseHandle(to_handle(handle));
		finished = true;
	}

	return mint::create_boolean(finished);
#else
	const auto pid = static_cast<pid_t>(to_handle(handle));

	int status = 0;
	int options = 0;
	bool finished = false;

	if (!to_boolean(wait_for_finished)) {
		options |= WNOHANG;
	}

	do {
		if (waitpid(pid, &status, options) == pid) {
			exit_status.data<mint::Boolean>().value = WIFEXITED(status);
			exit_code.data<mint::Number>().value = WEXITSTATUS(status);
			finished = true;
		}
	}
	while (!finished && to_boolean(wait_for_finished));

	return mint::create_boolean(finished);
#endif
}

mint::WeakReference mint_process_kill(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	if (!TerminateProcess(to_handle(handle), internal_kill_code)) {
		return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
	}
#else
	const auto pid = static_cast<pid_t>(to_handle(handle));

	if (kill(pid, SIGKILL)) {
		return mint::create_number(errno);
	}
#endif
	return {};
}

mint::WeakReference mint_process_terminate(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, GetProcessId(to_handle(handle)))) {
		return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
	}
#else
	const auto pid = static_cast<pid_t>(to_handle(handle));

	if (kill(pid, SIGTERM)) {
		return mint::create_number(errno);
	}
#endif
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_process_list, 0);
MINT_EXPORT_FUNCTION(mint_process_exec, 1);
MINT_EXPORT_FUNCTION(mint_process_get_handle, 1);
MINT_EXPORT_FUNCTION(mint_process_get_pid, 1);
MINT_EXPORT_FUNCTION(mint_process_close_handle, 1);
MINT_EXPORT_FUNCTION(mint_process_start, 5);
MINT_EXPORT_FUNCTION(mint_process_getcmdline, 1);
MINT_EXPORT_FUNCTION(mint_process_getcwd, 1);
MINT_EXPORT_FUNCTION(mint_process_getenv, 1);
MINT_EXPORT_FUNCTION(mint_process_getpid, 0);
MINT_EXPORT_FUNCTION(mint_process_waitpid, 4);
MINT_EXPORT_FUNCTION(mint_process_kill, 1);
MINT_EXPORT_FUNCTION(mint_process_terminate, 1);
