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

#ifndef MINT_CLASSTOOL_H
#define MINT_CLASSTOOL_H

#include "mint/memory/class.h"

#include <initializer_list>
#include <utility>
#include <cstdint>

namespace mint {

MINT_EXPORT Class *create_enum(const std::string &name, std::initializer_list<std::pair<Symbol, std::optional<intmax_t>>> values);
MINT_EXPORT Class *create_enum(PackageData *package, const std::string &name, std::initializer_list<std::pair<Symbol, std::optional<intmax_t>>> values);

MINT_EXPORT Class *create_class(const std::string &name, std::initializer_list<std::pair<Symbol, Reference&&>> members);
MINT_EXPORT Class *create_class(PackageData *package, const std::string &name, std::initializer_list<std::pair<Symbol, Reference&&>> members);
MINT_EXPORT Class *create_class(const std::string &name, std::initializer_list<ClassDescription *> bases, std::initializer_list<std::pair<Symbol, Reference&&>> members);
MINT_EXPORT Class *create_class(PackageData *package, const std::string &name, std::initializer_list<ClassDescription *> bases, std::initializer_list<std::pair<Symbol, Reference&&>> members);

}

#endif // MINT_CLASSTOOL_H
