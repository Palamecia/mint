#ifndef COMPIMER_H
#define COMPIMER_H

#include "buildtool.h"

class Compiler {
public:
	Compiler();

	bool build(DataStream *stream, Modul::Context node);

	static BuildContext *context();
	static Data *makeData(const std::string &token);

private:
	static BuildContext *g_ctx;
};

#endif // COMPIMER_H
