#ifndef MINT_ERROR_H
#define MINT_ERROR_H

#include "system/mintsystemerror.hpp"
#include "config.h"

#include <functional>

namespace mint {

MINT_EXPORT MINT_NORETURN void error(const char *format, ...) __attribute__((format(printf, 1, 2)));

MINT_EXPORT int add_error_callback(std::function<void(void)> on_error);
MINT_EXPORT void remove_error_callback(int id);
MINT_EXPORT void call_error_callbacks();

MINT_EXPORT void set_exit_callback(const std::function<void(void)> &on_exit);
MINT_EXPORT void call_exit_callback();

}

#endif // MINT_ERROR_H
