#ifndef CAST_TOOL_H
#define CAST_TOOL_H

#include "Memory/reference.h"
#include <queue>

double to_number(const Reference &ref);
std::string to_string(const Reference &ref);

void iterator_init(std::queue<SharedReference> &iterator, const Reference &ref);

#endif // CAST_TOOL_H
