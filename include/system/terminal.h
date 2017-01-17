#ifndef TERMINAL_H
#define TERMINAL_H

#include <cstring>

char *readline(const char *prompt);
void add_history(const char *line);

#endif
