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

#ifndef MDBG_DEBUGPRINTER_H
#define MDBG_DEBUGPRINTER_H

#include <mint/ast/printer.h>
#include <string>

namespace mint {
class Reference;
struct Iterator;
struct Array;
struct Hash;
struct Function;
}

class DebugPrinter : public mint::Printer {
public:
	DebugPrinter();
	~DebugPrinter() override;

	void print(mint::Reference &reference) override;
};

std::string reference_value(const mint::Reference &reference);
std::string iterator_value(mint::Iterator *iterator);
std::string array_value(mint::Array *array);
std::string hash_value(mint::Hash *hash);
std::string function_value(mint::Function *function);

void print_debug_trace(const char *format, ...) __attribute__((format(printf, 1, 2)));

#endif // MDBG_DEBUGPRINTER_H
