#ifndef CONTEXT_H
#define CONTEXT_H

#include "ast/symbolmapping.hpp"
#include "branch.h"

#include <vector>
#include <stack>
#include <list>

namespace mint {

class Banch;
class ClassDescription;

struct Block;

static constexpr const size_t InvalidOffset = static_cast<size_t>(-1);

struct Context {
	std::stack<ClassDescription *> classes;
	std::list<Block *> blocks;
	size_t printers = 0;
	size_t packages = 0;
	std::unique_ptr<std::vector<Symbol *>> condition_scoped_symbols;
};

struct Parameter {
	Reference::Flags flags;
	Symbol *symbol;
};

struct Definition : public Context {
	std::vector<Branch::BackwardNodeIndex> exitPoints;
	SymbolMapping<int> fastSymbolIndexes;
	std::stack<Parameter> parameters;
	size_t beginOffset = InvalidOffset;
	size_t retrievePointCount = 0;
	Reference *function = nullptr;
	Branch *capture = nullptr;
	bool capture_all = false;
	bool with_fast = true;
	bool variadic = false;
	bool generator = false;
	bool returned = false;
};

int find_fast_symbol_index(Definition *def, Symbol *symbol);
int fast_symbol_index(Definition *def, Symbol *symbol);

}

#endif // CONTEXT_H
