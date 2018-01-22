#ifndef ERROR_H
#define ERROR_H

#include "config.h"

#include <functional>
#include <exception>

namespace mint {

class MINT_EXPORT MintSystemError : std::exception {
public:
	MintSystemError();
};

MINT_EXPORT void error(const char *format, ...);

MINT_EXPORT int add_error_callback(std::function<void(void)> on_error);
MINT_EXPORT void remove_error_callback(int id);

MINT_EXPORT void set_exit_callback(std::function<void(void)> on_exit);

}

#endif // ERROR_H
