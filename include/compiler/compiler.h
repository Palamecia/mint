#ifndef COMPIMER_H
#define COMPIMER_H

#include "buildtool.h"

#ifdef BUILD_TYPE_DEBUG
#define DEBUG_STACK(context, msg, ...) printf("[%08lx] " msg "\n", context->data.module->nextNodeOffset(), ##__VA_ARGS__)
#else
#define DEBUG_STACK(context, msg, ...) ((void)0)
#endif

namespace mint {

class MINT_EXPORT Compiler {
public:
	Compiler();

	bool build(DataStream *stream, Module::Infos node);

	static Data *makeLibrary(const std::string &token);
	static Data *makeData(const std::string &token);
	static Data *makeArray();
	static Data *makeHash();
};

}

#endif // COMPIMER_H
