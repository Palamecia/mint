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

#ifndef MINT_DEBUG_DEBUG_TOOLS_H
#define MINT_DEBUG_DEBUG_TOOLS_H

#include "mint/ast/node.h"
#include "mint/config.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <string_view>

namespace mint {

class Array;
class Boolean;
class Coroutine;
class Cursor;
class Function;
class Hash;
class Iterator;
class Number;
class Package;
class String;

MINT_EXPORT bool is_module_file(const std::filesystem::path& file_path);

MINT_EXPORT std::filesystem::path to_system_path(const std::string& module);
MINT_EXPORT std::string to_module_path(const std::filesystem::path& file_path);

MINT_EXPORT std::ifstream get_module_stream(const std::string& module);
MINT_EXPORT std::string get_module_line(const std::string& module, std::size_t line);

MINT_EXPORT Node::Command dump_command(Cursor& cursor, std::ostream& stream);

std::string to_debug_string(std::size_t offset);
std::string to_debug_string(std::string_view command);
std::string to_debug_string(Reference::Flags flags);
std::string to_debug_string(const Number& number);
std::string to_debug_string(const Boolean& boolean);
std::string to_debug_string(const String& string);
std::string to_debug_string(Cursor& cursor, const Array& array);
std::string to_debug_string(Cursor& cursor, const Hash& hash);
std::string to_debug_string(Cursor& cursor, const Iterator& iterator);
std::string to_debug_string(const Package& package);
std::string to_debug_string(Cursor& cursor, const Function& function);
std::string to_debug_string(Cursor& cursor, const Coroutine& coroutine);
std::string to_debug_string(Cursor& cursor, const Reference& constant);

}

#endif // MINT_DEBUG_DEBUG_TOOLS_H
