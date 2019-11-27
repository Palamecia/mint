#ifndef TERMINAL_H
#define TERMINAL_H

#include "config.h"
#include "system/stdio.h"

#include <list>

namespace mint {

MINT_EXPORT void term_init();

MINT_EXPORT char *term_read_line(const char *prompt);
MINT_EXPORT void term_add_history(const char *line);
MINT_EXPORT void term_add_history(const char *line);
MINT_EXPORT void term_cprint(FILE *stream, const char *str);

}

#endif
