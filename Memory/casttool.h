#ifndef CAST_TOOL_H
#define CAST_TOOL_H

#include "Memory/reference.h"
#include <queue>

class AbstractSynatxTree;

double to_number(AbstractSynatxTree *ast, const Reference &ref);
std::string to_string(const Reference &ref);
std::vector<Reference *> to_array(const Reference &ref);
std::map<Reference, Reference> to_hash(const Reference &ref);

void iterator_init(std::queue<SharedReference> &iterator, const Reference &ref);

#endif // CAST_TOOL_H
