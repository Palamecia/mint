#ifndef MINT_TERMINAL_H
#define MINT_TERMINAL_H

#include "system/stdio.h"
#include "config.h"

#include <list>

namespace mint {

MINT_EXPORT void term_init();

MINT_EXPORT char *term_read_line(const char *prompt);
MINT_EXPORT void term_add_history(const char *line);
MINT_EXPORT void term_add_history(const char *line);
MINT_EXPORT void term_cprint(FILE *stream, const char *str);

}

#endif // MINT_TERMINAL_H
