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

#ifndef MINT_DEBUGTOOL_H
#define MINT_DEBUGTOOL_H

#include "mint/ast/node.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace mint {

class Cursor;

MINT_EXPORT bool is_module_file(const std::filesystem::path &file_path);

MINT_EXPORT std::filesystem::path to_system_path(const std::string &module);
MINT_EXPORT std::string to_module_path(const std::filesystem::path &file_path);

MINT_EXPORT std::ifstream get_module_stream(const std::string &module);
MINT_EXPORT std::string get_module_line(const std::string &module, size_t line);

MINT_EXPORT void dump_command(size_t offset, Node::Command command, Cursor *cursor, std::ostream &stream);

}

#endif // MINT_DEBUGTOOL_H
