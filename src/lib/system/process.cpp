#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "scheduler/scheduler.h"

#include <string>

using namespace std;

extern "C" {

void mint_process_exec_2(Cursor *cursor) {

	FunctionHelper helper(cursor, 2);

	auto args = to_array(*helper.popParameter());
	string command = to_string(*helper.popParameter());

	/// \todo build command line

	string command_line = command;
	helper.returnValue(create_number(system(command_line.c_str())));
}

void mint_process_fork_0(Cursor *cursor) {

	FunctionHelper helper(cursor, 0);

	Cursor *child = Scheduler::instance()->ast()->createCursor();
	FunctionHelper child_helper(child, 0);

	helper.returnValue(create_number(0));
	child_helper.returnValue(create_number(Scheduler::instance()->createThread(new Process(child))));
}

}
