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

#include "generators/gollumgenerator.h"
#include "docnode.h"
#include "gollumgenerator.h"

#include <cstddef>
#include <limits>
#include <mint/system/utf8.h>
#include <mint/system/terminal.h>
#include <mint/system/filesystem.h>
#include <mint/memory/reference.h>

#include <filesystem>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <regex>
#include <set>

using namespace mint;

namespace {

void trace(const std::string &type, const std::string &name, const std::string &doc = {}) {
	if (doc.empty()) {
		mint::printf(stdout,
					 MINT_TERM_FG_BLUE_WITH(MINT_TERM_BOLD_OPTION) " >> " MINT_TERM_FG_RED_WITH(
						 MINT_TERM_ITALIC_OPTION) "%s " MINT_TERM_RESET "%s\n",
					 type.c_str(), name.c_str());
	}
	else {
		mint::printf(stdout,
					 MINT_TERM_FG_BLUE_WITH(MINT_TERM_BOLD_OPTION) " >> " MINT_TERM_FG_RED_WITH(
						 MINT_TERM_ITALIC_OPTION) "%s " MINT_TERM_RESET
												  "%s " MINT_TERM_FG_GREEN_WITH(
													  MINT_TERM_ITALIC_OPTION) "%s" MINT_TERM_RESET "\n",
					 type.c_str(), name.c_str(), doc.c_str());
	}
}

void infos(const std::string &info, const std::string &doc = {}) {
	if (doc.empty()) {
		mint::printf(stdout, MINT_TERM_FG_GREY_WITH(MINT_TERM_BOLD_OPTION) "    %s" MINT_TERM_RESET "\n", info.c_str());
	}
	else {
		mint::printf(stdout,
					 MINT_TERM_FG_GREY_WITH(MINT_TERM_BOLD_OPTION) "    %s" MINT_TERM_RESET " " MINT_TERM_FG_GREEN_WITH(
						 MINT_TERM_ITALIC_OPTION) "%s" MINT_TERM_RESET "\n",
					 info.c_str(), doc.c_str());
	}
}

std::string indent(std::size_t count) {
	return std::string(count * 2, ' ');
}

std::string definition_modifiers(Definition *definition) {

	std::string modifiers;

	if (definition->flags & Reference::FINAL_MEMBER) {
		modifiers += "`final` ";
	}
	else if (definition->flags & Reference::OVERRIDE_MEMBER) {
		modifiers += "`override` ";
	}

	if (definition->flags & Reference::GLOBAL) {
		modifiers += "`@` ";
	}

	if ((definition->flags & Reference::CONST_VALUE) && (definition->flags & Reference::CONST_ADDRESS)) {
		modifiers += "`const` ";
	}
	else {
		if (definition->flags & Reference::CONST_VALUE) {
			modifiers += "`%` ";
		}
		if (definition->flags & Reference::CONST_ADDRESS) {
			modifiers += "`$` ";
		}
	}

	switch (definition->type) {
	case Definition::PACKAGE_DEFINITION:
		modifiers += "`package`";
		break;

	case Definition::ENUM_DEFINITION:
		modifiers += "`enum`";
		break;

	case Definition::CLASS_DEFINITION:
		modifiers += "`class`";
		break;

	default:
		break;
	}

	return modifiers;
}

struct MemberBrief {
	std::string modifiers;
	std::string link;
	std::string brief;
};

void dump_members_brief(FILE *file, const char *visibility, const std::vector<MemberBrief> &members) {

	if (members.empty()) {
		return;
	}

	fprintf(file, "#### %s members\n\n", visibility);
	fprintf(file, "| Modifiers | Member | Description |\n"
				  "|-----------|--------|-------------|\n");
	
	for (const MemberBrief &member : members) {
		fprintf(file, "| %s | %s | %s |\n", member.modifiers.c_str(), member.link.c_str(), member.brief.c_str());
	}

	fprintf(file, "\n");
}

void process_script(std::stringstream &stream, std::string &token) {

	if (!stream.eof()) {

		int c = stream.get();
		token += static_cast<char>(c);

		if (c == '`') {
			do {
				process_script(stream, token);
				c = stream.get();
				token += static_cast<char>(c);
			}
			while (c != '`');
		}
		else {

			bool finished = false;

			while (!finished && !stream.eof()) {
				switch (c = stream.get()) {
				case '`':
					token += static_cast<char>(c);
					finished = true;
					break;

				default:
					token += static_cast<char>(c);
					break;
				}
			}
		}
	}
}

bool insert_pritable_text(std::string &documentation, const std::string &text, std::size_t &max_length) {
	if (max_length < text.length()) {
		if (max_length >= 3) {
			documentation.append(text.substr(0, max_length - 3));
		}
		max_length = 0;
		return false;
	}
	documentation.append(text);
	max_length -= text.length();
	return true;
}

bool insert_linebreak(std::string &documentation, bool enabled, std::size_t &max_length) {
	if (enabled) {
		return insert_pritable_text(documentation, "\n", max_length);
	}
	if (!documentation.empty() && documentation.back() != ' ' && documentation.back() != '\t') {
		return insert_pritable_text(documentation, " ", max_length);
	}
	return true;
}


}

void GollumGenerator::setup_links(const Dictionary *dictionary, Module *module) {

	std::set<std::string> links;

	for (const auto &def : module->definitions) {

		std::string link;

		for (const_utf8iterator it = def.first.cbegin(); it != def.first.cend(); ++it) {
			if (utf8_is_space(*it)) {
				link += '-';
			}
			else if (utf8_is_alnum(*it)) {
				link += *it;
			}
			else if (*it == "-" || *it == "_") {
				link += *it;
			}
		}

		int count = 1;
		std::string suffix;

		while (links.find(link + suffix) != links.end()) {
			std::stringstream stream;
			stream << '-' << count++;
			suffix = stream.str();
		}

		module->links.emplace(def.first, link + suffix);
		links.emplace(link + suffix);
	}
}

void GollumGenerator::generate_module_list(const Dictionary *dictionary, const std::filesystem::path &path,
										   const std::vector<Module *> &modules) {

	std::filesystem::path file_path = path / "Modules.md";

	if (FILE *file = open_file(file_path, "w")) {

		fprintf(file, "# Modules\n\n");

		for (Module *module : modules) {
			size_t level = static_cast<size_t>(count(module->name.begin(), module->name.end(), '.'));
			std::string indent_str = indent(level);
			std::string base_name = level ? module->name.substr(module->name.rfind('.') + 1) : module->name;
			std::string brief_str = brief(dictionary, module->doc);
			fprintf(file, "%s* [[%s|%s]] %s\n", indent_str.c_str(), base_name.c_str(), module->name.c_str(),
					brief_str.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generate_module(const Dictionary *dictionary, const std::filesystem::path &path, Module *module) {

	std::filesystem::path module_path = path / (module->name + ".md");

	if (FILE *file = open_file(module_path, "w")) {

		switch (module->type) {
		case Module::SCRIPT:
			generate_module(dictionary, file, module);
			break;

		case Module::GROUP:
			generate_module_group(dictionary, file, module);
			break;
		}

		fclose(file);
	}
}

void GollumGenerator::generate_package_list(const Dictionary *dictionary, const std::filesystem::path &path,
											const std::vector<Package *> &packages) {

	std::filesystem::path file_path = path / "Packages.md";

	if (FILE *file = open_file(file_path, "w")) {

		fprintf(file, "# Packages\n\n");

		for (Package *package : packages) {
			size_t level = static_cast<size_t>(count(package->name.begin(), package->name.end(), '.'));
			std::string base_name = level ? package->symbol() : package->name;
			std::string indent_str = indent(level);
			std::string link_str = external_link(base_name, "Package " + package->name);
			std::string brief_str = brief(dictionary, package->doc, package);
			fprintf(file, "%s* %s %s\n", indent_str.c_str(), link_str.c_str(), brief_str.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generate_package(const Dictionary *dictionary, const std::filesystem::path &path,
									   Package *package) {

	std::filesystem::path package_path = path / ("Package " + package->name + ".md");

	if (FILE *file = open_file(package_path, "w")) {
		generate_package(dictionary, file, package);
		fclose(file);
	}
}

void GollumGenerator::generate_page_list(const Dictionary *dictionary, const std::filesystem::path &path,
										 const std::vector<Page *> &pages) {

	std::filesystem::path file_path = path / "Pages.md";

	if (FILE *file = open_file(file_path, "w")) {

		fprintf(file, "# Pages\n\n");

		for (const Page *page : pages) {
			std::string link_str = external_link(page->name);
			std::string brief_str = brief(dictionary, page->doc);
			fprintf(file, "* %s %s\n", link_str.c_str(), brief_str.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generate_page(const Dictionary *dictionary, const std::filesystem::path &path, Page *page) {

	std::filesystem::path package_path = path / (page->name + ".md");

	if (FILE *file = open_file(package_path, "w")) {
		std::string doc_str = doc_from_mintdoc(dictionary, page->doc);
		fprintf(file, "%s", doc_str.c_str());
		fclose(file);
	}
}

std::string GollumGenerator::external_link(const std::string &label, const std::string &target,
										   const std::string &section) {
	return "[" + regex_replace(label, std::regex("\\|"), "&#124;") + "](" + target + "#" + section + ")";
}

std::string GollumGenerator::external_link(const std::string &label, const std::string &target) {
	return "[[" + regex_replace(label, std::regex("\\|"), "&#124;") + "|" + target + "]]";
}

std::string GollumGenerator::external_link(const std::string &target) {
	return "[" + regex_replace(target, std::regex("\\|"), "&#124;") + "](" + target + ")";
}

std::string GollumGenerator::internal_link(const std::string &label, const std::string &section) {
	return "[" + regex_replace(label, std::regex("\\|"), "&#124;") + "](#" + section + ")";
}

std::string GollumGenerator::brief(const Dictionary *dictionary, const std::unique_ptr<DocNode> &node,
								   const Definition *context, size_t max_length) {
	std::string brief;
	if (!mintdoc_to_string(dictionary, context, node, {}, brief, max_length,
						   WITHOUT_LINEBREAK | WITHOUT_LINKS | WITHOUT_UNFENCED_CODE)) {
		brief.append("...");
	}
	return regex_replace(brief, std::regex("\\|"), "&#124;");
}

std::string GollumGenerator::doc_from_mintdoc(const Dictionary *dictionary, const std::unique_ptr<DocNode> &node,
											  const Definition *context) {
	std::string documentation;
	std::size_t max_length = std::numeric_limits<std::size_t>::max();
	mintdoc_to_string(dictionary, context, node, {}, documentation, max_length);
	return documentation;
}

std::string GollumGenerator::definition_brief(const Dictionary *dictionary, const Definition *definition) {

	switch (definition->type) {
	case Definition::PACKAGE_DEFINITION:
		if (const auto *instance = definition->as<Package>()) {
			return brief(dictionary, instance->doc, instance);
		}
		break;

	case Definition::ENUM_DEFINITION:
		if (const auto *instance = definition->as<Enum>()) {
			return brief(dictionary, instance->doc, instance);
		}
		break;

	case Definition::CLASS_DEFINITION:
		if (const auto *instance = definition->as<Class>()) {
			return brief(dictionary, instance->doc, instance);
		}
		break;

	case Definition::CONSTANT_DEFINITION:
		if (const auto *instance = definition->as<Constant>()) {
			return brief(dictionary, instance->doc, instance);
		}
		break;

	case Definition::FUNCTION_DEFINITION:
		if (const auto *instance = definition->as<Function>()) {
			if (!instance->signatures.empty()) {
				return brief(dictionary, instance->signatures.front()->doc, instance);
			}
		}
		break;
	}

	return {};
}

bool GollumGenerator::mintdoc_to_string(const Dictionary *dictionary, const Definition *context,
										const std::unique_ptr<DocNode> &node, const std::string &prefix,
										std::string &documentation, std::size_t &max_length, FormatOptions options) {

	switch (node->type) {
	case DocNode::NODE_DOCUMENT:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					return false;
				}
			}
		}
		break;
	case DocNode::NODE_BLOCK_QUOTE:
		if (options & WITHOUT_LINEBREAK) {
			return true;
		}
		if (const auto *node_data = node->as<DocNodeBlockQuote>()) {
			switch (node_data->alert_type) {
			case DocNodeBlockQuote::ALERT_NONE:
				break;
			case DocNodeBlockQuote::ALERT_NOTE:
				documentation += "> [!NOTE]\n";
				break;
			case DocNodeBlockQuote::ALERT_TIP:
				documentation += "> [!TIP]\n";
				break;
			case DocNodeBlockQuote::ALERT_IMPORTANT:
				documentation += "> [!IMPORTANT]\n";
				break;
			case DocNodeBlockQuote::ALERT_WARNING:
				documentation += "> [!WARNING]\n";
				break;
			case DocNodeBlockQuote::ALERT_CAUTION:
				documentation += "> [!CAUTION]\n";
				break;
			}
			documentation += "> ";
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix + "> ", documentation, max_length, options)) {
					return false;
				}
			}
			insert_linebreak(documentation, !(options & WITHOUT_LINEBREAK), max_length);
		}
		break;
	case DocNode::NODE_TABLE:
		if (options & WITHOUT_LINEBREAK) {
			return true;
		}
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					return false;
				}
			}
		}
		break;
	case DocNode::NODE_TABLE_HEAD:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			std::vector<std::size_t> column_sizes;
			column_sizes.reserve(node_data->children.size());
			documentation += "|";
			for (const auto &column : node_data->children) {
				std::string column_text;
				mintdoc_to_string(dictionary, context, column, {}, column_text, max_length, options);
				column_sizes.push_back(std::max(column_text.length() - 1, std::size_t(3)));
				documentation += column_text;
			}
			documentation += "\n|";
			auto it = column_sizes.begin();
			for (const auto &column : node_data->children) {
				if (const auto *column_data = column->as<DocNodeTableColumn>()) {
					switch (column_data->align) {
					case DocNodeTableColumn::ALIGN_AUTO:
						documentation += std::string(*it++, '-') + '|';
						break;
					case DocNodeTableColumn::ALIGN_LEFT:
						documentation += ':' + std::string((*it++) - 1, '-') + '|';
						break;
					case DocNodeTableColumn::ALIGN_CENTER:
						documentation += ':' + std::string((*it++) - 2, '-') + ":|";
						break;
					case DocNodeTableColumn::ALIGN_RIGHT:
						documentation += std::string((*it++) - 1, '-') + ":|";
						break;
					}
				}
			}
			documentation += "\n";
		}
		break;
	case DocNode::NODE_TABLE_COLUMN:
		if (const auto *node_data = node->as<DocNodeTableColumn>()) {
			for (const auto &child_node : node_data->children) {
				mintdoc_to_string(dictionary, context, child_node, prefix, documentation , max_length, options);
			}
			documentation += "|";
		}
		break;
	case DocNode::NODE_TABLE_BODY:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			for (const auto &child_node : node_data->children) {
				mintdoc_to_string(dictionary, context, child_node, prefix, documentation , max_length, options);
			}
			documentation += "\n";
		}
		break;
	case DocNode::NODE_TABLE_ROW:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			documentation += "|";
			for (const auto &child_node : node_data->children) {
				mintdoc_to_string(dictionary, context, child_node, prefix, documentation , max_length, options);
			}
			documentation += "\n";
		}
		break;
	case DocNode::NODE_TABLE_ITEM:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			for (const auto &child_node : node_data->children) {
				mintdoc_to_string(dictionary, context, child_node, prefix, documentation , max_length, options);
			}
			documentation += "|";
		}
		break;
	case DocNode::NODE_LIST:
		if (options & WITHOUT_LINEBREAK) {
			return true;
		}
		if (const auto *node_data = node->as<DocNodeList>()) {
			std::size_t index = 0;
			for (const auto &child_node : node_data->children) {
				documentation += indent(node_data->indent);
				documentation += node_data->ordered ? std::to_string(++index) + ". " : "* ";
				mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
			}
		}
		break;
	case DocNode::NODE_ITEM:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			for (const auto &child_node : node_data->children) {
				mintdoc_to_string(dictionary, context, child_node, prefix, documentation , max_length, options);
			}
		}
		break;
	case DocNode::NODE_LINK:
		if (const auto *node_data = node->as<DocNodeLink>()) {
			if (!(options & WITHOUT_LINKS)) {
				documentation += node_data->wiki_style ? "[[" : "[";
			}
			bool state = true;
			for (const auto &child_node : node_data->children) {
				state = mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
				if (!state) {
					break;
				}
			}
			if (!(options & WITHOUT_LINKS)) {
				if (node_data->wiki_style) {
					if (!node_data->children.empty()) {
						documentation += "|";
					}
					documentation += node_data->url + "]]";
				}
				else {
					documentation += "](" + node_data->url + ")";
				}
			}
			return state;
		}
		break;
	case DocNode::NODE_DEL:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			documentation += "~~";
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					documentation += "~~";
					return false;
				}
			}
			documentation += "~~";
		}
		break;
	case DocNode::NODE_EMPH:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			documentation += '*';
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					documentation += "*";
					return false;
				}
			}
			documentation += '*';
		}
		break;
	case DocNode::NODE_STRONG:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			documentation += "**";
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					documentation += "**";
					return false;
				}
			}
			documentation += "**";
		}
		break;
	case DocNode::NODE_STRONG_EMPH:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			documentation += "***";
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					documentation += "***";
					return false;
				}
			}
			documentation += "***";
		}
		break;
	case DocNode::NODE_CODE_BLOCK:
		if (const auto *node_data = node->as<DocNodeCodeBlock>()) {
			if (node_data->fenced) {
				documentation += std::string(node_data->fence_length, node_data->fence_char);
			}
			else if (!(options & WITHOUT_UNFENCED_CODE)) {

			}
			if (node_data->info) {
				documentation += *node_data->info + '\n';
			}
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					documentation += std::string(node_data->fence_length, node_data->fence_char);
					return false;
				}
			}
			documentation += std::string(node_data->fence_length, node_data->fence_char);
		}
		break;
	case DocNode::NODE_CUSTOM_BLOCK:
		break;
	case DocNode::NODE_PARAGRAPH:
		if (const auto *node_data = node->as<DocNodeBlock>()) {
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					return false;
				}
			}
			insert_linebreak(documentation, !(options & WITHOUT_LINEBREAK), max_length);
		}
		break;
	case DocNode::NODE_HEADING:
		if (options & WITHOUT_LINEBREAK) {
			return true;
		}
		if (const auto *node_data = node->as<DocNodeHeading>()) {
			documentation += std::string(node_data->level, '#') + ' ';
			for (const auto &child_node : node_data->children) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					return false;
				}
			}
			insert_linebreak(documentation, !(options & WITHOUT_LINEBREAK), max_length);
		}
		break;
	case DocNode::NODE_CODE:
	case DocNode::NODE_TEXT:
		if (const auto *node_data = node->as<DocNodeLiteral>()) {
			return insert_pritable_text(documentation, node_data->str, max_length);
		}
		break;
	case DocNode::NODE_HTML:
		if (const auto *node_data = node->as<DocNodeLiteral>()) {
			documentation += '<' + node_data->str + '>';
		}
		break;
	case DocNode::NODE_SOFTBREAK:
		break;
	case DocNode::NODE_LINEBREAK:
		insert_linebreak(documentation, !(options & WITHOUT_LINEBREAK), max_length);
		insert_pritable_text(documentation, prefix, max_length);
		break;
	case DocNode::NODE_THEMATIC_BREAK:
		if (!(options & WITHOUT_LINEBREAK)) {
			documentation += "---\n\n";
		}
		break;
	case DocNode::NODE_CUSTOM_INLINE:
		break;
	case DocNode::NODE_IMAGE:
		break;
	case DocNode::NODE_SYMBOL_LINK:
		if (const auto *node_data = node->as<DocNodeSymbolLink>()) {
			const std::string target_symbol = symbol_link_target(node_data, context);
			if (options & WITHOUT_LINKS) {
				documentation += "`";
				if (!insert_pritable_text(documentation, target_symbol, max_length)) {
					documentation += "`";
					return false;
				}
				documentation += "`";
			}
			else {
				switch (node_data->tag_type) {
				case DocNodeSymbolLink::NO_TAG:
					if (Module *module = dictionary->find_definition_module(node_data->symbol)) {
						documentation += external_link(node_data->symbol, module->name,
													   module->links.at(node_data->symbol));
					}
					else {
						documentation += external_link(node_data->symbol);
					}
					break;

				case DocNodeSymbolLink::SEE_TAG:
					if (Module *module = dictionary->find_definition_module(target_symbol)) {
						documentation += internal_link(node_data->symbol, module->links.at(target_symbol));
					}
					else {
						documentation += external_link(node_data->symbol);
					}
					break;

				case DocNodeSymbolLink::MODULE_TAG:
					documentation += external_link(node_data->symbol);
					break;
				}
			}
		}
		break;
	}

	return true;
}

void GollumGenerator::generate_module(const Dictionary *dictionary, FILE *file, const Module *module) {

	trace("module", module->name, brief(dictionary, module->doc));
	fprintf(file, "# Module `%s`\n\n", module->name.c_str());

	{
		std::string doc_str = doc_from_mintdoc(dictionary, module->doc);
		fprintf(file,
				"## Description\n\n"
				"`load %s`\n\n"
				"%s",
				module->name.c_str(), doc_str.c_str());
	}

	for (const auto &type : module->elements) {

		switch (type.first) {
		case Definition::PACKAGE_DEFINITION:
			fprintf(file, "## Packages\n\n");
			break;
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "## Constants\n\n");
			break;
		case Definition::CLASS_DEFINITION:
			fprintf(file, "## Classes\n\n");
			break;
		case Definition::ENUM_DEFINITION:
			fprintf(file, "## Enums\n\n");
			break;
		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "## Functions\n\n");
			break;
		}

		for (const auto &def : type.second) {
			switch (def.second->type) {
			case Definition::PACKAGE_DEFINITION:
				{
					std::string link_str = external_link(def.first, "Package " + def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;

			case Definition::ENUM_DEFINITION:
				fprintf(file, "### %s\n\n", def.first.c_str());
				if (const auto *instance = def.second->as<Enum>()) {

					trace("enum", def.first, brief(dictionary, instance->doc, instance));

					std::string doc_str = doc_from_mintdoc(dictionary, instance->doc, instance);
					fprintf(file, "%s", doc_str.c_str());
					fprintf(file, "| Constant | Value | Description |\n"
								  "|----------|-------|-------------|\n");

					for (Definition *definition : dictionary->enum_definitions(instance)) {
						if (definition->type == Definition::CONSTANT_DEFINITION) {
							if (const auto *value = definition->as<Constant>()) {
								std::string link_str = internal_link(definition->symbol(),
																	 module->links.at(definition->name));
								std::string brief_str = definition_brief(dictionary, definition);
								fprintf(file, "| %s | `%s` | %s |\n", link_str.c_str(), value->value.c_str(),
										brief_str.c_str());
							}
						}
					}
				}
				break;

			case Definition::CLASS_DEFINITION:
				fprintf(file, "### %s\n\n", def.first.c_str());
				if (const auto *instance = def.second->as<Class>()) {

					trace("class", def.first, brief(dictionary, instance->doc, instance));

					std::string doc_str = doc_from_mintdoc(dictionary, instance->doc, instance);
					fprintf(file, "%s", doc_str.c_str());

					if (!instance->bases.empty()) {
						fprintf(file, "#### Inherits\n\n");
						std::string context = instance->context();
						for (const std::string &base : instance->bases) {
							if (Module *script = dictionary->find_definition_module(base)) {
								std::string link_str = external_link(base, script->name, script->links.at(base));
								fprintf(file, "* %s\n", link_str.c_str());
							}
							else if (Module *script = dictionary->find_definition_module(context + "." + base)) {
								std::string link_str = external_link(context + "." + base, script->name,
																	 script->links.at(context + "." + base));
								fprintf(file, "* %s\n", link_str.c_str());
							}
							else {
								std::string link_str = external_link(base);
								fprintf(file, "* %s\n", link_str.c_str());
							}
						}
						fprintf(file, "\n");
					}

					std::vector<MemberBrief> public_members;
					std::vector<MemberBrief> protected_members;
					std::vector<MemberBrief> package_members;
					std::vector<MemberBrief> private_members;
					
					for (Definition *definition : dictionary->class_definitions(instance)) {
						if (instance->name == definition->context()) {
							MemberBrief member_brief {
								/*.modifiers = */ definition_modifiers(definition),
								/*.link = */ internal_link(definition->symbol(), module->links.at(definition->name)),
								/*.brief = */ definition_brief(dictionary, definition),
							};
							if (definition->flags & Reference::PRIVATE_VISIBILITY) {
								private_members.push_back(member_brief);
							}
							else if (definition->flags & Reference::PROTECTED_VISIBILITY) {
								protected_members.push_back(member_brief);
							}
							else if (definition->flags & Reference::PACKAGE_VISIBILITY) {
								package_members.push_back(member_brief);
							}
							else {
								public_members.push_back(member_brief);
							}
						}
					}

					dump_members_brief(file, "Public", public_members);
					dump_members_brief(file, "Protected", protected_members);
					dump_members_brief(file, "Package", package_members);
					dump_members_brief(file, "Private", private_members);
				}

				fprintf(file, "\n");
				break;

			default:
				std::string link_str = internal_link(def.first, module->links.at(def.first));
				fprintf(file, "* %s\n", link_str.c_str());
				break;
			}
		}

		fprintf(file, "\n");
	}

	fprintf(file, "## Descriptions\n\n");

	for (const auto &def : module->definitions) {
		switch (def.second->type) {
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "### %s\n\n", def.first.c_str());
			if (const Constant *instance = def.second->as<Constant>()) {
				trace("constant", def.first, brief(dictionary, instance->doc, instance));
				fprintf(file, "`%s`\n\n", instance->value.empty() ? "none" : instance->value.c_str());
				std::string doc_str = doc_from_mintdoc(dictionary, instance->doc, instance);
				fprintf(file, "%s", doc_str.c_str());
			}
			break;

		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "### %s\n\n", def.first.c_str());
			if (const Function *instance = def.second->as<Function>()) {
				trace("function", def.first);
				for (auto *signature : instance->signatures) {
					infos(signature->format, brief(dictionary, signature->doc, instance));
					fprintf(file, "`%s`\n\n", signature->format.c_str());
					std::string doc_str = doc_from_mintdoc(dictionary, signature->doc, instance);
					fprintf(file, "%s", doc_str.c_str());
				}
			}
			break;

		default:
			break;
		}
	}
}

void GollumGenerator::generate_module_group(const Dictionary *dictionary, FILE *file, Module *module) {

	trace("module group", module->name);
	fprintf(file, "# Module `%s`\n\n", module->name.c_str());

	std::string doc_str = doc_from_mintdoc(dictionary, module->doc);
	fprintf(file, "## Description\n\n%s", doc_str.c_str());

	for (const Module *script : dictionary->child_modules(module)) {
		for (const auto &type : script->elements) {
			for (auto def : type.second) {
				module->elements[type.first].insert(def);
			}
		}
	}

	for (const auto &type : module->elements) {

		switch (type.first) {
		case Definition::PACKAGE_DEFINITION:
			fprintf(file, "## Packages\n\n");
			break;
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "## Constants\n\n");
			break;
		case Definition::CLASS_DEFINITION:
			fprintf(file, "## Classes\n\n");
			break;
		case Definition::ENUM_DEFINITION:
			fprintf(file, "## Enums\n\n");
			break;
		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "## Functions\n\n");
			break;
		}

		for (const auto &def : type.second) {
			switch (type.first) {
			case Definition::PACKAGE_DEFINITION:
				{
					std::string link_str = external_link(def.first, "Package " + def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;

			default:
				if (Module *script = dictionary->find_definition_module(def.first)) {
					std::string link_str = external_link(def.first, script->name, script->links.at(def.first));
					fprintf(file, "* %s\n", link_str.c_str());
				}
				else {
					std::string link_str = external_link(def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;
			}
		}

		fprintf(file, "\n");
	}
}

void GollumGenerator::generate_package(const Dictionary *dictionary, FILE *file, const Package *package) {

	trace("package", package->name, brief(dictionary, package->doc, package));
	fprintf(file, "# Package `%s`\n\n", package->name.c_str());

	std::string doc_str = doc_from_mintdoc(dictionary, package->doc, package);
	fprintf(file, "## Description\n\n%s", doc_str.c_str());

	std::map<Definition::Type, std::map<std::string, Definition *>> elements;

	for (Definition *definition : dictionary->package_definitions(package)) {
		elements[definition->type].emplace(definition->name, definition);
	}

	for (const auto &type : elements) {

		switch (type.first) {
		case Definition::PACKAGE_DEFINITION:
			fprintf(file, "## Packages\n\n");
			break;
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "## Constants\n\n");
			break;
		case Definition::CLASS_DEFINITION:
			fprintf(file, "## Classes\n\n");
			break;
		case Definition::ENUM_DEFINITION:
			fprintf(file, "## Enums\n\n");
			break;
		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "## Functions\n\n");
			break;
		}

		for (const auto &def : type.second) {
			switch (type.first) {
			case Definition::PACKAGE_DEFINITION:
				{
					std::string link_str = external_link(def.first, "Package " + def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;

			default:
				if (Module *script = dictionary->find_definition_module(def.first)) {
					std::string link_str = external_link(def.first, script->name, script->links.at(def.first));
					fprintf(file, "* %s\n", link_str.c_str());
				}
				else {
					std::string link_str = external_link(def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;
			}
		}

		fprintf(file, "\n");
	}
}
