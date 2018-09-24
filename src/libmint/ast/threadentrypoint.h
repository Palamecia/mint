#ifndef THREAD_ENTRY_POINT_H
#define THREAD_ENTRY_POINT_H

#include "ast/module.h"

namespace mint {

class ThreadEntryPoint : public Module {
public:
	static ThreadEntryPoint *instance();

protected:
	ThreadEntryPoint();

private:
	static ThreadEntryPoint g_instance;
};

}

#endif // THREAD_ENTRY_POINT_H
