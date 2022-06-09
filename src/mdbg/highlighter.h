#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <fstream>

void print_highlighted(size_t from_line, size_t to_line, size_t current_line, std::ifstream &&script);

#endif // HIGHLIGHTER_H
