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

#ifndef MINT_SAVEDSTATE_H
#define MINT_SAVEDSTATE_H

#include "mint/ast/cursor.h"

#include <stack>

namespace mint {

struct MINT_EXPORT SavedState {
	SavedState() = delete;
	SavedState(SavedState &&other) = delete;
	SavedState(const SavedState &other) = delete;
	SavedState(Cursor *cursor, Cursor::Context *context);
	~SavedState();

	SavedState &operator=(SavedState &&other) = delete;
	SavedState &operator=(const SavedState &other) = delete;
	
	Cursor *cursor;
	Cursor::Context *context;
	std::stack<Cursor::RetrievePoint> retrieve_points;
};

}

#endif // MINT_SAVEDSTATE_H
