#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "config.h"

namespace mint {

class Cursor;

MINT_EXPORT bool run_step(Cursor *cursor);

}

#endif // PROCESSOR_H
