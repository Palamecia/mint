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

#ifndef MINTDOC_DOCNODE_H
#define MINTDOC_DOCNODE_H

#include <cassert>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

struct Definition;

struct DocNode {
	enum Type : std::uint8_t {
		/* Block */
		node_document,
		node_block_quote,
		node_table,
		node_table_head,
		node_table_column,
		node_table_body,
		node_table_row,
		node_table_item,
		node_list,
		node_item,
		node_link,
		node_del,
		node_emph,
		node_strong,
		node_strong_emph,
		node_code_block,
		node_custom_block,
		node_paragraph,
		node_heading,

		/* Inline */
		node_text,
		node_code,
		node_html,
		node_softbreak,
		node_linebreak,
		node_thematic_break,
		node_custom_inline,
		node_image,
		node_symbol_link
	};

	static constexpr Type node_first_block = node_document;
	static constexpr Type node_last_block = node_heading;

	static constexpr Type node_first_inline = node_text;
	static constexpr Type node_last_inline = node_symbol_link;

	Type type;

	template<std::derived_from<DocNode> T>
	inline const T& as() const;
};

std::unique_ptr<DocNode> parse_doc(std::stringstream& stream);
std::unique_ptr<DocNode> parse_doc(const std::string& doc);

struct DocNodeBlock : public DocNode {
	std::vector<std::unique_ptr<DocNode>> children;
};

template<>
inline const DocNodeBlock& DocNode::as<DocNodeBlock>() const {
	assert(type >= node_first_block && type <= node_last_block);
	return static_cast<const DocNodeBlock&>(*this);
}

struct DocNodeBlockQuote : public DocNodeBlock {
	enum AlertType : std::uint8_t {
		alert_none,
		alert_note,
		alert_tip,
		alert_important,
		alert_warning,
		alert_caution,
	};

	AlertType alert_type;
};

template<>
inline const DocNodeBlockQuote& DocNode::as<DocNodeBlockQuote>() const {
	assert(type == node_block_quote);
	return static_cast<const DocNodeBlockQuote&>(*this);
}

struct DocNodeCodeBlock : public DocNodeBlock {
	std::optional<std::string> info;
	std::uint8_t fence_length;
	std::uint8_t fence_offset;
	char fence_char;
	bool fenced;
};

template<>
inline const DocNodeCodeBlock& DocNode::as<DocNodeCodeBlock>() const {
	assert(type == node_code_block);
	return static_cast<const DocNodeCodeBlock&>(*this);
}

struct DocNodeTableColumn : public DocNodeBlock {
	enum Align : std::uint8_t {
		align_auto,
		align_left,
		align_center,
		align_right
	};

	Align align;
};

template<>
inline const DocNodeTableColumn& DocNode::as<DocNodeTableColumn>() const {
	assert(type == node_table_column);
	return static_cast<const DocNodeTableColumn&>(*this);
}

struct DocNodeList : public DocNodeBlock {
	std::uint8_t indent;
	bool ordered;
};

template<>
inline const DocNodeList& DocNode::as<DocNodeList>() const {
	assert(type == node_list);
	return static_cast<const DocNodeList&>(*this);
}

struct DocNodeLink : public DocNodeBlock {
	std::string url;
	bool wiki_style;
};

template<>
inline const DocNodeLink& DocNode::as<DocNodeLink>() const {
	assert(type == node_link);
	return static_cast<const DocNodeLink&>(*this);
}

struct DocNodeHeading : public DocNodeBlock {
	std::int8_t level;
	bool setext;
};

template<>
inline const DocNodeHeading& DocNode::as<DocNodeHeading>() const {
	assert(type == node_heading);
	return static_cast<const DocNodeHeading&>(*this);
}

struct DocNodeLiteral : public DocNode {
	std::string str;
};

template<>
inline const DocNodeLiteral& DocNode::as<DocNodeLiteral>() const {
	assert(type == node_text || type == node_code || type == node_html);
	return static_cast<const DocNodeLiteral&>(*this);
}

struct DocNodeSymbolLink : public DocNode {
	enum TagType : std::uint8_t {
		no_tag,
		see_tag,
		module_tag
	};

	TagType tag_type;
	std::string symbol;
};

template<>
inline const DocNodeSymbolLink& DocNode::as<DocNodeSymbolLink>() const {
	assert(type == node_symbol_link);
	return static_cast<const DocNodeSymbolLink&>(*this);
}

std::string symbol_link_target(const DocNodeSymbolLink& node, const Definition* context = nullptr);

#endif // MINTDOC_DOCNODE_H
