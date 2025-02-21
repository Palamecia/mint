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

#ifndef MINT_FILEPRINTER_H
#define MINT_FILEPRINTER_H

#include "mint/ast/printer.h"

#include <cstdio>

namespace mint {

class MINT_EXPORT FilePrinter : public Printer {
public:
	explicit FilePrinter(const char *path);
	explicit FilePrinter(int fd);
	FilePrinter(FilePrinter &&other) = delete;
	FilePrinter(const FilePrinter &other) = delete;
	~FilePrinter() override;

	FilePrinter &operator=(FilePrinter &&other) = delete;
	FilePrinter &operator=(const FilePrinter &other) = delete;

	void print(Reference &reference) override;

	[[nodiscard]] FILE *file() const;

protected:
	int internal_print(const char *str);

private:
	int (*m_print)(FILE *stream, const char *str);
	int (*m_close)(FILE *stream);
	FILE *m_stream;
};

}

#endif // MINT_FILEPRINTER_H
