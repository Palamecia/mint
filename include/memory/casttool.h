#ifndef CAST_TOOL_H
#define CAST_TOOL_H

#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"

class AbstractSyntaxTree;

double to_number(AbstractSyntaxTree *ast, const Reference &ref);
bool to_boolean(AbstractSyntaxTree *ast, const Reference &ref);
std::string to_char(const Reference &ref);
std::string to_string(const Reference &ref);
Array::values_type to_array(const Reference &ref);
Hash::values_type to_hash(const Reference &ref);

#endif // CAST_TOOL_H
