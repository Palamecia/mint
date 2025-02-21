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

#ifndef MINT_PROCESS_COMPLETER_H
#define MINT_PROCESS_COMPLETER_H

#include "mint/compiler/lexicalhandler.h"
#include "mint/system/terminal.h"

#include <filesystem>
#include <string>

namespace mint {

class Cursor;
class Reference;
class PackageData;
class ClassDescription;

class Completer : public LexicalHandler {
public:
	Completer(std::vector<Completion> &completions, std::string_view::size_type offset, Cursor *cursor);
	Completer(Completer &&) = delete;
	Completer(const Completer &) = delete;
	~Completer() = default;

	Completer &operator=(Completer &&) = delete;
	Completer &operator=(const Completer &) = delete;

protected:
	bool on_module_path_token(const std::vector<std::string> &context, const std::string &token,
							  std::string::size_type offset) override;
	bool on_symbol_token(const std::vector<std::string> &context, const std::string &token,
						 std::string::size_type offset) override;
	bool on_symbol_token(const std::vector<std::string> &context, std::string::size_type offset) override;

	void find_module_recursive_helper(const std::filesystem::path &root_path,
									  const std::filesystem::path &directory_path,
									  const std::string &token_path);
	void find_context_symbols_helper(PackageData *pack, ClassDescription *desc, Reference *member,
									 const std::string &token, std::string::size_type offset);

	static bool token_match(const std::string &token, const std::string &pattern);
	static bool resolve_path(const std::vector<std::string> &context, PackageData *&pack, ClassDescription *&desc,
							 Reference *&member);

private:
	std::vector<Completion> &m_completions;
	std::string_view::size_type m_offset;
	Cursor *m_cursor;
};

}

#endif // MINT_PROCESS_COMPLETER_H
