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

#include "generators/gollumgenerator.h"
#include "definition.h"
#include "dictionary.h"
#include "docnode.h"
#include "gollumgenerator.h"
#include "mint/memory/reference.h"
#include "mint/system/filesystem.h"
#include "mint/system/stdio.h"
#include "mint/system/terminal.h"
#include "mint/system/utf8.h"
#include "module.h"
#include "page.h"
#include "utils.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <format>
#include <functional>
#include <gsl/pointers>
#include <limits>
#include <map>
#include <print>
#include <regex>
#include <set>
#include <string>
#include <vector>
#include <gsl/gsl>

namespace {

void trace(const std::string& type, const std::string& name, const std::string& doc = {}) {
	if (doc.empty()) {
		mint::print(stdout,
		    std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_BOLD,
		                    MINT_TERM_FG_BLUE) " >> " MINT_TERM_OPT(MINT_TERM_ITALIC, MINT_TERM_FG_RED) "{} ") "{}\n",
		        type, name));
	}
	else {
		mint::print(stdout,
		    std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_BOLD,
		                    MINT_TERM_FG_BLUE) " >> " MINT_TERM_OPT(MINT_TERM_ITALIC,
		                    MINT_TERM_FG_RED) "{} " MINT_TERM_OPT(MINT_TERM_RESET) "{} " MINT_TERM_OPT(MINT_TERM_ITALIC,
		                    MINT_TERM_FG_GREEN) "{}") "\n",
		        type, name, doc));
	}
}

void infos(const std::string& info, const std::string& doc = {}) {
	if (doc.empty()) {
		mint::print(stdout,
		    std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_BOLD, MINT_TERM_FG_GREY) "    {}") "\n", info));
	}
	else {
		mint::print(stdout,
		    std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_BOLD, MINT_TERM_FG_GREY) "    {}") " " MINT_TERM_STR(
		                    MINT_TERM_OPT(MINT_TERM_ITALIC, MINT_TERM_FG_GREEN) "{}") "\n",
		        info, doc));
	}
}

std::string indent(std::size_t count) {
	return std::string(count * 2, ' ');
}

std::string definition_modifiers(const Definition& definition) {

	std::string modifiers;

	if (definition.flags & mint::Reference::final_member) {
		modifiers += "`final` ";
	}
	else if (definition.flags & mint::Reference::override_member) {
		modifiers += "`override` ";
	}

	if (definition.flags & mint::Reference::global) {
		modifiers += "`@` ";
	}

	if ((definition.flags & mint::Reference::const_value) && (definition.flags & mint::Reference::const_address)) {
		modifiers += "`const` ";
	}
	else {
		if (definition.flags & mint::Reference::const_value) {
			modifiers += "`%` ";
		}
		if (definition.flags & mint::Reference::const_address) {
			modifiers += "`$` ";
		}
	}

	switch (definition.type) {
	case Definition::package_definition:
		modifiers += "`package`";
		break;
	case Definition::enum_definition:
		modifiers += "`enum`";
		break;
	case Definition::class_definition:
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

void dump_members_brief(FILE* file, const std::string& visibility, const std::vector<MemberBrief>& members) {

	if (members.empty()) {
		return;
	}

	std::println(file, "#### {} members\n", visibility);
	std::println(file, "| Modifiers | Member | Description |");
	std::println(file, "|-----------|--------|-------------|");

	for (const MemberBrief& member : members) {
		std::println(file, "| {} | {} | {} |", member.modifiers, member.link, member.brief);
	}

	std::println(file);
}

bool insert_pritable_text(std::string& documentation, const std::string& text, std::size_t& max_length) {
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

bool insert_linebreak(std::string& documentation, bool enabled, std::size_t& max_length) {
	if (enabled) {
		return insert_pritable_text(documentation, "\n", max_length);
	}
	if (!documentation.empty() && documentation.back() != ' ' && documentation.back() != '\t') {
		return insert_pritable_text(documentation, " ", max_length);
	}
	return true;
}

}

void GollumGenerator::setup_links(const Dictionary& /*dictionary*/, Module& module) {

	std::set<std::string> links;

	for (const auto& def : module.definitions) {

		std::string link;

		for (const auto& code_point : mint::views::utf8(def.first)) {
			if (mint::utf8_is_space(code_point)) {
				link += '-';
			}
			else if (mint::utf8_is_alnum(code_point) || code_point == "-" || code_point == "_") {
				link += code_point;
			}
		}

		std::string suffix;
		for (int count = 1; links.contains(link + suffix); ++count) {
			suffix = std::format("-{}", count);
		}
		module.links.emplace(def.first, link + suffix);
		links.emplace(link + suffix);
	}
}

void GollumGenerator::generate_module_list(const Dictionary& dictionary, const std::filesystem::path& path,
    const std::vector<std::reference_wrapper<const Module>>& modules) {

	const auto file_path = path / "Modules.md";

	if (gsl::owner<FILE*> file = mint::open_file(file_path, "w")) {

		std::println(file, "# Modules\n");

		for (const Module& module : modules) {
			auto level = static_cast<std::size_t>(std::ranges::count(module.name, '.'));
			std::string base_name = level ? module.name.substr(module.name.rfind('.') + 1) : module.name;
			std::println(file, "{}* [[{}|{}]] {}", indent(level), base_name, module.name,
			    brief(dictionary, *module.doc));
		}

		std::fclose(file);
	}
}

void GollumGenerator::generate_module(const Dictionary& dictionary, const std::filesystem::path& path,
    const Module& module) {

	const auto module_path = path / (module.name + ".md");

	if (gsl::owner<FILE*> file = mint::open_file(module_path, "w")) {

		switch (module.type) {
		case Module::script:
			generate_script_module(file, dictionary, module);
			break;

		case Module::group:
			generate_group_module(file, dictionary, module);
			break;
		}

		std::fclose(file);
	}
}

void GollumGenerator::generate_package_list(const Dictionary& dictionary, const std::filesystem::path& path,
    const std::vector<std::reference_wrapper<const Package>>& packages) {

	const auto file_path = path / "Packages.md";

	if (gsl::owner<FILE*> file = mint::open_file(file_path, "w")) {

		std::println(file, "# Packages\n");

		for (const Package& package : packages) {
			auto level = static_cast<std::size_t>(std::ranges::count(package.name, '.'));
			const auto base_name = level ? package.symbol() : package.name;
			std::println(file, "{}* {} {}", indent(level), external_link(base_name, "Package " + package.name),
			    brief(dictionary, *package.doc, &package));
		}

		std::fclose(file);
	}
}

void GollumGenerator::generate_package(const Dictionary& dictionary, const std::filesystem::path& path,
    const Package& package) {

	const auto package_path = path / ("Package " + package.name + ".md");

	if (gsl::owner<FILE*> file = mint::open_file(package_path, "w")) {
		generate_package(file, dictionary, package);
		std::fclose(file);
	}
}

void GollumGenerator::generate_page_list(const Dictionary& dictionary, const std::filesystem::path& path,
    const std::vector<std::reference_wrapper<const Page>>& pages) {

	const auto file_path = path / "Pages.md";

	if (gsl::owner<FILE*> file = mint::open_file(file_path, "w")) {

		std::println(file, "# Pages\n");

		for (const Page& page : pages) {
			std::println(file, "* {} {}", external_link(page.name), brief(dictionary, *page.doc));
		}

		std::fclose(file);
	}
}

void GollumGenerator::generate_page(const Dictionary& dictionary, const std::filesystem::path& path, const Page& page) {

	const auto package_path = path / (page.name + ".md");

	if (gsl::owner<FILE*> file = mint::open_file(package_path, "w")) {
		std::print(file, "{}", doc_from_mintdoc(dictionary, *page.doc));
		std::fclose(file);
	}
}

std::string GollumGenerator::external_link(const std::string& label, const std::string& target,
    const std::string& section) {
	return "[" + regex_replace(label, std::regex("\\|"), "&#124;") + "](" + target + "#" + section + ")";
}

std::string GollumGenerator::external_link(const std::string& label, const std::string& target) {
	return "[[" + regex_replace(label, std::regex("\\|"), "&#124;") + "|" + target + "]]";
}

std::string GollumGenerator::external_link(const std::string& target) {
	return "[" + regex_replace(target, std::regex("\\|"), "&#124;") + "](" + target + ")";
}

std::string GollumGenerator::internal_link(const std::string& label, const std::string& section) {
	return "[" + regex_replace(label, std::regex("\\|"), "&#124;") + "](#" + section + ")";
}

std::string GollumGenerator::brief(const Dictionary& dictionary, const DocNode& node, const Definition* context,
    std::size_t max_length) {
	std::string brief;
	if (!mintdoc_to_string(dictionary, context, node, {}, brief, max_length,
	        without_linebreak | without_links | without_unfenced_code)) {
		brief.append("...");
	}
	return regex_replace(brief, std::regex("\\|"), "&#124;");
}

std::string GollumGenerator::doc_from_mintdoc(const Dictionary& dictionary, const DocNode& node,
    const Definition* context) {
	std::string documentation;
	std::size_t max_length = std::numeric_limits<std::size_t>::max();
	mintdoc_to_string(dictionary, context, node, {}, documentation, max_length);
	return documentation;
}

std::string GollumGenerator::definition_brief(const Dictionary& dictionary, const Definition& definition) {
	return visit<std::string>(Overloaded {
	                              [&](const Package& instance) -> std::string {
		                              return brief(dictionary, *instance.doc, &instance);
	                              },
	                              [&](const Enum& instance) -> std::string {
		                              return brief(dictionary, *instance.doc, &instance);
	                              },
	                              [&](const Class& instance) -> std::string {
		                              return brief(dictionary, *instance.doc, &instance);
	                              },
	                              [&](const Constant& instance) -> std::string {
		                              return brief(dictionary, *instance.doc, &instance);
	                              },
	                              [&](const Function& instance) -> std::string {
		                              if (instance.signatures.empty()) {
			                              return {};
		                              }
		                              return brief(dictionary, *instance.signatures.front()->doc, &instance);
	                              },
	                          },
	    definition);
	return {};
}

bool GollumGenerator::mintdoc_to_string(const Dictionary& dictionary, const Definition* context, const DocNode& node,
    const std::string& prefix, std::string& documentation, std::size_t& max_length, FormatOptions options) {

	switch (node.type) {
	case DocNode::node_document:
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
				return false;
			}
		}
		break;
	case DocNode::node_block_quote:
		if (options & without_linebreak) {
			return true;
		}
		{
			const auto& node_data = node.as<DocNodeBlockQuote>();
			switch (node_data.alert_type) {
			case DocNodeBlockQuote::alert_none:
				break;
			case DocNodeBlockQuote::alert_note:
				documentation += "> [!NOTE]\n";
				break;
			case DocNodeBlockQuote::alert_tip:
				documentation += "> [!TIP]\n";
				break;
			case DocNodeBlockQuote::alert_important:
				documentation += "> [!IMPORTANT]\n";
				break;
			case DocNodeBlockQuote::alert_warning:
				documentation += "> [!WARNING]\n";
				break;
			case DocNodeBlockQuote::alert_caution:
				documentation += "> [!CAUTION]\n";
				break;
			}
			documentation += "> ";
			for (const DocNode& child_node : deref(node_data.children)) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix + "> ", documentation, max_length,
				        options)) {
					return false;
				}
			}
			insert_linebreak(documentation, !(options & without_linebreak), max_length);
		}
		break;
	case DocNode::node_table:
		if (options & without_linebreak) {
			return true;
		}
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
				return false;
			}
		}
		break;
	case DocNode::node_table_head:
		{
			const auto& node_data = node.as<DocNodeBlock>();
			std::vector<std::size_t> column_sizes;
			column_sizes.reserve(node_data.children.size());
			documentation += "|";
			for (const DocNode& column : deref(node_data.children)) {
				std::string column_text;
				mintdoc_to_string(dictionary, context, column, {}, column_text, max_length, options);
				column_sizes.push_back(std::max(column_text.length() - 1, std::size_t(3)));
				documentation += column_text;
			}
			documentation += "\n|";
			auto it = column_sizes.begin();
			for (const DocNode& column : deref(node_data.children)) {
				const auto& column_data = column.as<DocNodeTableColumn>();
				switch (column_data.align) {
				case DocNodeTableColumn::align_auto:
					documentation += std::string(*it++, '-') + '|';
					break;
				case DocNodeTableColumn::align_left:
					documentation += ':' + std::string((*it++) - 1, '-') + '|';
					break;
				case DocNodeTableColumn::align_center:
					documentation += ':' + std::string((*it++) - 2, '-') + ":|";
					break;
				case DocNodeTableColumn::align_right:
					documentation += std::string((*it++) - 1, '-') + ":|";
					break;
				}
			}
			documentation += "\n";
		}
		break;
	case DocNode::node_table_column:
		for (const DocNode& child_node : deref(node.as<DocNodeTableColumn>().children)) {
			mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
		}
		documentation += "|";
		break;
	case DocNode::node_table_body:
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
		}
		documentation += "\n";
		break;
	case DocNode::node_table_row:
		documentation += "|";
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
		}
		documentation += "\n";
		break;
	case DocNode::node_table_item:
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
		}
		documentation += "|";
		break;
	case DocNode::node_list:
		if (options & without_linebreak) {
			return true;
		}
		{
			const auto& node_data = node.as<DocNodeList>();
			std::size_t index = 0;
			for (const DocNode& child_node : deref(node_data.children)) {
				documentation += indent(node_data.indent);
				documentation += node_data.ordered ? std::to_string(++index) + ". " : "* ";
				mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
			}
		}
		break;
	case DocNode::node_item:
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
		}
		break;
	case DocNode::node_link:
		{
			const auto& node_data = node.as<DocNodeLink>();
			if (!(options & without_links)) {
				documentation += node_data.wiki_style ? "[[" : "[";
			}
			bool state = true;
			for (const DocNode& child_node : deref(node_data.children)) {
				state = mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options);
				if (!state) {
					break;
				}
			}
			if (!(options & without_links)) {
				if (node_data.wiki_style) {
					if (!node_data.children.empty()) {
						documentation += "|";
					}
					documentation += node_data.url + "]]";
				}
				else {
					documentation += "](" + node_data.url + ")";
				}
			}
			return state;
		}
		break;
	case DocNode::node_del:
		documentation += "~~";
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
				documentation += "~~";
				return false;
			}
		}
		documentation += "~~";
		break;
	case DocNode::node_emph:
		documentation += '*';
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
				documentation += "*";
				return false;
			}
		}
		documentation += '*';
		break;
	case DocNode::node_strong:
		documentation += "**";
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
				documentation += "**";
				return false;
			}
		}
		documentation += "**";
		break;
	case DocNode::node_strong_emph:
		documentation += "***";
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
				documentation += "***";
				return false;
			}
		}
		documentation += "***";
		break;
	case DocNode::node_code_block:
		{
			const auto& node_data = node.as<DocNodeCodeBlock>();
			if (node_data.fenced) {
				documentation += std::string(node_data.fence_length, node_data.fence_char);
			}
			else if (!(options & without_unfenced_code)) {
			}
			if (node_data.info) {
				documentation += *node_data.info + '\n';
			}
			for (const DocNode& child_node : deref(node_data.children)) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					documentation += std::string(node_data.fence_length, node_data.fence_char);
					return false;
				}
			}
			documentation += std::string(node_data.fence_length, node_data.fence_char);
		}
		break;
	case DocNode::node_custom_block:
		break;
	case DocNode::node_paragraph:
		for (const DocNode& child_node : deref(node.as<DocNodeBlock>().children)) {
			if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
				return false;
			}
		}
		insert_linebreak(documentation, !(options & without_linebreak), max_length);
		break;
	case DocNode::node_heading:
		if (options & without_linebreak) {
			return true;
		}
		{
			const auto& node_data = node.as<DocNodeHeading>();
			documentation += std::string(node_data.level, '#') + ' ';
			for (const DocNode& child_node : deref(node_data.children)) {
				if (!mintdoc_to_string(dictionary, context, child_node, prefix, documentation, max_length, options)) {
					return false;
				}
			}
			insert_linebreak(documentation, !(options & without_linebreak), max_length);
		}
		break;
	case DocNode::node_code:
	case DocNode::node_text:
		return insert_pritable_text(documentation, node.as<DocNodeLiteral>().str, max_length);
		break;
	case DocNode::node_html:
		documentation += '<' + node.as<DocNodeLiteral>().str + '>';
		break;
	case DocNode::node_softbreak:
		break;
	case DocNode::node_linebreak:
		insert_linebreak(documentation, !(options & without_linebreak), max_length);
		insert_pritable_text(documentation, prefix, max_length);
		break;
	case DocNode::node_thematic_break:
		if (!(options & without_linebreak)) {
			documentation += "---\n\n";
		}
		break;
	case DocNode::node_custom_inline:
		break;
	case DocNode::node_image:
		break;
	case DocNode::node_symbol_link:
		{
			const auto& node_data = node.as<DocNodeSymbolLink>();
			const std::string target_symbol = symbol_link_target(node_data, context);
			if (options & without_links) {
				documentation += "`";
				if (!insert_pritable_text(documentation, target_symbol, max_length)) {
					documentation += "`";
					return false;
				}
				documentation += "`";
			}
			else {
				switch (node_data.tag_type) {
				case DocNodeSymbolLink::no_tag:
					if (Module* module = dictionary.find_definition_module(node_data.symbol)) {
						documentation += external_link(node_data.symbol, module->name,
						    module->links.at(node_data.symbol));
					}
					else {
						documentation += external_link(node_data.symbol);
					}
					break;

				case DocNodeSymbolLink::see_tag:
					if (Module* module = dictionary.find_definition_module(target_symbol)) {
						documentation += internal_link(node_data.symbol, module->links.at(target_symbol));
					}
					else {
						documentation += external_link(node_data.symbol);
					}
					break;

				case DocNodeSymbolLink::module_tag:
					documentation += external_link(node_data.symbol);
					break;
				}
			}
		}
		break;
	}

	return true;
}

void GollumGenerator::generate_script_module(FILE* file, const Dictionary& dictionary, const Module& module) {

	trace("module", module.name, brief(dictionary, *module.doc));
	std::println(file, "# Module `{}`\n", module.name);

	std::println(file, "## Description\n");
	std::println(file, "`load {}`\n", module.name);
	std::print(file, "{}", doc_from_mintdoc(dictionary, *module.doc));

	for (const auto& type : module.elements) {

		switch (type.first) {
		case Definition::package_definition:
			std::println(file, "## Packages\n");
			break;
		case Definition::constant_definition:
			std::println(file, "## Constants\n");
			break;
		case Definition::class_definition:
			std::println(file, "## Classes\n");
			break;
		case Definition::enum_definition:
			std::println(file, "## Enums\n");
			break;
		case Definition::function_definition:
			std::println(file, "## Functions\n");
			break;
		}

		for (const auto& def : type.second) {
			switch (def.second->type) {
			case Definition::package_definition:
				std::println(file, "* {}", external_link(def.first, "Package " + def.first));
				break;

			case Definition::enum_definition:
				std::println(file, "### {}\n", def.first);
				{

					const auto& instance = def.second->as<Enum>();
					trace("enum", def.first, brief(dictionary, *instance.doc, &instance));

					std::print(file, "{}", doc_from_mintdoc(dictionary, *instance.doc, &instance));
					std::println(file, "| Constant | Value | Description |"
					                   "|----------|-------|-------------|\n");

					for (const Definition& definition : dictionary.enum_definitions(instance)) {
						if (definition.type == Definition::constant_definition) {
							const auto& value = definition.as<Constant>();
							std::println(file, "| {} | `{}` | {} |",
							    internal_link(definition.symbol(), module.links.at(definition.name)), value.value,
							    definition_brief(dictionary, definition));
						}
					}
				}
				break;

			case Definition::class_definition:
				std::println(file, "### {}\n", def.first);
				{

					const auto& instance = def.second->as<Class>();
					trace("class", def.first, brief(dictionary, *instance.doc, &instance));

					std::print(file, "{}", doc_from_mintdoc(dictionary, *instance.doc, &instance));

					if (!instance.bases.empty()) {
						std::println(file, "#### Inherits\n");
						std::string context = instance.context();
						for (const std::string& base : instance.bases) {
							if (Module* script = dictionary.find_definition_module(base)) {
								std::println(file, "* {}", external_link(base, script->name, script->links.at(base)));
							}
							else if (const auto path = std::format("{}.{}", context, base);
							    Module* script = dictionary.find_definition_module(path)) {
								std::println(file, "* {}", external_link(path, script->name, script->links.at(path)));
							}
							else {
								std::println(file, "* {}", external_link(base));
							}
						}
						std::println(file);
					}

					std::vector<MemberBrief> public_members;
					std::vector<MemberBrief> protected_members;
					std::vector<MemberBrief> package_members;
					std::vector<MemberBrief> private_members;

					for (const Definition& definition : dictionary.class_definitions(instance)) {
						if (instance.name == definition.context()) {
							const auto member_brief = MemberBrief {
							    .modifiers = definition_modifiers(definition),
							    .link = internal_link(definition.symbol(), module.links.at(definition.name)),
							    .brief = definition_brief(dictionary, definition),
							};
							if (definition.flags & mint::Reference::private_visibility) {
								private_members.push_back(member_brief);
							}
							else if (definition.flags & mint::Reference::protected_visibility) {
								protected_members.push_back(member_brief);
							}
							else if (definition.flags & mint::Reference::package_visibility) {
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

				std::println(file);
				break;

			default:
				std::println(file, "* {}", internal_link(def.first, module.links.at(def.first)));
				break;
			}
		}

		std::println(file);
	}

	std::println(file, "## Descriptions\n");

	for (const auto& def : module.definitions) {
		switch (def.second->type) {
		case Definition::constant_definition:
			std::println(file, "### {}\n", def.first);
			{
				const Constant& instance = def.second->as<Constant>();
				trace("constant", def.first, brief(dictionary, *instance.doc, &instance));
				std::println(file, "`{}`\n", instance.value.empty() ? "none" : instance.value);
				std::print(file, "{}", doc_from_mintdoc(dictionary, *instance.doc, &instance));
			}
			break;

		case Definition::function_definition:
			std::println(file, "### {}\n", def.first);
			{
				const Function& instance = def.second->as<Function>();
				trace("function", def.first);
				for (const Function::Signature& signature : deref(instance.signatures)) {
					infos(signature.format, brief(dictionary, *signature.doc, &instance));
					std::println(file, "`{}`\n", signature.format);
					std::print(file, "{}", doc_from_mintdoc(dictionary, *signature.doc, &instance));
				}
			}
			break;

		default:
			break;
		}
	}
}

void GollumGenerator::generate_group_module(FILE* file, const Dictionary& dictionary, const Module& module) {

	trace("module group", module.name);
	std::println(file, "# Module `{}`\n", module.name);

	std::println(file, "## Description\n");
	std::print(file, "{}", doc_from_mintdoc(dictionary, *module.doc));

	for (const auto& type : module.elements) {

		switch (type.first) {
		case Definition::package_definition:
			std::println(file, "## Packages\n");
			break;
		case Definition::constant_definition:
			std::println(file, "## Constants\n");
			break;
		case Definition::class_definition:
			std::println(file, "## Classes\n");
			break;
		case Definition::enum_definition:
			std::println(file, "## Enums\n");
			break;
		case Definition::function_definition:
			std::println(file, "## Functions\n");
			break;
		}

		for (const auto& def : type.second) {
			switch (type.first) {
			case Definition::package_definition:
				std::println(file, "* {}", external_link(def.first, "Package " + def.first));
				break;

			default:
				if (Module* script = dictionary.find_definition_module(def.first)) {
					std::println(file, "* {}", external_link(def.first, script->name, script->links.at(def.first)));
				}
				else {
					std::println(file, "* {}", external_link(def.first));
				}
				break;
			}
		}

		std::println(file);
	}
}

void GollumGenerator::generate_package(FILE* file, const Dictionary& dictionary, const Package& package) {

	trace("package", package.name, brief(dictionary, *package.doc, &package));
	std::println(file, "# Package `{}`\n", package.name);

	std::println(file, "## Description\n");
	std::print(file, "{}", doc_from_mintdoc(dictionary, *package.doc, &package));

	auto elements = std::map<Definition::Type, std::map<std::string, std::reference_wrapper<const Definition>>>();
	for (const Definition& definition : dictionary.package_definitions(package)) {
		elements[definition.type].emplace(definition.name, definition);
	}

	for (const auto& type : elements) {

		switch (type.first) {
		case Definition::package_definition:
			std::println(file, "## Packages\n");
			break;
		case Definition::constant_definition:
			std::println(file, "## Constants\n");
			break;
		case Definition::class_definition:
			std::println(file, "## Classes\n");
			break;
		case Definition::enum_definition:
			std::println(file, "## Enums\n");
			break;
		case Definition::function_definition:
			std::println(file, "## Functions\n");
			break;
		}

		for (const auto& def : type.second) {
			switch (type.first) {
			case Definition::package_definition:
				std::println(file, "* {}", external_link(def.first, "Package " + def.first));
				break;

			default:
				if (Module* script = dictionary.find_definition_module(def.first)) {
					std::println(file, "* {}", external_link(def.first, script->name, script->links.at(def.first)));
				}
				else {
					std::println(file, "* {}", external_link(def.first));
				}
				break;
			}
		}

		std::println(file);
	}
}
