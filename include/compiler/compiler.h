#ifndef COMPIMER_H
#define COMPIMER_H

#include "buildtool.h"

#if !defined(NDEBUG) && !defined(_DEBUG)
#define DEBUG_STACK(msg, ...) printf("[%08lx] " msg "\n", Compiler::context()->data.module->nextInstructionOffset(), ##__VA_ARGS__)
#else
#define DEBUG_STACK(msg, ...) ((void)0)
#endif

class Compiler {
public:
	Compiler();

	bool build(DataStream *stream, Module::Context node);

	static BuildContext *context();
	static Data *makeData(const std::string &token);

private:
	static BuildContext *g_ctx;
};

#endif // COMPIMER_H
