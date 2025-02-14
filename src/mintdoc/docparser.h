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

#include <cstddef>
#include <cstdint>
#include <initializer_list>
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

	[[nodiscard]] std::unique_ptr<DocNode> parse(std::stringstream &stream);

protected:
	static std::unique_ptr<DocNodeBlock> create_block(DocNode::Type type);
	static std::unique_ptr<DocNodeBlockQuote> create_block_quote();
	static std::unique_ptr<DocNodeTableColumn> create_table_column(DocNodeTableColumn::Align align);
	static std::unique_ptr<DocNodeList> create_list(std::uint8_t indent, bool ordered);
	static std::unique_ptr<DocNodeLink> create_link(bool wiki_style);
	static std::unique_ptr<DocNodeCodeBlock> create_code_block(std::uint8_t fence_length);
	static std::unique_ptr<DocNodeHeading> create_heading(std::int8_t level);
	static std::unique_ptr<DocNode> create_node(DocNode::Type type);
	static std::unique_ptr<DocNodeLiteral> create_text(const std::string &str);
	static std::unique_ptr<DocNodeLiteral> create_code(const std::string &str);
	static std::unique_ptr<DocNodeLiteral> create_html(const std::string &str);
	static std::unique_ptr<DocNodeSymbolLink> make_symbol_link(DocNodeSymbolLink::TagType tag_type,
															   const std::string &symbol);
	static std::vector<std::unique_ptr<DocNode>> make_node_list(std::unique_ptr<DocNode> &&node);
	static DocNodeSymbolLink::TagType get_tag_type(const std::string &tag);

	class Delimiter {
	public:
		Delimiter(DocLexer::Token delimiter);
		Delimiter(std::initializer_list<DocLexer::Token> delimiters);

		bool operator==(DocLexer::Token token) const;
		bool operator!=(DocLexer::Token token) const;

	private:
		std::vector<DocLexer::Token> m_delimiters;
	};

	std::unique_ptr<DocNode> parse_block_quote(DocLexer::Token &type, std::string &token);
	std::vector<std::unique_ptr<DocNode>> parse_table();
	std::unique_ptr<DocNode> parse_unordered_list(DocLexer::Token &type, std::string &token, std::size_t &column,
												  std::uint8_t indent);
	std::unique_ptr<DocNode> parse_ordered_list(DocLexer::Token &type, std::string &token, std::size_t &column,
												std::uint8_t indent);
	std::vector<std::unique_ptr<DocNode>> parse_link(DocLexer::Token &type, std::string &token);
	std::vector<std::unique_ptr<DocNode>> parse_wiki_link(DocLexer::Token &type, std::string &token);
	std::unique_ptr<DocNode> parse_heading();
	std::unique_ptr<DocNode> parse_paragraph(DocLexer::Token &type, std::string &token);
	std::unique_ptr<DocNode> parse_thematic_break(DocLexer::Token &type, std::string &token);
	std::vector<std::unique_ptr<DocNode>> parse_text(const Delimiter &delimiter);
	std::vector<std::unique_ptr<DocNode>> parse_text(DocLexer::Token &type, std::string &token,
													 const Delimiter &delimiter);
	std::unique_ptr<DocNode> parse_format_block(DocLexer::Token &type, std::string &token, DocNode::Type format,
												const Delimiter &delimiter, std::string text);
	std::unique_ptr<DocNode> parse_code(const Delimiter &delimiter, std::uint8_t fence_length, std::size_t column);
	std::vector<std::unique_ptr<DocNode>> parse_html(DocLexer::Token &type, std::string &token);

	std::string parse_url(const Delimiter &delimiter);

	std::unique_ptr<DocNode> parse_symbol_link();

	static std::vector<std::unique_ptr<DocNode>> join_table_nodes(
		std::vector<std::vector<std::unique_ptr<DocNode>>> &&node_lists);

private:
	std::unique_ptr<DocLexer> m_lexer;
};

#endif // MINTDOC_DOCPARSER_H
