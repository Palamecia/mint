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

#ifndef MINTDOC_DOCPARSER_H
#define MINTDOC_DOCPARSER_H

#include "docnode.h"
#include "doclexer.h"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class DocParser {
public:
	DocParser() = default;
	DocParser(const DocParser &) = delete;
	DocParser(DocParser &&) = delete;
	~DocParser() = default;

	DocParser &operator=(const DocParser &) = delete;
	DocParser &operator=(DocParser &&) = delete;

	void parse_error(const char* error);
	[[nodiscard]] int next_token(std::string *token);
	[[nodiscard]] std::unique_ptr<DocNode> parse(std::stringstream &stream);

	static std::unique_ptr<DocNodeLiteral> create_text(const std::string &str);
	static DocNodeSymbolLink::TagType get_tag_type(const std::string &tag);

	void insert_node(std::unique_ptr<DocNode> &&node);

	void push_paragraph();
	void push_heading(std::int8_t level);
	void pop_node();

private:
	std::unique_ptr<DocNodeBlock> m_root;
	std::vector<DocNodeBlock *> m_current;
	std::unique_ptr<DocLexer> m_lexer;
};

#endif // MINTDOC_DOCPARSER_H
