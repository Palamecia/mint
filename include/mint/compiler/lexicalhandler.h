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

#ifndef MINT_LEXICALHANDLER_H
#define MINT_LEXICALHANDLER_H

#include "mint/compiler/token.h"
#include "mint/system/datastream.h"

#include <istream>
#include <vector>

namespace mint {

class MINT_EXPORT AbstractLexicalHandlerStream : public DataStream {
public:
	std::string path() const override final;

	std::string::size_type find(const std::string &substr, std::string::size_type offset = 0) const noexcept;
	std::string::size_type find(const std::string::value_type ch, std::string::size_type offset = 0) const noexcept;
	std::string substr(std::string::size_type offset = 0, std::string::size_type count = std::string::npos) const noexcept;
	char operator [](std::string::size_type offset) const;
	size_t pos() const;

protected:
	int read_char() override final;
	int next_buffered_char() override final;

	virtual int get() = 0;

private:
	std::string m_script;
};

class MINT_EXPORT LexicalHandler {
public:
	virtual ~LexicalHandler() = default;

	bool parse(AbstractLexicalHandlerStream &script);
	bool parse(std::istream &script);

protected:
	virtual bool on_script_begin();
	virtual bool on_script_end();

	virtual bool on_comment_begin();
	virtual bool on_comment_end();

	virtual bool on_module_path_token(const std::vector<std::string> &context, const std::string &token, std::string::size_type offset);
	virtual bool on_symbol_token(const std::vector<std::string> &context, const std::string &token, std::string::size_type offset);
	virtual bool on_symbol_token(const std::vector<std::string> &context, std::string::size_type offset);
	virtual bool on_token(mint::token::Type type, const std::string &token, std::string::size_type offset);
	virtual bool on_white_space(const std::string &token, std::string::size_type offset);
	virtual bool on_comment(const std::string &token, std::string::size_type offset);

	virtual bool on_new_line(size_t line_number);
};

}

#endif // MINT_LEXICALHANDLER_H
