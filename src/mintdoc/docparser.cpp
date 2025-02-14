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

#include "docparser.h"
#include "docnode.h"

#include <memory>
#include <unordered_map>

std::unique_ptr<DocNode> DocParser::parse(std::stringstream &stream) {

	m_root.reset(new DocNode {/*.type = */ DocNode::NODE_DOCUMENT});
	m_current = {m_root.get()};

	/*bool all_matched = true;
	cmark_node *container;

	bytes = m_curline.size;

	m_line_number++;

	if (std::unique_ptr<DocNode> last_matched_container = check_open_blocks(parser, stream, &all_matched)) {
		container = last_matched_container;
        open_new_blocks(parser, &container, stream, all_matched);
        add_text_to_container(parser, container, last_matched_container, stream);
	}

	m_last_line_length = stream.len;
	if (m_last_line_length && stream.data[m_last_line_length - 1] == '\n') {
		m_last_line_length -= 1;
	}
	if (m_last_line_length && stream.data[m_last_line_length - 1] == '\r') {
		m_last_line_length -= 1;
	}*/

	return std::move(m_root);
}

DocNodeSymbolLink::TagType DocParser::get_tag_type(const std::string &tag) const {

	static const std::unordered_map<std::string, DocNodeSymbolLink::TagType> g_tags = {
		{"module", DocNodeSymbolLink::MODULE_TAG},
		{"see", DocNodeSymbolLink::SEE_TAG},
	};

	if (auto i = g_tags.find(tag); i != g_tags.end()) {
		return i->second;
	}

	return DocNodeSymbolLink::NO_TAG;
}

/*std::string GollumGenerator::doc_from_mintdoc(const Dictionary *dictionary, const std::unique_ptr<DocNode> &node,
											  const Definition *context) {

	std::string token;
	bool finished = false;
	bool new_line = true;
	bool suspect_tag = false;
	auto block_start = std::string::npos;
	DocNodeSymbolLink::Type tag_type = DocNodeSymbolLink::NO_TAG;

	std::string documentation;

	while (!finished && !stream.eof()) {
		switch (int c = stream.get()) {
		case EOF:
			if (!new_line && !token.empty()) {
				documentation += ' ' + token;
			}
			else {
				documentation += token;
			}
			finished = true;
			break;

		case '{':
			block_start = token.size();
			token += static_cast<char>(c);
			break;

		case '}':
			if (block_start != std::string::npos) {

				std::string symbol = token.substr(block_start + 1);
				std::string target_symbol = symbol;

				if (context) {
					switch (context->type) {
					case Definition::PACKAGE_DEFINITION:
					case Definition::ENUM_DEFINITION:
					case Definition::CLASS_DEFINITION:
						target_symbol = context->name + "." + symbol;
						break;
					case Definition::CONSTANT_DEFINITION:
					case Definition::FUNCTION_DEFINITION:
						target_symbol = context->context() + "." + symbol;
						break;
					}
				}

				switch (tag_type) {
				case DocNodeSymbolLink::NO_TAG:
					if (Module *module = dictionary->find_definition_module(symbol)) {
						token.replace(block_start, std::string::npos,
									  external_link(symbol, module->name, module->links.at(symbol)));
					}
					else {
						token.replace(block_start, std::string::npos, external_link(symbol));
					}
					break;

				case DocNodeSymbolLink::SEE_TAG:
					if (Module *module = dictionary->find_definition_module(target_symbol)) {
						token.replace(block_start, std::string::npos,
									  internal_link(symbol, module->links.at(target_symbol)));
					}
					else {
						token.replace(block_start, std::string::npos, external_link(symbol));
					}
					break;

				case DocNodeSymbolLink::MODULE_TAG:
					token.replace(block_start, std::string::npos, external_link(symbol));
					break;
				}
				tag_type = DocNodeSymbolLink::NO_TAG;
				block_start = std::string::npos;
				suspect_tag = false;
			}
			else {
				token += static_cast<char>(c);
			}
			break;

		case '@':
			if (suspect_tag) {
				token += static_cast<char>(c);
				suspect_tag = false;
			}
			else {
				suspect_tag = true;
			}
			break;

		case '`':
			if (block_start != std::string::npos) {
				block_start = std::string::npos;
				token += '{';
			}
			if (suspect_tag) {
				suspect_tag = false;
				token += '@';
			}
			token += static_cast<char>(c);
			process_script(stream, token);
			if (!new_line && !token.empty()) {
				documentation += ' ' + token;
			}
			else {
				documentation += token;
			}
			if (!token.empty()) {
				new_line = false;
				token.clear();
			}
			break;

		case '\n':
			if (block_start != std::string::npos) {
				block_start = std::string::npos;
				token += '{';
			}
			if (suspect_tag) {
				suspect_tag = false;
				token += '@';
			}
			if (!new_line && !token.empty() && !must_join(token.front())) {
				documentation += ' ' + token + "\n";
			}
			else {
				documentation += token + "\n";
			}
			new_line = true;
			token.clear();
			break;

		default:
			if (isspace(c)) {
				if (suspect_tag) {
					if (block_start != std::string::npos) {
						tag_type = dictionary->get_tag_type(token.substr(block_start + 1));
						token.erase(block_start + 1);
					}
					else {
						tag_type = dictionary->get_tag_type(token);
						token.clear();
					}
				}
				else if (new_line) {
					token += static_cast<char>(c);
				}
				else {
					if (!token.empty() && !must_join(token.front())) {
						documentation += ' ' + token;
					}
					else {
						documentation += token;
					}
					if (!token.empty()) {
						new_line = false;
						token.clear();
					}
					if (tag_type == DocNodeSymbolLink::NO_TAG) {
						block_start = std::string::npos;
					}
				}
			}
			else {
				token += static_cast<char>(c);
			}
		}
	}

	return documentation;
}*/
