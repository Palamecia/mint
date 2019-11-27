#include <memory/functiontool.h>
#include <memory/operatortool.h>
#include <memory/memorytool.h>
#include <ast/abstractsyntaxtree.h>
#include <scheduler/scheduler.h>

using namespace mint;
using namespace std;

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

	SharedReference args = move(helper.popParameter());
	SharedReference func = move(helper.popParameter());
	SharedReference inst = move(helper.popParameter());

	int argc = 0;

	if (Scheduler *scheduler = Scheduler::instance()) {
		Cursor *thread_cursor = AbstractSyntaxTree::instance().createCursor();
		/// \todo Copy ???
		thread_cursor->stack().emplace_back(move(inst));
		while (SharedReference argv = iterator_next(args->data<Iterator>())) {
			/// \todo Copy ???
			thread_cursor->stack().emplace_back(move(argv));
			argc++;
		}
		thread_cursor->waitingCalls().emplace(move(func));

		if (Class::MemberInfo *infos = get_member_infos(inst->data<Object>(), func)) {
			thread_cursor->waitingCalls().top().setMetadata(infos->owner);
		}

		call_member_operator(thread_cursor, argc);
		helper.returnValue(create_number(scheduler->createThread(new Process(thread_cursor))));
	}
}

MINT_FUNCTION(mint_thread_start, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference args = move(helper.popParameter());
	SharedReference func = move(helper.popParameter());

	int argc = 0;

	if (Scheduler *scheduler = Scheduler::instance()) {
		Cursor *thread_cursor = AbstractSyntaxTree::instance().createCursor();
		while (SharedReference argv = iterator_next(args->data<Iterator>())) {
			/// \todo Copy ???
			thread_cursor->stack().emplace_back(move(argv));
			argc++;
		}
		thread_cursor->waitingCalls().emplace(move(func));
		call_operator(thread_cursor, argc);
		helper.returnValue(create_number(scheduler->createThread(new Process(thread_cursor))));
	}
}

MINT_FUNCTION(mint_thread_is_joinable, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference thread_id = move(helper.popParameter());

	if (Scheduler *scheduler = Scheduler::instance()) {
		helper.returnValue(create_boolean(scheduler->findThread(static_cast<int>(to_number(cursor, thread_id)))));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}

MINT_FUNCTION(mint_thread_wait, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	Scheduler::instance()->currentProcess()->wait();
}

MINT_FUNCTION(mint_thread_sleep, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference time = move(helper.popParameter());
	Scheduler::instance()->currentProcess()->sleep(static_cast<unsigned int>(to_number(cursor, time)));
}
