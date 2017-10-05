#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "scheduler/scheduler.h"

#include <string>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

using namespace std;

extern "C" {

void mint_process_exec_2(Cursor *cursor) {

	FunctionHelper helper(cursor, 2);

	Array::values_type args = to_array(*helper.popParameter());
	string command = to_string(*helper.popParameter());

	/// \todo build command line

	string command_line = command;
	for (SharedReference &arg : args) {
		command_line += " " + to_string(*arg);
	}
	helper.returnValue(create_number(system(command_line.c_str())));
}

void mint_process_fork_0(Cursor *cursor) {

	FunctionHelper helper(cursor, 0);

	helper.returnValue(create_number(fork()));
}

void mint_process_getpid_0(Cursor *cursor) {

	FunctionHelper helper(cursor, 0);

	helper.returnValue(create_number(getpid()));
}

void mint_process_waitpid_1(Cursor *cursor) {

	FunctionHelper helper(cursor, 1);

	double pid = to_number(cursor, *helper.popParameter());

	/// \todo optional additional options
	helper.returnValue(create_number(waitpid(pid, nullptr, 0)));
}

}
