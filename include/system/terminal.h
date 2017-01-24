#ifndef TERMINAL_H
#define TERMINAL_H

void term_init();

char *term_read_line(const char *prompt);
void term_add_history(const char *line);

#endif
