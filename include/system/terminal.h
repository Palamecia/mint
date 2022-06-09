#ifndef MINT_TERMINAL_H
#define MINT_TERMINAL_H

#include "system/stdio.h"
#include "config.h"

namespace mint {

enum StdStreamFileNo {
	stdin_fileno = 0,
	stdout_fileno = 1,
	stderr_fileno = 2
};

MINT_EXPORT void term_init();

MINT_EXPORT char *term_read_line(const char *prompt);
MINT_EXPORT void term_add_history(const char *line);
MINT_EXPORT void term_add_history(const char *line);
MINT_EXPORT int term_printf(FILE *stream, const char *format, ...) __attribute__((format(printf, 2, 3)));
MINT_EXPORT int term_vprintf(FILE *stream, const char *format, va_list args);
MINT_EXPORT int term_print(FILE *stream, const char *str);
MINT_EXPORT bool is_term(FILE *stream);
MINT_EXPORT bool is_term(int fd);

}

#endif // MINT_TERMINAL_H
