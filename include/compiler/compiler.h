#ifndef COMPIMER_H
#define COMPIMER_H

#include "buildtool.h"

#ifdef BUILD_TYPE_DEBUG
#define DEBUG_METADATA(context, msg, ...) printf("[" msg "]\n", ##__VA_ARGS__)
#define DEBUG_STACK(context, msg, ...) printf("[%08lx] " msg "\n", context->data.module->nextNodeOffset(), ##__VA_ARGS__)
#else
#define DEBUG_METADATA(context, msg, ...) ((void)0)
#define DEBUG_STACK(context, msg, ...) ((void)0)
#endif

namespace mint {

class MINT_EXPORT Compiler {
public:
	Compiler();

	bool isPrinting() const;
	void setPrinting(bool enabled);

	bool build(DataStream *stream, Module::Infos node);

	static Data *makeLibrary(const std::string &token);
	static Data *makeData(const std::string &token);
	static Data *makeArray();
	static Data *makeHash();
	static Data *makeNone();

private:
	bool m_printing;
};

}

#endif // COMPIMER_H
