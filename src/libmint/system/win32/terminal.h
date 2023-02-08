#ifndef WIN32_TERMINAL_H
#define WIN32_TERMINAL_H

#include "mint/config.h"

#include <Windows.h>
#include <optional>
#include <chrono>

namespace mint {

struct tty_t;
struct term_t;
struct cursor_pos_t;

DWORD term_setup_mode();
void term_reset_mode(DWORD mode);

char *term_readline(const char *prompt);
void term_read_input(tty_t *tty, std::optional<std::chrono::milliseconds> timeout = std::nullopt);

bool term_update_dim(term_t *term);
bool term_get_cursor_pos(cursor_pos_t *pos);
bool term_set_cursor_pos(const cursor_pos_t &pos);

size_t term_get_tab_width(size_t column);

int WriteMultiByteToConsoleW(HANDLE hConsoleOutput, const char *str, int cbMultiByte = -1);
int WriteCharsToConsoleW(HANDLE hConsoleOutput, wchar_t wc, int cbRepeat = 1);

bool vt100_enabled_for_console(HANDLE hTerminal);
const char *handle_vt100_sequence(HANDLE hTerminal, const char *cptr);
int handle_format_flags(HANDLE hTerminal, const char **format, va_list *argptr);

};

#endif // WIN32_TERMINAL_H
