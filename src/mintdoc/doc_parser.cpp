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

#include "doc_parser.h"
#include "doc_lexer.h"
#include "doc_node.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr const std::size_t code_indent = 4;

std::vector<std::unique_ptr<DocNode>>& extend_nodes(std::vector<std::unique_ptr<DocNode>>& nodes,
    std::vector<std::unique_ptr<DocNode>>&& items) {
	std::ranges::move(std::move(items), std::back_inserter(nodes));
	return nodes;
}

std::vector<std::unique_ptr<DocNode>>& extend_text_nodes(std::vector<std::unique_ptr<DocNode>>& text,
    std::vector<std::unique_ptr<DocNode>>&& items) {
	auto it = std::begin(items);
	auto end = std::end(items);
	if (!text.empty() && text.back()->type == DocNode::node_text && it != end && (*it)->type == DocNode::node_text) {
		static_cast<DocNodeLiteral*>(text.back().get())->str += (*it++)->as<DocNodeLiteral>().str;
	}
	std::move(it, end, std::back_inserter(text));
	return text;
}

std::vector<std::unique_ptr<DocNode>>& extend_text_nodes(std::vector<std::unique_ptr<DocNode>>& text,
    std::unique_ptr<DocNode>&& item) {
	if (!text.empty() && text.back()->type == DocNode::node_text && item->type == DocNode::node_text) {
		static_cast<DocNodeLiteral*>(text.back().get())->str += item->as<DocNodeLiteral>().str;
	}
	else {
		text.emplace_back(std::move(item));
	}
	return text;
}

bool is_blank(const std::vector<std::unique_ptr<DocNode>>& nodes) {
	return std::ranges::none_of(nodes, [](const std::unique_ptr<DocNode>& node) {
		if (node->type == DocNode::node_linebreak) {
			return false;
		}
		if (node->type == DocNode::node_text) {
			return std::ranges::any_of(node->as<DocNodeLiteral>().str, [](char c) {
				return c != ' ' && c != '\t';
			});
		}
		return true;
	});
}

bool read_alert_type(const std::string& token, DocNodeBlockQuote::AlertType& alert_type) {
	static const std::unordered_map<std::string, DocNodeBlockQuote::AlertType> g_alerts = {
	    {"[!NOTE]", DocNodeBlockQuote::alert_note},
	    {"[!TIP]", DocNodeBlockQuote::alert_tip},
	    {"[!IMPORTANT]", DocNodeBlockQuote::alert_important},
	    {"[!WARNING]", DocNodeBlockQuote::alert_warning},
	    {"[!CAUTION]", DocNodeBlockQuote::alert_caution},
	};
	if (auto i = g_alerts.find(token.substr(0, token.find_first_of(" \t"))); i != g_alerts.end()) {
		alert_type = i->second;
		return true;
	}
	return false;
}

bool read_align(const std::string& token, DocNodeTableColumn::Align& align) {
	int hyphen_count = 0;
	auto pos = token.find_first_not_of(" \t");
	if (pos == std::string::npos) {
		return false;
	}
	while (pos < token.size()) {
		switch (token[pos++]) {
		case ':':
			if (hyphen_count == 0) {
				align = DocNodeTableColumn::align_left;
			}
			else {
				if (align == DocNodeTableColumn::align_left) {
					align = DocNodeTableColumn::align_center;
				}
				else {
					align = DocNodeTableColumn::align_right;
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

std::string text_to_url(std::vector<std::unique_ptr<DocNode>>&& text) {
	std::string url;
	for (auto&& node : std::move(text)) {
		switch (node->type) {
		case DocNode::node_text:
			url += node->as<DocNodeLiteral>().str;
			break;
		default:
			assert(false);
		}
	}
	return url;
}

}

std::unique_ptr<DocNode> DocParser::parse(std::stringstream& stream) {

	_lexer = std::make_unique<DocLexer>(stream);
	auto root = create_block(DocNode::node_document);

	for (;;) {

		auto [type, token] = _lexer->next_token();

		switch (type) {
		case DocLexer::Token::sharp:
			root->children.emplace_back(parse_heading());
			break;
		case DocLexer::Token::pipe:
			extend_nodes(root->children, parse_table());
			break;
		case DocLexer::Token::right_angled:
			root->children.emplace_back(parse_block_quote(type, token));
			root->children.emplace_back(create_node(DocNode::node_linebreak));
			break;
		case DocLexer::Token::blank:
			if (_lexer->get_column_number() - 1 >= code_indent) {
				root->children.emplace_back(parse_code(DocLexer::Token::line_break, 0, 0));
			}
			else {
				root->children.emplace_back(parse_paragraph(type, token));
			}
			break;
		case DocLexer::Token::tpl_hyphen:
		case DocLexer::Token::tpl_asterisk:
		case DocLexer::Token::tpl_underscore:
			root->children.emplace_back(parse_thematic_break(type, token));
			break;
		case DocLexer::Token::line_break:
			break;
		case DocLexer::Token::file_end:
			return std::move(root);
		default:
			root->children.emplace_back(parse_paragraph(type, token));
		}
	}

	return {};
}

std::unique_ptr<DocNodeBlock> DocParser::create_block(DocNode::Type type) {
	assert(type >= DocNode::node_first_block && type <= DocNode::node_last_block);
	auto node = std::make_unique<DocNodeBlock>();
	node->type = type;
	return node;
}

std::unique_ptr<DocNodeBlockQuote> DocParser::create_block_quote() {
	auto node = std::make_unique<DocNodeBlockQuote>();
	node->type = DocNode::node_block_quote;
	node->alert_type = DocNodeBlockQuote::alert_none;
	return node;
}

std::unique_ptr<DocNodeTableColumn> DocParser::create_table_column(DocNodeTableColumn::Align align) {
	auto node = std::make_unique<DocNodeTableColumn>();
	node->type = DocNode::node_table_column;
	node->align = align;
	return node;
}

std::unique_ptr<DocNodeList> DocParser::create_list(std::uint8_t indent, bool ordered) {
	auto node = std::make_unique<DocNodeList>();
	node->type = DocNode::node_list;
	node->indent = indent;
	node->ordered = ordered;
	return node;
}

std::unique_ptr<DocNodeLink> DocParser::create_link(bool wiki_style) {
	auto node = std::make_unique<DocNodeLink>();
	node->type = DocNode::node_link;
	node->wiki_style = wiki_style;
	return node;
}

std::unique_ptr<DocNodeCodeBlock> DocParser::create_code_block(std::uint8_t fence_length) {
	auto node = std::make_unique<DocNodeCodeBlock>();
	node->type = DocNode::node_code_block;
	node->fenced = fence_length != 0;
	node->fence_length = fence_length;
	node->fence_char = '`';
	return node;
}

std::unique_ptr<DocNodeHeading> DocParser::create_heading(std::int8_t level) {
	auto node = std::make_unique<DocNodeHeading>();
	node->type = DocNode::node_heading;
	node->level = level;
	return node;
}

std::unique_ptr<DocNode> DocParser::create_node(DocNode::Type type) {
	assert(type >= DocNode::node_first_inline && type <= DocNode::node_last_inline);
	auto node = std::make_unique<DocNode>();
	node->type = type;
	return node;
}

std::unique_ptr<DocNodeLiteral> DocParser::create_text(const std::string& str) {
	auto node = std::make_unique<DocNodeLiteral>();
	node->type = DocNode::node_text;
	node->str = str;
	return node;
}

std::unique_ptr<DocNodeLiteral> DocParser::create_code(const std::string& str) {
	auto node = std::make_unique<DocNodeLiteral>();
	node->type = DocNode::node_code;
	node->str = str;
	return node;
}

std::unique_ptr<DocNodeLiteral> DocParser::create_html(const std::string& str) {
	auto node = std::make_unique<DocNodeLiteral>();
	node->type = DocNode::node_html;
	node->str = str;
	return node;
}

std::unique_ptr<DocNodeSymbolLink> DocParser::make_symbol_link(DocNodeSymbolLink::TagType tag_type,
    const std::string& symbol) {
	auto node = std::make_unique<DocNodeSymbolLink>();
	node->type = DocNode::node_symbol_link;
	node->tag_type = tag_type;
	node->symbol = symbol;
	return node;
}

std::vector<std::unique_ptr<DocNode>> DocParser::make_node_list(std::unique_ptr<DocNode>&& node) {
	std::vector<std::unique_ptr<DocNode>> nodes;
	nodes.emplace_back(std::move(node));
	return nodes;
}

DocNodeSymbolLink::TagType DocParser::get_tag_type(const std::string& tag) {

	static const std::unordered_map<std::string, DocNodeSymbolLink::TagType> g_tags = {
	    {"@module", DocNodeSymbolLink::module_tag},
	    {"@see", DocNodeSymbolLink::see_tag},
	};

	if (auto i = g_tags.find(tag); i != g_tags.end()) {
		return i->second;
	}

	return DocNodeSymbolLink::no_tag;
}

DocParser::Delimiter::Delimiter(DocLexer::Token delimiter) :
    _delimiters({delimiter}) {}

DocParser::Delimiter::Delimiter(std::initializer_list<DocLexer::Token> delimiters) :
    _delimiters(delimiters) {}

bool DocParser::Delimiter::operator==(DocLexer::Token token) const {
	return std::find(std::begin(_delimiters), std::end(_delimiters), token) != std::end(_delimiters);
}

bool DocParser::Delimiter::operator!=(DocLexer::Token token) const {
	return std::find(std::begin(_delimiters), std::end(_delimiters), token) == std::end(_delimiters);
}

std::unique_ptr<DocNode> DocParser::parse_block_quote(DocLexer::Token& type, std::string& token) {

	auto block_quote = create_block_quote();
	bool enforce_continuation = true;
	bool accept_alert = true;
	bool first_line = true;

	std::tie(type, token) = _lexer->next_token();
	if (type == DocLexer::Token::blank) {
		std::tie(type, token) = _lexer->next_token();
	}

	for (auto line = parse_text(type, token, DocLexer::Token::line_break); enforce_continuation || !is_blank(line);
	    line = parse_text(type, token, DocLexer::Token::line_break)) {
		if (!first_line) {
			block_quote->children.emplace_back(create_node(DocNode::node_linebreak));
			extend_nodes(block_quote->children, std::move(line));
		}
		else if (accept_alert && line.size() == 1 && line.front()->type == DocNode::node_text) {
			if (!read_alert_type(line.front()->as<DocNodeLiteral>().str, block_quote->alert_type)) {
				extend_nodes(block_quote->children, std::move(line));
				first_line = false;
			}
			accept_alert = false;
		}
		else {
			extend_nodes(block_quote->children, std::move(line));
			first_line = false;
		}
		std::tie(type, token) = _lexer->next_token();
		if (type == DocLexer::Token::right_angled) {
			std::tie(type, token) = _lexer->next_token();
			if (type == DocLexer::Token::blank) {
				std::tie(type, token) = _lexer->next_token();
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

	auto table = create_block(DocNode::node_table);
	std::vector<std::vector<std::unique_ptr<DocNode>>> columns;

	auto [type, token] = _lexer->next_token();
	auto text = parse_text(type, token, {DocLexer::Token::pipe, DocLexer::Token::line_break});

	while (type == DocLexer::Token::pipe) {

		columns.emplace_back(std::move(text));

		std::tie(type, token) = _lexer->next_token();
		text = parse_text(type, token, {DocLexer::Token::pipe, DocLexer::Token::line_break});
	}

	std::tie(type, token) = _lexer->next_token();
	if (type != DocLexer::Token::pipe) {
		return join_table_nodes(std::move(columns));
	}

	auto head = create_block(DocNode::node_table_head);

	std::tie(type, token) = _lexer->next_token();
	text = parse_text(type, token, {DocLexer::Token::pipe, DocLexer::Token::line_break});
	std::vector<std::vector<std::unique_ptr<DocNode>>> align_tokens;

	for (auto it = columns.begin(); type == DocLexer::Token::pipe; ++it) {

		if (it == columns.end()) {
			auto nodes = join_table_nodes(std::move(columns));
			nodes.emplace_back(create_node(DocNode::node_linebreak));
			extend_nodes(nodes, join_table_nodes(std::move(align_tokens)));
			extend_text_nodes(nodes, std::move(text));
			return nodes;
		}

		DocNodeTableColumn::Align align = DocNodeTableColumn::align_auto;
		if (text.size() != 1 || text.front()->type != DocNode::node_text
		    || !read_align(text.front()->as<DocNodeLiteral>().str, align)) {
			auto nodes = join_table_nodes(std::move(columns));
			nodes.emplace_back(create_node(DocNode::node_linebreak));
			extend_nodes(nodes, join_table_nodes(std::move(align_tokens)));
			extend_text_nodes(nodes, std::move(text));
			return nodes;
		}
		align_tokens.emplace_back(std::move(text));

		auto column = create_table_column(align);
		extend_nodes(column->children, std::move(*it));
		head->children.emplace_back(std::move(column));

		std::tie(type, token) = _lexer->next_token();
		text = parse_text(type, token, {DocLexer::Token::pipe, DocLexer::Token::line_break});
	}

	table->children.emplace_back(std::move(head));

	std::tie(type, token) = _lexer->next_token();
	if (type != DocLexer::Token::pipe) {
		return make_node_list(std::move(table));
	}

	auto body = create_block(DocNode::node_table_body);

	do {
		auto row = create_block(DocNode::node_table_row);

		std::tie(type, token) = _lexer->next_token();
		text = parse_text(type, token, {DocLexer::Token::pipe, DocLexer::Token::line_break});

		while (type == DocLexer::Token::pipe) {

			auto item = create_block(DocNode::node_table_item);
			extend_nodes(item->children, std::move(text));
			row->children.emplace_back(std::move(item));

			std::tie(type, token) = _lexer->next_token();
			text = parse_text(type, token, {DocLexer::Token::pipe, DocLexer::Token::line_break});
		}

		body->children.emplace_back(std::move(row));
		std::tie(type, token) = _lexer->next_token();
	}
	while (type == DocLexer::Token::pipe);

	table->children.emplace_back(std::move(body));

	return make_node_list(std::move(table));
}

std::unique_ptr<DocNode> DocParser::parse_unordered_list(DocLexer::Token& type, std::string& token, std::size_t& column,
    std::uint8_t indent) {

	const std::size_t list_column = column;
	std::tie(type, token) = _lexer->next_token();

	if (type != DocLexer::Token::blank) {
		auto block = create_block(DocNode::node_emph);
		extend_nodes(block->children, parse_text(type, token, DocLexer::Token::asterisk));
		return std::move(block);
	}

	auto list = create_list(indent, false);
	auto item = create_block(DocNode::node_item);
	std::tie(type, token) = _lexer->next_token();

	while (type != DocLexer::Token::file_end) {

		extend_text_nodes(item->children, parse_text(type, token, DocLexer::Token::line_break));
		item->children.emplace_back(create_node(DocNode::node_linebreak));
		if (type != DocLexer::Token::line_break) {
			list->children.emplace_back(std::move(item));
			return list;
		}

		std::tie(type, token) = _lexer->next_token();
		if (type == DocLexer::Token::blank) {
			column = _lexer->get_column_number() - 1;
			std::tie(type, token) = _lexer->next_token();
		}
		else {
			column = 0;
		}

		switch (type) {
		case DocLexer::Token::asterisk:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_unordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::Token::asterisk) {
					return list;
				}
				std::tie(type, token) = _lexer->next_token();
				if (type == DocLexer::Token::blank) {
					column = _lexer->get_column_number() - 1;
					std::tie(type, token) = _lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::node_item);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = _lexer->next_token();
				if (type != DocLexer::Token::blank) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::Token::asterisk));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::node_item);
				}
				std::tie(type, token) = _lexer->next_token();
			}
			break;
		case DocLexer::Token::number_period:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_ordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::Token::asterisk) {
					return list;
				}
				std::tie(type, token) = _lexer->next_token();
				if (type == DocLexer::Token::blank) {
					column = _lexer->get_column_number() - 1;
					std::tie(type, token) = _lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::node_item);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = _lexer->next_token();
				if (type != DocLexer::Token::blank) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::Token::asterisk));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::node_item);
				}
				std::tie(type, token) = _lexer->next_token();
			}
			break;
		case DocLexer::Token::line_break:
			list->children.emplace_back(std::move(item));
			return list;
		default:
			break;
		}
	}

	list->children.emplace_back(std::move(item));
	return list;
}

std::unique_ptr<DocNode> DocParser::parse_ordered_list(DocLexer::Token& type, std::string& token, std::size_t& column,
    std::uint8_t indent) {

	const std::string list_token = token;
	const std::size_t list_column = column;
	std::tie(type, token) = _lexer->next_token();

	if (type != DocLexer::Token::blank) {
		return create_text(list_token);
	}

	auto list = create_list(indent, true);
	auto item = create_block(DocNode::node_item);
	std::tie(type, token) = _lexer->next_token();

	while (type != DocLexer::Token::file_end) {

		extend_text_nodes(item->children, parse_text(type, token, DocLexer::Token::line_break));
		item->children.emplace_back(create_node(DocNode::node_linebreak));
		if (type != DocLexer::Token::line_break) {
			list->children.emplace_back(std::move(item));
			return list;
		}

		std::tie(type, token) = _lexer->next_token();
		if (type == DocLexer::Token::blank) {
			column = _lexer->get_column_number() - 1;
			std::tie(type, token) = _lexer->next_token();
		}
		else {
			column = 0;
		}

		switch (type) {
		case DocLexer::Token::asterisk:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_unordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::Token::number_period) {
					return list;
				}
				std::tie(type, token) = _lexer->next_token();
				if (type == DocLexer::Token::blank) {
					column = _lexer->get_column_number() - 1;
					std::tie(type, token) = _lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::node_item);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = _lexer->next_token();
				if (type != DocLexer::Token::blank) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::Token::asterisk));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::node_item);
				}
				std::tie(type, token) = _lexer->next_token();
			}
			break;
		case DocLexer::Token::number_period:
			if (column > list_column) {
				extend_text_nodes(item->children, parse_ordered_list(type, token, column, indent + 1));
				list->children.emplace_back(std::move(item));
				if (type != DocLexer::Token::number_period) {
					return list;
				}
				std::tie(type, token) = _lexer->next_token();
				if (type == DocLexer::Token::blank) {
					column = _lexer->get_column_number() - 1;
					std::tie(type, token) = _lexer->next_token();
				}
				else {
					column = 0;
				}
				if (column < list_column) {
					return list;
				}
				item = create_block(DocNode::node_item);
			}
			else if (column < list_column) {
				list->children.emplace_back(std::move(item));
				return list;
			}
			else {
				std::tie(type, token) = _lexer->next_token();
				if (type != DocLexer::Token::blank) {
					extend_nodes(item->children, parse_text(type, token, DocLexer::Token::asterisk));
				}
				else {
					list->children.emplace_back(std::move(item));
					item = create_block(DocNode::node_item);
				}
				std::tie(type, token) = _lexer->next_token();
			}
			break;
		case DocLexer::Token::line_break:
			list->children.emplace_back(std::move(item));
			return list;
		default:
			break;
		}
	}

	list->children.emplace_back(std::move(item));
	return list;
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_link(DocLexer::Token& type, std::string& token) {
	std::tie(type, token) = _lexer->next_token();
	auto text = parse_text(type, token, {DocLexer::Token::close_bracket_open_parenthesis, DocLexer::Token::line_break});
	if (type != DocLexer::Token::close_bracket_open_parenthesis) {
		if (!text.empty() && text.front()->type == DocNode::node_text) {
			static_cast<DocNodeLiteral*>(text.front().get())->str.insert(0, "[");
		}
		else {
			text.insert(text.begin(), create_text("["));
		}
		return text;
	}
	auto link = create_link(false);
	link->url = parse_url(DocLexer::Token::close_parenthesis);
	extend_nodes(link->children, std::move(text));
	return make_node_list(std::move(link));
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_wiki_link(DocLexer::Token& type, std::string& token) {
	std::tie(type, token) = _lexer->next_token();
	auto text = parse_text(type, token,
	    {DocLexer::Token::pipe, DocLexer::Token::dbl_close_bracket, DocLexer::Token::line_break});
	if (type != DocLexer::Token::pipe && type != DocLexer::Token::dbl_close_bracket) {
		if (!text.empty() && text.front()->type == DocNode::node_text) {
			static_cast<DocNodeLiteral*>(text.front().get())->str.insert(0, "[[");
		}
		else {
			text.insert(text.begin(), create_text("[["));
		}
		return text;
	}
	auto link = create_link(true);
	if (type == DocLexer::Token::pipe) {
		link->url = parse_url(DocLexer::Token::dbl_close_bracket);
		extend_nodes(link->children, std::move(text));
	}
	else {
		link->url = text_to_url(std::move(text));
	}
	return make_node_list(std::move(link));
}

std::unique_ptr<DocNode> DocParser::parse_heading() {

	std::int8_t level = 1;
	auto [type, token] = _lexer->next_token();

	while (type == DocLexer::Token::sharp) {
		if (++level > 6) {
			type = DocLexer::Token::word;
			token = std::string(level, '#');
			return parse_paragraph(type, token);
		}
		std::tie(type, token) = _lexer->next_token();
	}

	if (type == DocLexer::Token::blank) {
		auto heading = create_heading(level);
		extend_nodes(heading->children, parse_text(DocLexer::Token::line_break));
		heading->children.emplace_back(create_node(DocNode::node_linebreak));
		return heading;
	}

	type = DocLexer::Token::word;
	token = std::string(level, '#') + token;
	return parse_paragraph(type, token);
}

std::unique_ptr<DocNode> DocParser::parse_paragraph(DocLexer::Token& type, std::string& token) {

	auto paragraph = create_block(DocNode::node_paragraph);
	extend_nodes(paragraph->children, parse_text(type, token, DocLexer::Token::line_break));
	paragraph->children.emplace_back(create_node(DocNode::node_linebreak));

	for (auto line = parse_text(DocLexer::Token::line_break); !is_blank(line);
	    line = parse_text(DocLexer::Token::line_break)) {
		extend_nodes(paragraph->children, std::move(line));
		paragraph->children.emplace_back(create_node(DocNode::node_linebreak));
	}

	return paragraph;
}

std::unique_ptr<DocNode> DocParser::parse_thematic_break(DocLexer::Token& type, std::string& token) {

	const std::string token_text = token;
	std::tie(type, token) = _lexer->next_token();
	auto line = parse_text(type, token, DocLexer::Token::line_break);

	if (is_blank(line)) {
		return create_node(DocNode::node_thematic_break);
	}

	if (!line.empty() && line.front()->type == DocNode::node_text) {
		static_cast<DocNodeLiteral*>(line.front().get())->str.insert(0, token_text);
	}
	else {
		line.insert(line.begin(), create_text(token_text));
	}

	auto paragraph = create_block(DocNode::node_paragraph);
	extend_nodes(paragraph->children, parse_text(type, token, DocLexer::Token::line_break));
	paragraph->children.emplace_back(create_node(DocNode::node_linebreak));

	for (line = parse_text(DocLexer::Token::line_break); !is_blank(line);
	    line = parse_text(DocLexer::Token::line_break)) {
		extend_nodes(paragraph->children, std::move(line));
		paragraph->children.emplace_back(create_node(DocNode::node_linebreak));
	}

	return paragraph;
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_text(const Delimiter& delimiter) {
	auto [type, token] = _lexer->next_token();
	return parse_text(type, token, delimiter);
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_text(DocLexer::Token& type, std::string& token,
    const Delimiter& delimiter) {

	std::vector<std::unique_ptr<DocNode>> text;

	while (delimiter != type) {
		switch (type) {
		case DocLexer::Token::asterisk:
			if (std::size_t column = _lexer->get_token_column_number();
			    column == _lexer->get_first_non_blank_column_number()) {
				extend_text_nodes(text, parse_unordered_list(type, token, column, 0));
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::node_emph, type, token));
			}
			break;
		case DocLexer::Token::underscore:
			if (!text.empty() && text.back()->type == DocNode::node_text
			    && !DocLexer::is_white_space(text.back()->as<DocNodeLiteral>().str.back())) {
				static_cast<DocNodeLiteral*>(text.back().get())->str += token;
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::node_emph, type, token));
			}
			break;
		case DocLexer::Token::dbl_asterisk:
			extend_text_nodes(text, parse_format_block(type, token, DocNode::node_strong, type, token));
			break;
		case DocLexer::Token::dbl_underscore:
			if (!text.empty() && text.back()->type == DocNode::node_text
			    && !DocLexer::is_white_space(text.back()->as<DocNodeLiteral>().str.back())) {
				static_cast<DocNodeLiteral*>(text.back().get())->str += token;
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::node_strong, type, token));
			}
			break;
		case DocLexer::Token::tpl_asterisk:
			extend_text_nodes(text, parse_format_block(type, token, DocNode::node_strong_emph, type, token));
			break;
		case DocLexer::Token::tpl_underscore:
			if (!text.empty() && text.back()->type == DocNode::node_text
			    && !DocLexer::is_white_space(text.back()->as<DocNodeLiteral>().str.back())) {
				static_cast<DocNodeLiteral*>(text.back().get())->str += token;
			}
			else {
				extend_text_nodes(text, parse_format_block(type, token, DocNode::node_strong_emph, type, token));
			}
			break;
		case DocLexer::Token::dbl_tilde:
			extend_text_nodes(text, parse_format_block(type, token, DocNode::node_del, type, token));
			break;
		case DocLexer::Token::backquote:
			text.emplace_back(parse_code(type, 1, _lexer->get_token_column_number()));
			break;
		case DocLexer::Token::dbl_backquote:
			text.emplace_back(parse_code(type, 2, _lexer->get_token_column_number()));
			break;
		case DocLexer::Token::tpl_backquote:
			text.emplace_back(parse_code(type, 3, _lexer->get_token_column_number()));
			break;
		case DocLexer::Token::open_bracket:
			extend_text_nodes(text, parse_link(type, token));
			break;
		case DocLexer::Token::dbl_open_bracket:
			extend_text_nodes(text, parse_wiki_link(type, token));
			break;
		case DocLexer::Token::open_brace:
			text.emplace_back(parse_symbol_link());
			break;
		case DocLexer::Token::left_angled:
			extend_text_nodes(text, parse_html(type, token));
			break;
		case DocLexer::Token::number_period:
			if (std::size_t column = _lexer->get_token_column_number();
			    column == _lexer->get_first_non_blank_column_number()) {
				extend_text_nodes(text, parse_ordered_list(type, token, column, 0));
			}
			else if (!text.empty() && text.back()->type == DocNode::node_text) {
				static_cast<DocNodeLiteral*>(text.back().get())->str += token;
			}
			else {
				text.emplace_back(create_text(token));
			}
			break;
		case DocLexer::Token::line_break:
			text.emplace_back(create_node(DocNode::node_linebreak));
			break;
		case DocLexer::Token::file_end:
			return text;
		default:
			if (!text.empty() && text.back()->type == DocNode::node_text) {
				static_cast<DocNodeLiteral*>(text.back().get())->str += token;
			}
			else {
				text.emplace_back(create_text(token));
			}
			break;
		}

		if (delimiter != type) {
			std::tie(type, token) = _lexer->next_token();
		}
	}

	return text;
}

std::unique_ptr<DocNode> DocParser::parse_format_block(DocLexer::Token& type, std::string& token, DocNode::Type format,
    const Delimiter& delimiter, std::string text) {
	std::tie(type, token) = _lexer->next_token();
	if (type == DocLexer::Token::line_break || type == DocLexer::Token::file_end) {
		return create_text(std::forward<std::string>(text));
	}
	if (type == DocLexer::Token::blank) {
		return create_text(std::forward<std::string>(text) + token);
	}
	auto block = create_block(format);
	extend_nodes(block->children, parse_text(type, token, delimiter));
	return std::move(block);
}

std::unique_ptr<DocNode> DocParser::parse_code(const Delimiter& delimiter, std::uint8_t fence_length,
    std::size_t column) {

	std::string code;
	auto [type, token] = _lexer->next_token();
	auto block = create_code_block(fence_length);

	while (delimiter != type && type != DocLexer::Token::line_break && type != DocLexer::Token::file_end) {
		code += token;
		std::tie(type, token) = _lexer->next_token();
	}

	if (type != DocLexer::Token::line_break) {
		block->children.emplace_back(create_code(code));
		return block;
	}

	block->info = code;
	code.clear();

	while (delimiter != type && type != DocLexer::Token::file_end) {
		std::tie(type, token) = _lexer->next_token();
		if (type != DocLexer::Token::line_break) {
			code += token;
		}
		else {
			block->children.emplace_back(create_code(code));
			do {
				block->children.emplace_back(create_node(DocNode::node_linebreak));
			}
			while (!_lexer->skip_to_column(column));
			code.clear();
		}
	}

	return std::move(block);
}

std::vector<std::unique_ptr<DocNode>> DocParser::parse_html(DocLexer::Token& type, std::string& token) {

	std::string html;
	std::vector<std::unique_ptr<DocNode>> nodes;
	std::tie(type, token) = _lexer->next_token();

	while (type != DocLexer::Token::right_angled) {
		switch (type) {
		case DocLexer::Token::file_end:
		case DocLexer::Token::line_break:
			nodes.emplace_back(create_text('<' + html));
			return nodes;
		default:
			html += token;
		}
		std::tie(type, token) = _lexer->next_token();
	}

	nodes.emplace_back(create_html(html));
	return nodes;
}

std::string DocParser::parse_url(const Delimiter& delimiter) {

	std::string url;

	auto [type, token] = _lexer->next_token();

	while (delimiter != type && type != DocLexer::Token::file_end) {
		url += token;
		std::tie(type, token) = _lexer->next_token();
	}

	return url;
}

std::unique_ptr<DocNode> DocParser::parse_symbol_link() {

	std::string symbol;
	auto [type, token] = _lexer->next_token();
	auto tag_type = DocNodeSymbolLink::no_tag;

	if (type == DocLexer::Token::word && token[0] == '@') {
		tag_type = get_tag_type(token);
		std::tie(type, token) = _lexer->next_token();
		if (type == DocLexer::Token::blank) {
			std::tie(type, token) = _lexer->next_token();
		}
	}

	while (type != DocLexer::Token::close_brace && type != DocLexer::Token::file_end) {
		symbol += token;
		std::tie(type, token) = _lexer->next_token();
	}

	return make_symbol_link(tag_type, symbol);
}

std::vector<std::unique_ptr<DocNode>> DocParser::join_table_nodes(
    std::vector<std::vector<std::unique_ptr<DocNode>>> node_lists) {
	std::vector<std::unique_ptr<DocNode>> nodes;
	nodes.emplace_back(create_text("|"));
	for (auto& node_list : node_lists) {
		if (!node_list.empty() && node_list.front()->type == DocNode::node_text) {
			static_cast<DocNodeLiteral*>(nodes.back().get())->str += node_list.front()->as<DocNodeLiteral>().str;
			node_list.erase(node_list.begin());
		}
		for (auto& node : node_list) {
			nodes.emplace_back(std::move(node));
		}
		if (nodes.back()->type == DocNode::node_text) {
			static_cast<DocNodeLiteral*>(nodes.back().get())->str += "|";
		}
		else {
			nodes.emplace_back(create_text("|"));
		}
	}
	return nodes;
}
