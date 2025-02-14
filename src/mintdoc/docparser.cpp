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

#include "docparser.h"
#include "doclexer.h"
#include "docnode.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr const std::size_t CODE_INDENT = 4;

std::vector<std::unique_ptr<DocNode>> &extend_nodes(std::vector<std::unique_ptr<DocNode>> &nodes,
													std::vector<std::unique_ptr<DocNode>> &&items) {
	std::move(std::begin(items), std::end(items), std::back_inserter(nodes));
	return nodes;
}

std::vector<std::unique_ptr<DocNode>> &extend_text_nodes(std::vector<std::unique_ptr<DocNode>> &text,
														 std::vector<std::unique_ptr<DocNode>> &&items) {
	auto it = std::begin(items);
	auto end = std::end(items);
	if (!text.empty() && text.back()->type == DocNode::NODE_TEXT && it != end && (*it)->type == DocNode::NODE_TEXT) {
		static_cast<DocNodeLiteral *>(text.back().get())->str += (*it++)->as<DocNodeLiteral>()->str;
	}
	std::move(it, end, std::back_inserter(text));
	return text;
}

std::vector<std::unique_ptr<DocNode>> &extend_text_nodes(std::vector<std::unique_ptr<DocNode>> &text,
														 std::unique_ptr<DocNode> &&item) {
	if (!text.empty() && text.back()->type == DocNode::NODE_TEXT && item->type == DocNode::NODE_TEXT) {
		static_cast<DocNodeLiteral *>(text.back().get())->str += item->as<DocNodeLiteral>()->str;
	}
	else {
		text.emplace_back(std::move(item));
	}
	return text;
}

bool is_blank(const std::vector<std::unique_ptr<DocNode>> &nodes) {
	return std::none_of(std::begin(nodes), std::end(nodes), [](const std::unique_ptr<DocNode> &node) {
		if (node->type == DocNode::NODE_LINEBREAK) {
			return false;
		}
		if (node->type == DocNode::NODE_TEXT) {
			const std::string &str = node->as<DocNodeLiteral>()->str;
			return std::any_of(std::begin(str), std::end(str), [](char c) {
				return c != ' ' && c != '\t';
			});
		}
		return true;
	});
}

bool read_alert_type(const std::string &token, DocNodeBlockQuote::AlertType &alert_type) {
	static const std::unordered_map<std::string, DocNodeBlockQuote::AlertType> g_alerts = {
		{"[!NOTE]", DocNodeBlockQuote::ALERT_NOTE},			  {"[!TIP]", DocNodeBlockQuote::ALERT_TIP},
		{"[!IMPORTANT]", DocNodeBlockQuote::ALERT_IMPORTANT}, {"[!WARNING]", DocNodeBlockQuote::ALERT_WARNING},
		{"[!CAUTION]", DocNodeBlockQuote::ALERT_CAUTION},
	};
	if (auto i = g_alerts.find(token.substr(0, token.find_first_of(" \t"))); i != g_alerts.end()) {
		alert_type = i->second;
		return true;
	}
	return false;
}

bool read_align(const std::string &token, DocNodeTableColumn::Align &align) {
	int hyphen_count = 0;
	auto pos = token.find_first_not_of(" \t");
	if (pos == std::string::npos) {
		return false;
	}
	while (pos < token.size()) {
		switch (token[pos++]) {
		case ':':
			if (hyphen_count == 0) {
				align = DocNodeTableColumn::ALIGN_LEFT;
			}
			else {
				if (align == DocNodeTableColumn::ALIGN_LEFT) {
					align = DocNodeTableColumn::ALIGN_CENTER;
				}
				else {
					align = DocNodeTableColumn::ALIGN_RIGHT;
				}
				pos = token.find_first_not_of(" \t", pos);
				if (pos != std::string::npos) {
					return false;
				}
			}
			break;
		case '-':
			++hyphen_count;
			break;
		case ' ':
		case '\t':
			pos = token.find_first_not_of(" \t", pos);
			if (pos != std::string::npos) {
				return false;
			}
			break;
		default:
			return false;
		}
	}
	return true;
}

std::string text_to_url(std::vector<std::unique_ptr<DocNode>> &&text) {
	std::string url;
	for (auto &&node : text) {
		switch (node->type) {
		case DocNode::NODE_TEXT:
			if (const auto *node_data = node->as<DocNodeLiteral>()) {
				url += node_data->str;
			}
			break;
		default:
			assert(false);
		}
	}
	return url;
}
}

std::unique_ptr<DocNode> DocParser::parse(std::stringstream &stream) {

	m_lexer = std::make_unique<DocLexer>(stream);
	auto root = create_block(DocNode::NODE_DOCUMENT);
	std::size_t column = 0;

	for (;;) {

		auto [type, token] = m_lexer->next_token();

		switch (type) {
		case DocLexer::SHARP_TOKEN:
			root->children.emplace_back(parse_heading());
			break;
		case DocLexer::PIPE_TOKEN:
			extend_nodes(root->children, parse_table());
			break;
		case DocLexer::RIGHT_ANGLED_TOKEN:
			root->children.emplace_back(parse_block_quote(type, token));
			root->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
			break;
		case DocLexer::BLANK_TOKEN:
			if (m_lexer->get_column_number() - 1 >= CODE_INDENT) {
				root->children.emplace_back(parse_code(DocLexer::LINE_BREAK_TOKEN, 0, column));
			}
			else {
				root->children.emplace_back(parse_paragraph(type, token));
			}
			break;
		case DocLexer::TPL_HYPHEN_TOKEN:
		case DocLexer::TPL_ASTERISK_TOKEN:
		case DocLexer::TPL_UNDERSCORE_TOKEN:
			root->children.emplace_back(parse_thematic_break(type, token));
			break;
		case DocLexer::LINE_BREAK_TOKEN:
			break;
		case DocLexer::FILE_END_TOKEN:
			return std::move(root);
		default:
			root->children.emplace_back(parse_paragraph(type, token));
		}
	}

	return {};
}

std::unique_ptr<DocNodeBlock> DocParser::create_block(DocNode::Type type) {
	assert(type >= DocNode::NODE_FIRST_BLOCK && type <= DocNode::NODE_LAST_BLOCK);
	auto node = std::make_unique<DocNodeBlock>();
	node->type = type;
	return node;
}

std::unique_ptr<DocNodeBlockQuote> DocParser::create_block_quote() {
	auto node = std::make_unique<DocNodeBlockQuote>();
	node->type = DocNode::NODE_BLOCK_QUOTE;
	node->alert_type = DocNodeBlockQuote::ALERT_NONE;
	return node;
}

std::unique_ptr<DocNodeTableColumn> DocParser::create_table_column(DocNodeTableColumn::Align align) {
	auto node = std::make_unique<DocNodeTableColumn>();
	node->type = DocNode::NODE_TABLE_COLUMN;
	node->align = align;
	return node;
}

std::unique_ptr<DocNodeList> DocParser::create_list(std::uint8_t indent, bool ordered) {
	auto node = std::make_unique<DocNodeList>();
	node->type = DocNode::NODE_LIST;
	node->indent = indent;
	node->ordered = ordered;
	return node;
}

std::unique_ptr<DocNodeLink> DocParser::create_link(bool wiki_style) {
	auto node = std::make_unique<DocNodeLink>();
	node->type = DocNode::NODE_LINK;
	node->wiki_style = wiki_style;
	return node;
}

std::unique_ptr<DocNodeCodeBlock> DocParser::create_code_block(std::uint8_t fence_length) {
	auto node = std::make_unique<DocNodeCodeBlock>();
	node->type = DocNode::NODE_CODE_BLOCK;
	node->fenced = fence_length != 0;
	node->fence_length = fence_length;
	node->fence_char = '`';
	return node;
}

std::unique_ptr<DocNodeHeading> DocParser::create_heading(std::int8_t level) {
	auto node = std::make_unique<DocNodeHeading>();
	node->type = DocNode::NODE_HEADING;
	node->level = level;
	return node;
}

std::unique_ptr<DocNode> DocParser::create_node(DocNode::Type type) {
	assert(type >= DocNode::NODE_FIRST_INLINE && type <= DocNode::NODE_LAST_INLINE);
	auto node = std::make_unique<DocNode>();
	node->type = type;
	return node;
}

std::unique_ptr<DocNodeLiteral> DocParser::create_text(const std::string &str) {
	auto node = std::make_unique<DocNodeLiteral>();
	node->type = DocNode::NODE_TEXT;
	node->str = str;
	return node;
}

std::unique_ptr<DocNodeLiteral> DocParser::create_code(const std::string &str) {
	auto node = std::make_unique<DocNodeLiteral>();
	node->type = DocNode::NODE_CODE;
	node->str = str;
	return node;
}

std::unique_ptr<DocNodeLiteral> DocParser::create_html(const std::string &str) {
	auto node = std::make_unique<DocNodeLiteral>();
	node->type = DocNode::NODE_HTML;
	node->str = str;
	return node;
}

std::unique_ptr<DocNodeSymbolLink> DocParser::make_symbol_link(DocNodeSymbolLink::TagType tag_type,
															   const std::string &symbol) {
	auto node = std::make_unique<DocNodeSymbolLink>();
	node->type = DocNode::NODE_SYMBOL_LINK;
	node->tag_type = tag_type;
	node->symbol = symbol;
	return node;
}

std::vector<std::unique_ptr<DocNode>> DocParser::make_node_list(std::unique_ptr<DocNode> &&node) {
	std::vector<std::unique_ptr<DocNode>> nodes;
	nodes.emplace_back(std::move(node));
	return nodes;
}

DocNodeSymbolLink::TagType DocParser::get_tag_type(const std::string &tag) {

	static const std::unordered_map<std::string, DocNodeSymbolLink::TagType> g_tags = {
		{"@module", DocNodeSymbolLink::MODULE_TAG},
		{"@see", DocNodeSymbolLink::SEE_TAG},
	};

	if (auto i = g_tags.find(tag); i != g_tags.end()) {
		return i->second;
	}

	return DocNodeSymbolLink::NO_TAG;
}

DocParser::Delimiter::Delimiter(DocLexer::Token delimiter) :
	m_delimiters({delimiter}) {}

DocParser::Delimiter::Delimiter(std::initializer_list<DocLexer::Token> delimiters) :
	m_delimiters(delimiters) {}

bool DocParser::Delimiter::operator==(DocLexer::Token token) const {
	return std::find(std::begin(m_delimiters), std::end(m_delimiters), token) != std::end(m_delimiters);
}

bool DocParser::Delimiter::operator!=(DocLexer::Token token) const {
	return std::find(std::begin(m_delimiters), std::end(m_delimiters), token) == std::end(m_delimiters);
}

std::unique_ptr<DocNode> DocParser::parse_block_quote(DocLexer::Token &type, std::string &token) {

	auto block_quote = create_block_quote();
	bool enforce_continuation = true;
	bool accept_alert = true;
	bool first_line = true;

	std::tie(type, token) = m_lexer->next_token();
	if (type == DocLexer::BLANK_TOKEN) {
		std::tie(type, token) = m_lexer->next_token();
	}

	for (auto line = parse_text(type, token, DocLexer::LINE_BREAK_TOKEN); enforce_continuation || !is_blank(line);
		 line = parse_text(type, token, DocLexer::LINE_BREAK_TOKEN)) {
		if (!first_line) {
			block_quote->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
			extend_nodes(block_quote->children, std::move(line));
		}
		else if (accept_alert && line.size() == 1 && line.front()->type == DocNode::NODE_TEXT) {
			if (!read_alert_type(line.front()->as<DocNodeLiteral>()->str, block_quote->alert_type)) {
				extend_nodes(block_quote->children, std::move(line));
				first_line = false;
			}
			accept_alert = false;
		}
		else {
			extend_nodes(block_quote->children, std::move(line));
			first_line = false;
		}
		std::tie(type, token) = m_lexer->next_token();
		if (type == DocLexer::RIGHT_ANGLED_TOKEN) {
			std::tie(type, token) = m_lexer->next_token();
			if (type == DocLexer::BLANK_TOKEN) {
				std::tie(type, token) = m_lexer->next_token();
			}
			enforce_continuation = true;
		}
		else {
			enforce_continuation = false;
		}
	}

	return block_quote;
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_table() {

	auto table = create_block(DocNode::NODE_TABLE);
	std::vector<std::vector<std::unique_ptr<DocNode>>> columns;

	auto [type, token] = m_lexer->next_token();
	auto text = parse_text(type, token, {DocLexer::PIPE_TOKEN, DocLexer::LINE_BREAK_TOKEN});

	while (type == DocLexer::PIPE_TOKEN) {

		columns.emplace_back(std::move(text));

		std::tie(type, token) = m_lexer->next_token();
		text = parse_text(type, token, {DocLexer::PIPE_TOKEN, DocLexer::LINE_BREAK_TOKEN});
	}

	std::tie(type, token) = m_lexer->next_token();
	if (type != DocLexer::PIPE_TOKEN) {
		return join_table_nodes(std::move(columns));
	}

	auto head = create_block(DocNode::NODE_TABLE_HEAD);

	std::tie(type, token) = m_lexer->next_token();
	text = parse_text(type, token, {DocLexer::PIPE_TOKEN, DocLexer::LINE_BREAK_TOKEN});
	std::vector<std::vector<std::unique_ptr<DocNode>>> align_tokens;

	for (auto it = columns.begin(); type == DocLexer::PIPE_TOKEN; ++it) {

		if (it == columns.end()) {
			auto nodes = join_table_nodes(std::move(columns));
			nodes.emplace_back(create_node(DocNode::NODE_LINEBREAK));
			extend_nodes(nodes, join_table_nodes(std::move(align_tokens)));
			extend_text_nodes(nodes, std::move(text));
			return nodes;
		}

		DocNodeTableColumn::Align align = DocNodeTableColumn::ALIGN_AUTO;
		if (text.size() != 1 || text.front()->type != DocNode::NODE_TEXT
			|| !read_align(text.front()->as<DocNodeLiteral>()->str, align)) {
			auto nodes = join_table_nodes(std::move(columns));
			nodes.emplace_back(create_node(DocNode::NODE_LINEBREAK));
			extend_nodes(nodes, join_table_nodes(std::move(align_tokens)));
			extend_text_nodes(nodes, std::move(text));
			return nodes;
		}
		align_tokens.emplace_back(std::move(text));

		auto column = create_table_column(align);
		extend_nodes(column->children, std::move(*it));
		head->children.emplace_back(std::move(column));

		std::tie(type, token) = m_lexer->next_token();
		text = parse_text(type, token, {DocLexer::PIPE_TOKEN, DocLexer::LINE_BREAK_TOKEN});
	}

	table->children.emplace_back(std::move(head));

	std::tie(type, token) = m_lexer->next_token();
	if (type != DocLexer::PIPE_TOKEN) {
		return make_node_list(std::move(table));
	}

	auto body = create_block(DocNode::NODE_TABLE_BODY);

	do {
		auto row = create_block(DocNode::NODE_TABLE_ROW);

		std::tie(type, token) = m_lexer->next_token();
		text = parse_text(type, token, {DocLexer::PIPE_TOKEN, DocLexer::LINE_BREAK_TOKEN});

		while (type == DocLexer::PIPE_TOKEN) {

			auto item = create_block(DocNode::NODE_TABLE_ITEM);
			extend_nodes(item->children, std::move(text));
			row->children.emplace_back(std::move(item));

			std::tie(type, token) = m_lexer->next_token();
			text = parse_text(type, token, {DocLexer::PIPE_TOKEN, DocLexer::LINE_BREAK_TOKEN});
		}

		body->children.emplace_back(std::move(row));
		std::tie(type, token) = m_lexer->next_token();
	}
	while (type == DocLexer::PIPE_TOKEN);

	table->children.emplace_back(std::move(body));

	return make_node_list(std::move(table));
}

std::unique_ptr<DocNode> DocParser::parse_unordered_list(DocLexer::Token &type, std::string &token, std::size_t &column,
														 std::uint8_t indent) {

	const std::size_t list_column = column;
	std::tie(type, token) = m_lexer->next_token();

	if (type != DocLexer::BLANK_TOKEN) {
		auto block = create_block(DocNode::NODE_EMPH);
		extend_nodes(block->children, parse_text(type, token, DocLexer::ASTERISK_TOKEN));
		return std::move(block);
	}

	auto list = create_list(indent, false);
	auto item = create_block(DocNode::NODE_ITEM);
	std::tie(type, token) = m_lexer->next_token();

	while (type != DocLexer::FILE_END_TOKEN) {

		extend_text_nodes(item->children, parse_text(type, token, DocLexer::LINE_BREAK_TOKEN));
		item->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
		if (type != DocLexer::LINE_BREAK_TOKEN) {
			list->children.emplace_back(std::move(item));
			return list;
		}

		std::tie(type, token) = m_lexer->next_token();
		if (type == DocLexer::BLANK_TOKEN) {
			column = m_lexer->get_column_number() - 1;
			std::tie(type, token) = m_lexer->next_token();
		}
		else {
			column = 0;
		}

		switch (type) {
		case DocLexer::ASTERISK_TOKEN:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_unordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::ASTERISK_TOKEN) {
					return list;
				}
				std::tie(type, token) = m_lexer->next_token();
				if (type == DocLexer::BLANK_TOKEN) {
					column = m_lexer->get_column_number() - 1;
					std::tie(type, token) = m_lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::NODE_ITEM);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = m_lexer->next_token();
				if (type != DocLexer::BLANK_TOKEN) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::ASTERISK_TOKEN));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::NODE_ITEM);
				}
				std::tie(type, token) = m_lexer->next_token();
			}
			break;
		case DocLexer::NUMBER_PERIOD_TOKEN:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_ordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::ASTERISK_TOKEN) {
					return list;
				}
				std::tie(type, token) = m_lexer->next_token();
				if (type == DocLexer::BLANK_TOKEN) {
					column = m_lexer->get_column_number() - 1;
					std::tie(type, token) = m_lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::NODE_ITEM);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = m_lexer->next_token();
				if (type != DocLexer::BLANK_TOKEN) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::ASTERISK_TOKEN));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::NODE_ITEM);
				}
				std::tie(type, token) = m_lexer->next_token();
			}
			break;
		case DocLexer::LINE_BREAK_TOKEN:
			list->children.emplace_back(std::move(item));
			return list;
		default:
			break;
		}
	}

	list->children.emplace_back(std::move(item));
	return list;
}

std::unique_ptr<DocNode> DocParser::parse_ordered_list(DocLexer::Token &type, std::string &token, std::size_t &column,
													   std::uint8_t indent) {

	const std::string list_token = token;
	const std::size_t list_column = column;
	std::tie(type, token) = m_lexer->next_token();

	if (type != DocLexer::BLANK_TOKEN) {
		return create_text(list_token);
	}

	auto list = create_list(indent, true);
	auto item = create_block(DocNode::NODE_ITEM);
	std::tie(type, token) = m_lexer->next_token();

	while (type != DocLexer::FILE_END_TOKEN) {

		extend_text_nodes(item->children, parse_text(type, token, DocLexer::LINE_BREAK_TOKEN));
		item->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
		if (type != DocLexer::LINE_BREAK_TOKEN) {
			list->children.emplace_back(std::move(item));
			return list;
		}

		std::tie(type, token) = m_lexer->next_token();
		if (type == DocLexer::BLANK_TOKEN) {
			column = m_lexer->get_column_number() - 1;
			std::tie(type, token) = m_lexer->next_token();
		}
		else {
			column = 0;
		}

		switch (type) {
		case DocLexer::ASTERISK_TOKEN:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_unordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::NUMBER_PERIOD_TOKEN) {
					return list;
				}
				std::tie(type, token) = m_lexer->next_token();
				if (type == DocLexer::BLANK_TOKEN) {
					column = m_lexer->get_column_number() - 1;
					std::tie(type, token) = m_lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::NODE_ITEM);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = m_lexer->next_token();
				if (type != DocLexer::BLANK_TOKEN) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::ASTERISK_TOKEN));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::NODE_ITEM);
				}
				std::tie(type, token) = m_lexer->next_token();
			}
			break;
		case DocLexer::NUMBER_PERIOD_TOKEN:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_ordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::NUMBER_PERIOD_TOKEN) {
					return list;
				}
				std::tie(type, token) = m_lexer->next_token();
				if (type == DocLexer::BLANK_TOKEN) {
					column = m_lexer->get_column_number() - 1;
					std::tie(type, token) = m_lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::NODE_ITEM);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = m_lexer->next_token();
				if (type != DocLexer::BLANK_TOKEN) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::ASTERISK_TOKEN));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::NODE_ITEM);
				}
				std::tie(type, token) = m_lexer->next_token();
			}
			break;
		case DocLexer::LINE_BREAK_TOKEN:
			list->children.emplace_back(std::move(item));
			return list;
		default:
			break;
		}
	}

	list->children.emplace_back(std::move(item));
	return list;
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_link(DocLexer::Token &type, std::string &token) {
	std::tie(type, token) = m_lexer->next_token();
	auto text = parse_text(type, token, {DocLexer::CLOSE_BRACKET_OPEN_PARENTHESIS_TOKEN, DocLexer::LINE_BREAK_TOKEN});
	if (type != DocLexer::CLOSE_BRACKET_OPEN_PARENTHESIS_TOKEN) {
		if (!text.empty() && text.front()->type == DocNode::NODE_TEXT) {
			static_cast<DocNodeLiteral *>(text.front().get())->str.insert(0, "[");
		}
		else {
			text.insert(text.begin(), create_text("["));
		}
		return text;
	}
	auto link = create_link(false);
	link->url = parse_url(DocLexer::CLOSE_PARENTHESIS_TOKEN);
	extend_nodes(link->children, std::move(text));
	return make_node_list(std::move(link));
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_wiki_link(DocLexer::Token &type, std::string &token) {
	std::tie(type, token) = m_lexer->next_token();
	auto text = parse_text(type, token, {DocLexer::PIPE_TOKEN, DocLexer::DBL_CLOSE_BRACKET_TOKEN, DocLexer::LINE_BREAK_TOKEN});
	if (type != DocLexer::PIPE_TOKEN && type != DocLexer::DBL_CLOSE_BRACKET_TOKEN) {
		if (!text.empty() && text.front()->type == DocNode::NODE_TEXT) {
			static_cast<DocNodeLiteral *>(text.front().get())->str.insert(0, "[[");
		}
		else {
			text.insert(text.begin(), create_text("[["));
		}
		return text;
	}
	auto link = create_link(true);
	if (type == DocLexer::PIPE_TOKEN) {
		link->url = parse_url(DocLexer::DBL_CLOSE_BRACKET_TOKEN);
		extend_nodes(link->children, std::move(text));
	}
	else {
		link->url = text_to_url(std::move(text));
	}
	return make_node_list(std::move(link));
}

std::unique_ptr<DocNode> DocParser::parse_heading() {

	std::int8_t level = 1;
	auto [type, token] = m_lexer->next_token();

	while (type == DocLexer::SHARP_TOKEN) {
		if (++level > 6) {
			type = DocLexer::WORD_TOKEN;
			token = std::string(level, '#');
			return parse_paragraph(type, token);
		}
		std::tie(type, token) = m_lexer->next_token();
	}

	if (type == DocLexer::BLANK_TOKEN) {
		auto heading = create_heading(level);
		extend_nodes(heading->children, parse_text(DocLexer::LINE_BREAK_TOKEN));
		heading->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
		return heading;
	}

	type = DocLexer::WORD_TOKEN;
	token = std::string(level, '#') + token;
	return parse_paragraph(type, token);
}

std::unique_ptr<DocNode> DocParser::parse_paragraph(DocLexer::Token &type, std::string &token) {

	auto paragraph = create_block(DocNode::NODE_PARAGRAPH);
	extend_nodes(paragraph->children, parse_text(type, token, DocLexer::LINE_BREAK_TOKEN));
	paragraph->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));

	for (auto line = parse_text(DocLexer::LINE_BREAK_TOKEN); !is_blank(line);
		 line = parse_text(DocLexer::LINE_BREAK_TOKEN)) {
		extend_nodes(paragraph->children, std::move(line));
		paragraph->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
	}

	return paragraph;
}

std::unique_ptr<DocNode> DocParser::parse_thematic_break(DocLexer::Token &type, std::string &token) {

	const std::string token_text = token;
	std::tie(type, token) = m_lexer->next_token();
	auto line = parse_text(type, token, DocLexer::LINE_BREAK_TOKEN);

	if (is_blank(line)) {
		return create_node(DocNode::NODE_THEMATIC_BREAK);
	}

	if (!line.empty() && line.front()->type == DocNode::NODE_TEXT) {
		static_cast<DocNodeLiteral *>(line.front().get())->str.insert(0, token_text);
	}
	else {
		line.insert(line.begin(), create_text(token_text));
	}

	auto paragraph = create_block(DocNode::NODE_PARAGRAPH);
	extend_nodes(paragraph->children, parse_text(type, token, DocLexer::LINE_BREAK_TOKEN));
	paragraph->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));

	for (line = parse_text(DocLexer::LINE_BREAK_TOKEN); !is_blank(line);
		 line = parse_text(DocLexer::LINE_BREAK_TOKEN)) {
		extend_nodes(paragraph->children, std::move(line));
		paragraph->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
	}

	return paragraph;
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_text(const Delimiter &delimiter) {
	auto [type, token] = m_lexer->next_token();
	return parse_text(type, token, delimiter);
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_text(DocLexer::Token &type, std::string &token,
															const Delimiter &delimiter) {

	std::vector<std::unique_ptr<DocNode>> text;

	while (delimiter != type) {
		switch (type) {
		case DocLexer::ASTERISK_TOKEN:
			if (std::size_t column = m_lexer->get_token_column_number();
				column == m_lexer->get_first_non_blank_column_number()) {
				extend_text_nodes(text, parse_unordered_list(type, token, column, 0));
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::NODE_EMPH, type, token));
			}
			break;
		case DocLexer::UNDERSCORE_TOKEN:
			if (!text.empty() && text.back()->type == DocNode::NODE_TEXT
				&& !DocLexer::is_white_space(text.back()->as<DocNodeLiteral>()->str.back())) {
				static_cast<DocNodeLiteral *>(text.back().get())->str += token;
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::NODE_EMPH, type, token));
			}
			break;
		case DocLexer::DBL_ASTERISK_TOKEN:
			extend_text_nodes(text, parse_format_block(type, token, DocNode::NODE_STRONG, type, token));
			break;
		case DocLexer::DBL_UNDERSCORE_TOKEN:
			if (!text.empty() && text.back()->type == DocNode::NODE_TEXT
				&& !DocLexer::is_white_space(text.back()->as<DocNodeLiteral>()->str.back())) {
				static_cast<DocNodeLiteral *>(text.back().get())->str += token;
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::NODE_STRONG, type, token));
			}
			break;
		case DocLexer::TPL_ASTERISK_TOKEN:
			extend_text_nodes(text, parse_format_block(type, token, DocNode::NODE_STRONG_EMPH, type, token));
			break;
		case DocLexer::TPL_UNDERSCORE_TOKEN:
			if (!text.empty() && text.back()->type == DocNode::NODE_TEXT
				&& !DocLexer::is_white_space(text.back()->as<DocNodeLiteral>()->str.back())) {
				static_cast<DocNodeLiteral *>(text.back().get())->str += token;
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::NODE_STRONG_EMPH, type, token));
			}
			break;
		case DocLexer::DBL_TILDE_TOKEN:
			extend_text_nodes(text, parse_format_block(type, token, DocNode::NODE_DEL, type, token));
			break;
		case DocLexer::BACKQUOTE_TOKEN:
			text.emplace_back(parse_code(type, 1, m_lexer->get_token_column_number()));
			break;
		case DocLexer::DBL_BACKQUOTE_TOKEN:
			text.emplace_back(parse_code(type, 2, m_lexer->get_token_column_number()));
			break;
		case DocLexer::TPL_BACKQUOTE_TOKEN:
			text.emplace_back(parse_code(type, 3, m_lexer->get_token_column_number()));
			break;
		case DocLexer::OPEN_BRACKET_TOKEN:
			extend_text_nodes(text, parse_link(type, token));
			break;
		case DocLexer::DBL_OPEN_BRACKET_TOKEN:
			extend_text_nodes(text, parse_wiki_link(type, token));
			break;
		case DocLexer::OPEN_BRACE_TOKEN:
			text.emplace_back(parse_symbol_link());
			break;
		case DocLexer::LEFT_ANGLED_TOKEN:
			extend_text_nodes(text, parse_html(type, token));
			break;
		case DocLexer::NUMBER_PERIOD_TOKEN:
			if (std::size_t column = m_lexer->get_token_column_number();
				column == m_lexer->get_first_non_blank_column_number()) {
				extend_text_nodes(text, parse_ordered_list(type, token, column, 0));
			}
			else if (!text.empty() && text.back()->type == DocNode::NODE_TEXT) {
				static_cast<DocNodeLiteral *>(text.back().get())->str += token;
			}
			else {
				text.emplace_back(create_text(token));
			}
			break;
		case DocLexer::LINE_BREAK_TOKEN:
			text.emplace_back(create_node(DocNode::NODE_LINEBREAK));
			break;
		case DocLexer::FILE_END_TOKEN:
			return text;
		default:
			if (!text.empty() && text.back()->type == DocNode::NODE_TEXT) {
				static_cast<DocNodeLiteral *>(text.back().get())->str += token;
			}
			else {
				text.emplace_back(create_text(token));
			}
			break;
		}

		if (delimiter != type) {
			std::tie(type, token) = m_lexer->next_token();
		}
	}

	return text;
}

std::unique_ptr<DocNode> DocParser::parse_format_block(DocLexer::Token &type, std::string &token, DocNode::Type format,
													   const Delimiter &delimiter, std::string text) {
	std::tie(type, token) = m_lexer->next_token();
	if (type == DocLexer::LINE_BREAK_TOKEN || type == DocLexer::FILE_END_TOKEN) {
		return create_text(std::forward<std::string>(text));
	}
	if (type == DocLexer::BLANK_TOKEN) {
		return create_text(std::forward<std::string>(text) + token);
	}
	auto block = create_block(format);
	extend_nodes(block->children, parse_text(type, token, delimiter));
	return std::move(block);
}

std::unique_ptr<DocNode> DocParser::parse_code(const Delimiter &delimiter, std::uint8_t fence_length, std::size_t column) {

	std::string code;
	auto [type, token] = m_lexer->next_token();
	auto block = create_code_block(fence_length);

	while (delimiter != type && type != DocLexer::LINE_BREAK_TOKEN && type != DocLexer::FILE_END_TOKEN) {
		code += token;
		std::tie(type, token) = m_lexer->next_token();
	}

	if (type != DocLexer::LINE_BREAK_TOKEN) {
		block->children.emplace_back(create_code(code));
		return block;
	}

	block->info = code;
	code.clear();

	while (delimiter != type && type != DocLexer::FILE_END_TOKEN) {
		std::tie(type, token) = m_lexer->next_token();
		if (type != DocLexer::LINE_BREAK_TOKEN) {
			code += token;
		}
		else {
			block->children.emplace_back(create_code(code));
			do {
				block->children.emplace_back(create_node(DocNode::NODE_LINEBREAK));
			}
			while (!m_lexer->skip_to_column(column));
			code.clear();
		}
	}

	return std::move(block);
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_html(DocLexer::Token &type, std::string &token) {

	std::string html;
	std::vector<std::unique_ptr<DocNode>> nodes;
	std::tie(type, token) = m_lexer->next_token();

	while (type != DocLexer::RIGHT_ANGLED_TOKEN) {
		switch (type) {
		case DocLexer::FILE_END_TOKEN:
		case DocLexer::LINE_BREAK_TOKEN:
			nodes.emplace_back(create_text('<' + html));
			return nodes;
		default:
			html += token;
		}
		std::tie(type, token) = m_lexer->next_token();
	}

	nodes.emplace_back(create_html(html));
	return nodes;
}

std::string DocParser::parse_url(const Delimiter &delimiter) {

	std::string url;

	auto [type, token] = m_lexer->next_token();

	while (delimiter != type && type != DocLexer::FILE_END_TOKEN) {
		url += token;
		std::tie(type, token) = m_lexer->next_token();
	}

	return url;
}

std::unique_ptr<DocNode> DocParser::parse_symbol_link() {

	std::string symbol;
	auto [type, token] = m_lexer->next_token();
	auto tag_type = DocNodeSymbolLink::NO_TAG;

	if (type == DocLexer::WORD_TOKEN && token[0] == '@') {
		tag_type = get_tag_type(token);
		std::tie(type, token) = m_lexer->next_token();
		if (type == DocLexer::BLANK_TOKEN) {
			std::tie(type, token) = m_lexer->next_token();
		}
	}

	while (type != DocLexer::CLOSE_BRACE_TOKEN && type != DocLexer::FILE_END_TOKEN) {
		symbol += token;
		std::tie(type, token) = m_lexer->next_token();
	}

	return make_symbol_link(tag_type, symbol);
}

std::vector<std::unique_ptr<DocNode>> DocParser::join_table_nodes(std::vector<std::vector<std::unique_ptr<DocNode>>> &&node_lists) {
	std::vector<std::unique_ptr<DocNode>> nodes;
	nodes.emplace_back(create_text("|"));
	for (auto &node_list : node_lists) {
		if (!node_list.empty() && node_list.front()->type == DocNode::NODE_TEXT) {
			static_cast<DocNodeLiteral *>(nodes.back().get())->str += node_list.front()->as<DocNodeLiteral>()->str;
			node_list.erase(node_list.begin());
		}
		for (auto &node : node_list) {
			nodes.emplace_back(std::move(node));
		}
		if (nodes.back()->type == DocNode::NODE_TEXT) {
			static_cast<DocNodeLiteral *>(nodes.back().get())->str += "|";
		}
		else {
			nodes.emplace_back(create_text("|"));
		}
	}
	return nodes;
}
