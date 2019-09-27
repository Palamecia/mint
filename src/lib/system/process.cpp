#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/filesystem.h"

#include <climits>
#include <string>
#include <thread>
#ifdef OS_WINDOWS
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/resource.h>
#endif

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_process_exec, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	string command = to_string(helper.popParameter());

#ifdef OS_WINDOWS
	/// \todo
#else
	helper.returnValue(create_number(system(command.data())));
#endif
}

MINT_FUNCTION(mint_process_get_handle, 1, cursor) {

#ifdef OS_WINDOWS

	FunctionHelper helper(cursor, 1);

	DWOD procId = to_number(helper.popParameter());

	helper.returnValue(create_object(OpenProcess(PROCESS_ALL_ACCESS, TRUE, procId)));
#else
	((void)cursor);
#endif
}

MINT_FUNCTION(mint_process_get_pid, 1, cursor) {

#ifdef OS_WINDOWS

	FunctionHelper helper(cursor, 1);

	SharedReference handle = helper.popParameter();

	if (handle->data()->format != Data::fmt_none) {
		helper.returnValue(create_number(GetProcessId(*handle->data<LibObject<HANDLE>>()->impl)));
	}
#else
	((void)cursor);
#endif
}

MINT_FUNCTION(mint_process_start, 5, cursor) {

	FunctionHelper helper(cursor, 5);

	SharedReference pipes = helper.popParameter();
	SharedReference environement = helper.popParameter();
	SharedReference workingDirectory = helper.popParameter();
	SharedReference arguments = helper.popParameter();
	SharedReference process = helper.popParameter();

	vector<char *> args;

	for (SharedReference argv : to_array(arguments)) {
		args.push_back(strdup(to_string(argv).data()));
	}

#ifdef OS_WINDOWS

	// CreateProcess
	// SetStdHandle

#else
	pid_t pid = fork();

	if (pid == 0) {

		struct rlimit limit;

		getrlimit(RLIMIT_NOFILE, &limit);

		for (int fd = 3; fd < static_cast<int>(limit.rlim_cur); ++fd) {
			close(fd);
		}

		if (workingDirectory->data()->format != Data::fmt_none) {
			chdir(to_string(workingDirectory).data());
		}

		if (pipes->data()->format != Data::fmt_none) {
			for (int fd = 0; fd < 3; ++fd) {
				dup2(fd, static_cast<int>(to_number(cursor, array_get_item(pipes->data<Array>(), static_cast<long>(fd)))));
			}
		}

		if (environement->data()->format == Data::fmt_none) {
			execv(to_string(process).data(), args.data());
		}
		else {

			vector<char *> envp;

			for (SharedReference argv : to_array(environement)) {
				envp.push_back(strdup(to_string(argv).data()));
			}

			execve(to_string(process).data(), args.data(), envp.data());
		}
	}
#endif

	if (pid != -1) {
		helper.returnValue(create_number(pid));
	}
}

MINT_FUNCTION(mint_process_getcmdline, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	/// \todo
#else

	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	char cmdline_path[PATH_MAX];
	Reference *results = Reference::create<Iterator>();
	Reference *args = Reference::create<Array>();

	snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);
	FILE *cmdline = open_file(cmdline_path, "r");

	char *buffer = nullptr;
	size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, cmdline) != -1) {
		if (results->data<Iterator>()->ctx.empty()) {
			iterator_insert(results->data<Iterator>(), create_string(string(buffer, buffer_length)));
		}
		else {
			array_append(args->data<Array>(), create_string(string(buffer, buffer_length)));
		}
	}

	results->data<Iterator>()->construct();
	args->data<Array>()->construct();
	iterator_insert(results->data<Iterator>(), SharedReference::unique(args));
	fclose(cmdline);
	free(buffer);

	helper.returnValue(results);
#endif
}

MINT_FUNCTION(mint_process_getcwd, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	/// \todo
#else

	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	char exe_path[PATH_MAX];
	char proc_path[PATH_MAX];
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
	/// \todo
#else

	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	char environ_path[PATH_MAX];
	Reference *results = Reference::create<Hash>();

	snprintf(environ_path, sizeof(environ_path), "/proc/%d/environ", pid);
	FILE *environ = open_file(environ_path, "r");

	char *buffer = nullptr;
	size_t buffer_length = 0;

	while (getdelim(&buffer, &buffer_length, 0, environ) != -1) {
		char *cptr = strchr(buffer, '=');
		hash_insert(results->data<Hash>(), create_string(string(buffer, cptr)), create_string(cptr + 1));
	}

	results->data<Hash>()->construct();
	fclose(environ);
	free(buffer);

	helper.returnValue(results);
#endif
}

MINT_FUNCTION(mint_process_getpid, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	/// \todo
#else
	helper.returnValue(create_number(getpid()));
#endif
}

MINT_FUNCTION(mint_process_waitpid, 4, cursor) {

	FunctionHelper helper(cursor, 4);

	SharedReference exit_code = helper.popParameter();
	SharedReference exit_status = helper.popParameter();
	bool wait_for_finished = to_boolean(cursor, helper.popParameter());
	int pid = static_cast<int>(to_number(cursor, helper.popParameter()));

#ifdef OS_WINDOWS
	/// \todo
#else
	int status = 0;
	int options = 0;
	bool finished = false;

	if (!wait_for_finished) {
		options |= WNOHANG;
	}

	do {
		if (waitpid(pid, &status, options) == pid) {
			exit_status->data<Boolean>()->value = WIFEXITED(status);
			exit_code->data<Number>()->value = WEXITSTATUS(status);
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
	/// \todo
#else

	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	helper.returnValue(create_boolean(!kill(pid, SIGKILL)));
#endif
}

MINT_FUNCTION(mint_process_treminate, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	/// \todo
#else

	pid_t pid = static_cast<pid_t>(to_number(cursor, helper.popParameter()));

	helper.returnValue(create_boolean(!kill(pid, SIGTERM)));
#endif
}
