#ifndef CATCHCONTEXT_H
#define CATCHCONTEXT_H

#include "ast/symbol.h"

namespace mint {

struct CatchContext {
	CatchContext();
	~CatchContext();

	Symbol *symbol = nullptr;
};

}

#endif // CATCHCONTEXT_H
