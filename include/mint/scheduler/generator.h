/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#ifndef MINT_GENERATOR_H
#define MINT_GENERATOR_H

#include "mint/scheduler/process.h"

namespace mint {

class MINT_EXPORT Generator : public Process {
public:
	Generator(std::unique_ptr<SavedState> state, const Process *process);
	Generator(Generator &&) = delete;
	Generator(const Generator &) = delete;
	~Generator() override;

	Generator &operator=(Generator &&) = delete;
	Generator &operator=(const Generator &) = delete;

	void setup() override;
	void cleanup() override;

private:
	std::unique_ptr<SavedState> m_state;
};

MINT_EXPORT bool is_generator(Process *process);

}

#endif // MINT_GENERATOR_H
