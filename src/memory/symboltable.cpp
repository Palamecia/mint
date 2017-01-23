#include "memory/symboltable.h"
#include "memory/class.h"

SymbolTable::SymbolTable() : metadata(nullptr), defaultResult(Reference::const_ref | Reference::const_value) {}
