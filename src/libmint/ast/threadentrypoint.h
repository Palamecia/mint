#ifndef THREAD_ENTRY_POINT_H
#define THREAD_ENTRY_POINT_H

#include "ast/module.h"

class ThreadEntryPoint : public Module {
public:
	static ThreadEntryPoint *instance();

protected:
	ThreadEntryPoint();
};

#endif // THREAD_ENTRY_POINT_H
