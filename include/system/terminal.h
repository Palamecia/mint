#ifndef TERMINAL_H
#define TERMINAL_H

#include <cstring>

void terminal_init();

char *readline(const char *prompt);
void add_history(const char *line);

#endif
