/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#ifndef MINT_ERRNO_H
#define MINT_ERRNO_H

#include <errno.h>
#include <mint/config.h>

namespace mint {

class MINT_EXPORT SystemError {
public:
	SystemError(bool status);
	SystemError(const SystemError &other) noexcept;

	SystemError &operator=(const SystemError &other) noexcept;

#ifdef OS_WINDOWS
	static SystemError from_windows_last_error();
#endif

	operator bool() const;
	int get_errno() const;

private:
	SystemError(bool _status, int _errno);

	bool m_status;
	int m_errno;
};

#ifdef OS_WINDOWS
MINT_EXPORT int errno_from_windows_last_error();
#endif

}

#endif // MINT_ERRNO_H
