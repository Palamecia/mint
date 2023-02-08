#ifndef UNIX_TERMINAL_H
#define UNIX_TERMINAL_H

#include "mint/config.h"

#include <termios.h>
#include <optional>
#include <vector>
#include <chrono>

namespace mint {

struct tty_t;
struct term_t;
struct cursor_pos_t;

termios term_setup_mode();
void term_reset_mode(termios mode);

bool term_update_dim(term_t *term);
bool term_get_cursor_pos(cursor_pos_t *pos);
bool term_set_cursor_pos(const cursor_pos_t &pos);

size_t term_get_tab_width(size_t column);

void term_read_input(tty_t *tty, std::optional<std::chrono::milliseconds> timeout = std::nullopt);

}

#endif // UNIX_TERMINAL_H
