#ifndef BLOCK_H
#define BLOCK_H

#include "compiler/buildtool.h"
#include "catchcontext.h"
#include "branch.h"

namespace mint {

struct Block {
	Block(BuildContext::BlockType type);

	BuildContext::BlockType type;
	Branch::ForwardNodeIndex *forward = nullptr;
	Branch::BackwardNodeIndex *backward = nullptr;
	CatchContext *catch_context = nullptr;
	CaseTable *case_table = nullptr;
	size_t retrievePointCount = 0;
	std::vector<Symbol *> *condition_scoped_symbols = nullptr;
	std::vector<Symbol *> block_scoped_symbols;

	bool is_breakable() const;
	bool is_continuable() const;
};

}

#endif // BLOCK_H
