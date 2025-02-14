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

#ifndef MINTDOC_DOCNODE_H
#define MINTDOC_DOCNODE_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

class Definition;

struct DocNode {
	enum Type : std::uint8_t {
		/* Block */
		NODE_DOCUMENT,
		NODE_BLOCK_QUOTE,
		NODE_TABLE,
		NODE_TABLE_HEAD,
		NODE_TABLE_COLUMN,
		NODE_TABLE_BODY,
		NODE_TABLE_ROW,
		NODE_TABLE_ITEM,
		NODE_LIST,
		NODE_ITEM,
		NODE_LINK,
		NODE_DEL,
		NODE_EMPH,
		NODE_STRONG,
		NODE_STRONG_EMPH,
		NODE_CODE_BLOCK,
		NODE_CUSTOM_BLOCK,
		NODE_PARAGRAPH,
		NODE_HEADING,
		
		/* Inline */
		NODE_TEXT,
		NODE_CODE,
		NODE_HTML,
		NODE_SOFTBREAK,
		NODE_LINEBREAK,
		NODE_THEMATIC_BREAK,
		NODE_CUSTOM_INLINE,
		NODE_IMAGE,
		NODE_SYMBOL_LINK
	};

	static constexpr Type NODE_FIRST_BLOCK = NODE_DOCUMENT;
	static constexpr Type NODE_LAST_BLOCK = NODE_HEADING;

	static constexpr Type NODE_FIRST_INLINE = NODE_TEXT;
	static constexpr Type NODE_LAST_INLINE = NODE_SYMBOL_LINK;

	Type type;

	template<class T, typename = std::enable_if_t<std::is_base_of_v<DocNode, T>>>
	inline const T *as() const;
};

std::unique_ptr<DocNode> parse_doc(std::stringstream &stream);
std::unique_ptr<DocNode> parse_doc(const std::string &doc);

struct DocNodeBlock : public DocNode {
	std::vector<std::unique_ptr<DocNode>> children;
};

template<>
inline const DocNodeBlock *DocNode::as<DocNodeBlock>() const {
	assert(type >= NODE_FIRST_BLOCK && type <= NODE_LAST_BLOCK);
	return static_cast<const DocNodeBlock *>(this);
}

struct DocNodeBlockQuote : public DocNodeBlock {
	enum AlertType : std::uint8_t {
		ALERT_NONE,
		ALERT_NOTE,
		ALERT_TIP,
		ALERT_IMPORTANT,
		ALERT_WARNING,
		ALERT_CAUTION,
	};

	AlertType alert_type;
};

template<>
inline const DocNodeBlockQuote *DocNode::as<DocNodeBlockQuote>() const {
	assert(type == NODE_BLOCK_QUOTE);
	return static_cast<const DocNodeBlockQuote *>(this);
}

struct DocNodeCodeBlock : public DocNodeBlock {
	std::optional<std::string> info;
	std::uint8_t fence_length;
	std::uint8_t fence_offset;
	char fence_char;
	bool fenced;
};

template<>
inline const DocNodeCodeBlock *DocNode::as<DocNodeCodeBlock>() const {
	assert(type == NODE_CODE_BLOCK);
	return static_cast<const DocNodeCodeBlock *>(this);
}

struct DocNodeTableColumn : public DocNodeBlock {
	enum Align : std::uint8_t {
		ALIGN_AUTO,
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};
	Align align;
};

template<>
inline const DocNodeTableColumn *DocNode::as<DocNodeTableColumn>() const {
	assert(type == NODE_TABLE_COLUMN);
	return static_cast<const DocNodeTableColumn *>(this);
}

struct DocNodeList : public DocNodeBlock {
	std::uint8_t indent;
	bool ordered;
};

template<>
inline const DocNodeList *DocNode::as<DocNodeList>() const {
	assert(type == NODE_LIST);
	return static_cast<const DocNodeList *>(this);
}

struct DocNodeLink : public DocNodeBlock {
	std::string url;
	bool wiki_style;
};

template<>
inline const DocNodeLink *DocNode::as<DocNodeLink>() const {
	assert(type == NODE_LINK);
	return static_cast<const DocNodeLink *>(this);
}

struct DocNodeHeading : public DocNodeBlock {
	std::int8_t level;
	bool setext;
};

template<>
inline const DocNodeHeading *DocNode::as<DocNodeHeading>() const {
	assert(type == NODE_HEADING);
	return static_cast<const DocNodeHeading *>(this);
}

struct DocNodeLiteral : public DocNode {
	std::string str;
};

template<>
inline const DocNodeLiteral *DocNode::as<DocNodeLiteral>() const {
	assert(type == NODE_TEXT || type == NODE_CODE || type == NODE_HTML);
	return static_cast<const DocNodeLiteral *>(this);
}

struct DocNodeSymbolLink : public DocNode {
	enum TagType : std::uint8_t {
		NO_TAG,
		SEE_TAG,
		MODULE_TAG
	};

	TagType tag_type;
	std::string symbol;
};

template<>
inline const DocNodeSymbolLink *DocNode::as<DocNodeSymbolLink>() const {
	assert(type == NODE_SYMBOL_LINK);
	return static_cast<const DocNodeSymbolLink *>(this);
}

std::string symbol_link_target(const DocNodeSymbolLink *node, const Definition *context = nullptr);

#endif // MINTDOC_DOCNODE_H
