#ifndef UNIX_TERMINAL_H
#define UNIX_TERMINAL_H

#include "mint/config.h"

#include <termios.h>
#include <optional>
#include <vector>
#include <chrono>

namespace mint {

struct Tty;
struct TerminalInfo;
struct CursorPos;

termios term_setup_mode();
void term_reset_mode(termios mode);

bool term_update_dim(TerminalInfo *term);
bool term_get_cursor_pos(CursorPos *pos);
bool term_set_cursor_pos(const CursorPos &pos);

size_t term_get_tab_width(size_t column);

void term_read_input(Tty *tty, std::optional<std::chrono::milliseconds> timeout = std::nullopt);

}

#endif // UNIX_TERMINAL_H
