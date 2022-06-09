#ifndef MINT_MALLOC_H
#define MINT_MALLOC_H

#include "config.h"

#include <cstddef>

namespace mint {

MINT_EXPORT size_t malloc_size(void *ptr);

}

#endif // MINT_MALLOC_H
