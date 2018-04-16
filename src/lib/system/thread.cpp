#include <memory/functiontool.h>
#include <memory/operatortool.h>
#include <scheduler/scheduler.h>

using namespace mint;

MINT_FUNCTION(mint_thread_current_id, 0, cursor) {

	FunctionHelper helper(cursor, 0);

	if (Process *process = Scheduler::instance()->currentProcess()) {
		helper.returnValue(create_number(process->getThreadId()));
	}
	else {
		helper.returnValue(Reference::create<None>());
	}
}

MINT_FUNCTION(mint_thread_start_member, 3, cursor) {

	FunctionHelper helper(cursor, 3);

	SharedReference inst = helper.popParameter();
	SharedReference args = helper.popParameter();
	SharedReference func = helper.popParameter();

	int argc = 0;
	if (Scheduler *scheduler = Scheduler::instance()) {
		Cursor *thread_cursor = scheduler->ast()->createCursor();
		/// \todo Copy ???
		thread_cursor->stack().push_back(inst);
		while (SharedReference argv = iterator_next(args->data<Iterator>())) {
			/// \todo Copy ???
			thread_cursor->stack().push_back(argv);
			argc++;
		}
		thread_cursor->waitingCalls().push(func);
		call_member_operator(thread_cursor, argc);
		helper.returnValue(create_number(scheduler->createThread(new Process(thread_cursor))));
	}
	else {
		helper.returnValue(Reference::create<None>());
	}
}

MINT_FUNCTION(mint_thread_start, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference args = helper.popParameter();
	SharedReference func = helper.popParameter();

	int argc = 0;
	if (Scheduler *scheduler = Scheduler::instance()) {
		Cursor *thread_cursor = scheduler->ast()->createCursor();
		while (SharedReference argv = iterator_next(args->data<Iterator>())) {
			/// \todo Copy ???
			thread_cursor->stack().push_back(argv);
			argc++;
		}
		thread_cursor->waitingCalls().push(func);
		call_operator(thread_cursor, argc);
		helper.returnValue(create_number(scheduler->createThread(new Process(thread_cursor))));
	}
	else {
		helper.returnValue(Reference::create<None>());
	}
}

MINT_FUNCTION(mint_thread_is_joinable, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference thread_id = helper.popParameter();

	if (Scheduler *scheduler = Scheduler::instance()) {
		helper.returnValue(create_boolean(scheduler->findThread(static_cast<int>(to_number(cursor, *thread_id)))));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}
