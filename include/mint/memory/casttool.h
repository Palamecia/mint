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

#ifndef MINT_CASTTOOL_H
#define MINT_CASTTOOL_H

#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"

#include <regex>

namespace mint {

class Cursor;

MINT_EXPORT double to_unsigned_number(const std::string &str, bool *error = nullptr);
MINT_EXPORT double to_signed_number(const std::string &str, bool *error = nullptr);

MINT_EXPORT intmax_t to_integer(double value);
MINT_EXPORT intmax_t to_integer(Cursor *cursor, Reference &ref);
MINT_EXPORT intmax_t to_integer(Cursor *cursor, Reference &&ref);
MINT_EXPORT double to_number(Cursor *cursor, Reference &ref);
MINT_EXPORT double to_number(Cursor *cursor, Reference &&ref);
MINT_EXPORT bool to_boolean(Cursor *cursor, Reference &ref);
MINT_EXPORT bool to_boolean(Cursor *cursor, Reference &&ref);
MINT_EXPORT std::string to_char(const Reference &ref);
MINT_EXPORT std::string to_string(const Reference &ref);
MINT_EXPORT std::regex to_regex(Reference &ref);
MINT_EXPORT Array::values_type to_array(Reference &ref);
MINT_EXPORT Hash::values_type to_hash(Cursor *cursor, Reference &ref);

}

#endif // MINT_CASTTOOL_H
