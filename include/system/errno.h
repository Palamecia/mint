#ifndef MINT_ERRNO_H
#define MINT_ERRNO_H

#include <errno.h>
#include <config.h>

namespace mint {

#ifdef OS_WINDOWS
MINT_EXPORT int errno_from_windows_last_error();
#endif

}

#endif // MINT_ERRNO_H
