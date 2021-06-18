#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "config.h"

#include <cstddef>

namespace mint {

class Cursor;
class DebugInterface;

MINT_EXPORT bool debug_steps(Cursor *cursor, DebugInterface *interface);
MINT_EXPORT bool run_steps(Cursor *cursor);
MINT_EXPORT bool run_step(Cursor *cursor);

MINT_EXPORT void set_multi_thread(bool enabled);
MINT_EXPORT void lock_processor();
MINT_EXPORT void unlock_processor();

}

#endif // PROCESSOR_H
