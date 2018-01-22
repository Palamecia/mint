#ifndef COMPIMER_H
#define COMPIMER_H

#include "buildtool.h"

#if !defined(NDEBUG) && !defined(_DEBUG)
#define DEBUG_STACK(msg, ...) printf("[%08lx] " msg "\n", mint::Compiler::context()->data.module->nextNodeOffset(), ##__VA_ARGS__)
#else
#define DEBUG_STACK(msg, ...) ((void)0)
#endif

namespace mint {

class MINT_EXPORT Compiler {
public:
	Compiler();

	bool build(DataStream *stream, Module::Infos node);

	static BuildContext *context();
	static Data *makeLibrary(const std::string &token);
	static Data *makeData(const std::string &token);
	static Data *makeArray();
	static Data *makeHash();

private:
	static BuildContext *g_ctx;
};

}

#endif // COMPIMER_H
