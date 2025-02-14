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

#ifndef MINTDOC_DOCNODE_H
#define MINTDOC_DOCNODE_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

class Definition;

struct DocNode {
	enum Type : std::uint8_t {
		/* Error status */
		NODE_NONE,

		/* Block */
		NODE_DOCUMENT,
		NODE_BLOCK_QUOTE,
		NODE_LIST,
		NODE_ITEM,
		NODE_CODE_BLOCK,
		NODE_HTML_BLOCK,
		NODE_CUSTOM_BLOCK,
		NODE_PARAGRAPH,
		NODE_HEADING,
		NODE_THEMATIC_BREAK,

		/* Inline */
		NODE_TEXT,
		NODE_SOFTBREAK,
		NODE_LINEBREAK,
		NODE_CODE,
		NODE_HTML_INLINE,
		NODE_CUSTOM_INLINE,
		NODE_EMPH,
		NODE_STRONG,
		NODE_LINK,
		NODE_IMAGE,
		NODE_SYMBOL_LINK
	};

	static constexpr Type NODE_FIRST_BLOCK = NODE_DOCUMENT;
	static constexpr Type NODE_LAST_BLOCK = NODE_THEMATIC_BREAK;

	static constexpr Type NODE_FIRST_INLINE = NODE_TEXT;
	static constexpr Type NODE_LAST_INLINE = NODE_IMAGE;

	Type type;
	std::unique_ptr<DocNode> next;

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

struct DocNodeLiteral : public DocNode {
	std::string str;
};

template<>
inline const DocNodeLiteral *DocNode::as<DocNodeLiteral>() const {
	assert(type == NODE_TEXT || type == NODE_CODE || type == NODE_HTML_INLINE);
	return static_cast<const DocNodeLiteral *>(this);
}

struct DocNodeLink : public DocNode {
	std::string url;
	std::string title;
};

template<>
inline const DocNodeLink *DocNode::as<DocNodeLink>() const {
	assert(type == NODE_LINK);
	return static_cast<const DocNodeLink *>(this);
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
