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

#ifndef MINT_SCHEDULER_EXCEPTION_H
#define MINT_SCHEDULER_EXCEPTION_H

#include "mint/ast/cursor.h"
#include "mint/config.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/process.h"
#include <exception>
#include <functional>
#include <utility>

namespace mint {

class MINT_EXPORT Exception : public Process {
public:
	Exception(Reference&& reference, const Process& process);
	Exception(Exception&&) = delete;
	Exception(const Exception&) = delete;
	~Exception() override;

	Exception& operator=(Exception&&) = delete;
	Exception& operator=(const Exception&) = delete;

	void setup() override;
	void cleanup() override;

private:
	StrongReference _reference;
	bool _handled;
};

MINT_EXPORT bool is_exception(Process& process);

class MintException : public std::exception {
public:
	MintException(Cursor& cursor, Reference&& reference) :
	    _cursor(cursor),
	    _reference(std::move(reference)) {}

	Cursor& cursor() {
		return _cursor;
	}

	Reference&& take_exception() noexcept {
		return std::move(_reference);
	}

	[[nodiscard]] const char* what() const noexcept override {
		return "MintException";
	}

private:
	std::reference_wrapper<Cursor> _cursor;
	StrongReference _reference;
};

}

#endif // MINT_SCHEDULER_EXCEPTION_H
