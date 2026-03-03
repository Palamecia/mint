#ifndef MINT_DEBUG_PROCESSDEBUGGER_H
#define MINT_DEBUG_PROCESSDEBUGGER_H

#include "mint/debug/debuginterface.h"
#include "mint/scheduler/process.h"

namespace mint {

class ProcessDebugger {
public:
	ProcessDebugger(DebugInterface& handle, Process& process);

	bool exec();
	void resume();

private:
	DebugThreadLocker _thread_locker;
};

}

#endif // MINT_DEBUG_PROCESSDEBUGGER_H
