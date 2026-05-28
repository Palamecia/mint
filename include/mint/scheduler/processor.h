/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_SCHEDULER_PROCESSOR_H
#define MINT_SCHEDULER_PROCESSOR_H

#include "mint/config.h"
#include <cstdint>

namespace mint {

class Cursor;
class CursorDebugger;
class DebugInterface;

class MINT_EXPORT ProcessorLocker {
public:
	ProcessorLocker();
	ProcessorLocker(const ProcessorLocker&) = delete;
	ProcessorLocker(ProcessorLocker&&) = delete;
	~ProcessorLocker();

	ProcessorLocker& operator=(const ProcessorLocker&) = delete;
	ProcessorLocker& operator=(ProcessorLocker&&) = delete;
};

class MINT_EXPORT ProcessorUnlocker {
public:
	ProcessorUnlocker();
	ProcessorUnlocker(const ProcessorUnlocker&) = delete;
	ProcessorUnlocker(ProcessorUnlocker&&) = delete;
	~ProcessorUnlocker();

	ProcessorUnlocker& operator=(const ProcessorUnlocker&) = delete;
	ProcessorUnlocker& operator=(ProcessorUnlocker&&) = delete;
};

enum class ProcessStatus : std::uint8_t {
	paused,
	completed,
	failed,
};

MINT_EXPORT ProcessStatus debug_steps(CursorDebugger& cursor, DebugInterface& handle);
MINT_EXPORT ProcessStatus run_steps(Cursor& cursor);
MINT_EXPORT ProcessStatus run_step(Cursor& cursor);

MINT_EXPORT void set_multi_thread(bool enabled);
MINT_EXPORT void lock_processor();
MINT_EXPORT void unlock_processor();

}

#endif // MINT_SCHEDULER_PROCESSOR_H
