#include "context.h"

using namespace mint;

int mint::fast_symbol_index(Definition *def, Symbol *symbol) {

	auto i = def->fastSymbolIndexes.find(*symbol);
	if (i != def->fastSymbolIndexes.end()) {
		return i->second;
	}

	int index = static_cast<int>(def->fastSymbolIndexes.size());
	return def->fastSymbolIndexes[*symbol] = index;
}
