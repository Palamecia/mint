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

#ifndef MINTDOC_DOCPARSER_H
#define MINTDOC_DOCPARSER_H

#include "docnode.h"

#include <memory>
#include <sstream>
#include <vector>

class DocParser {
public:
	DocParser() = default;
	DocParser(const DocParser &) = delete;
	DocParser(DocParser &&) = delete;
	~DocParser() = default;

	DocParser &operator=(const DocParser &) = delete;
	DocParser &operator=(DocParser &&) = delete;

	[[nodiscard]] std::unique_ptr<DocNode> parse(std::stringstream &stream);

protected:
	[[nodiscard]] DocNodeSymbolLink::TagType get_tag_type(const std::string &tag) const;

private:
	std::unique_ptr<DocNode> m_root;
	std::vector<DocNode *> m_current;
	int m_line_number = 0;
	std::stringstream::off_type m_column = 0;
	std::stringstream::off_type m_first_nonspace = 0;
	std::stringstream::off_type m_first_nonspace_column = 0;
	std::stringstream::off_type m_thematic_break_kill_pos = 0;
	int m_indent = 0;
	bool m_blank = false;
	bool m_partially_consumed_tab = false;
	bool m_last_buffer_ended_with_cr = false;
};

#endif // MINTDOC_DOCPARSER_H
