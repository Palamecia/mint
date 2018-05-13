#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "scheduler/scheduler.h"

#include <string>
#include <thread>
#ifdef OS_WINDOWS
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_process_exec, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	Array::values_type args = to_array(*helper.popParameter());
	string command = to_string(*helper.popParameter());

	/// \todo build command line

	string command_line = command;
	for (SharedReference &arg : args) {
		command_line += " " + to_string(*arg);
	}


#ifdef OS_WINDOWS
	/// \todo
#else
	helper.returnValue(create_number(system(command_line.c_str())));
#endif
}

MINT_FUNCTION(mint_process_fork, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	/// \todo
	// helper.returnValue(create_number(CreateProcess()));
#else
	helper.returnValue(create_number(fork()));
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

MINT_FUNCTION(mint_process_waitpid, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	double pid = to_number(cursor, *helper.popParameter());

#ifdef OS_WINDOWS
	/// \todo
#else
	/// \todo optional additional options
	helper.returnValue(create_number(waitpid(pid, nullptr, 0)));
#endif
}

MINT_FUNCTION(mint_process_wait, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	Scheduler::instance()->currentProcess()->wait();
}

MINT_FUNCTION(mint_process_sleep, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference time = helper.popParameter();
	Scheduler::instance()->currentProcess()->sleep(static_cast<unsigned int>(to_number(cursor, *time)));
}
