#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "config.h"

namespace mint {

class Cursor;

MINT_EXPORT bool run_step(Cursor *cursor);
MINT_EXPORT void lock_processor();
MINT_EXPORT void unlock_processor();

}

#endif // PROCESSOR_H
