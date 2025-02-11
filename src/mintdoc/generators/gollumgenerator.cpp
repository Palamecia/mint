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

#include "generators/gollumgenerator.h"

#include <mint/system/utf8.h>
#include <mint/system/terminal.h>
#include <mint/system/filesystem.h>
#include <mint/memory/reference.h>
#include <filesystem>
#include <algorithm>
#include <sstream>
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

std::string indent(size_t count) {
	std::string str;
	for (size_t i = 0; i < count; ++i) {
		str += "   ";
	}
	return str;
}

std::string definition_modifiers(Definition *definition) {

	std::string modifiers;

	if (definition->flags & Reference::PRIVATE_VISIBILITY) {
		modifiers += "`-` ";
	}
	else if (definition->flags & Reference::PROTECTED_VISIBILITY) {
		modifiers += "`#` ";
	}
	else if (definition->flags & Reference::PACKAGE_VISIBILITY) {
		modifiers += "`~` ";
	}
	else {
		modifiers += "`+` ";
	}

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

bool must_join(char c) {
	return c == '!' || c == ',' || c == '.' || c == ':' || c == ';' || c == '?' || c == ')' || c == ']' || c == '}';
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

		for (Module *module : modules) {
			size_t level = static_cast<size_t>(count(module->name.begin(), module->name.end(), '.'));
			std::string indent_str = indent(level);
			std::string base_name = level ? module->name.substr(module->name.rfind('.') + 1) : module->name;
			std::string brief_str = brief(doc_from_mintdoc(dictionary, module->doc));
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

		for (Package *package : packages) {
			size_t level = static_cast<size_t>(count(package->name.begin(), package->name.end(), '.'));
			std::string base_name = level ? package->symbol() : package->name;
			std::string indent_str = indent(level);
			std::string link_str = external_link(base_name, "Package " + package->name);
			std::string brief_str = brief(doc_from_mintdoc(dictionary, package->doc, package));
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

		for (const Page *page : pages) {
			std::string link_str = external_link(page->name);
			std::string brief_str = brief(doc_from_mintdoc(dictionary, page->doc));
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

std::string GollumGenerator::brief(const std::string &documentation) {

	std::string brief = documentation;

	brief = regex_replace(brief, std::regex("\\n+"), " ");
	brief = regex_replace(brief, std::regex("^[\\s]+"), "");
	brief = regex_replace(brief, std::regex("\\[\\[(.+?)\\|.+?\\]\\]"), "$1");
	brief = regex_replace(brief, std::regex("\\[(.+?)\\]\\(.+?\\)"), "$1");

	if (brief.size() > 80) {
		brief.resize(77);
		if (count(brief.begin(), brief.end(), '`') % 2) {
			if (brief[76] != '`') {
				brief[76] = '`';
			}
			else {
				brief.pop_back();
			}
		}
		brief += "...";
	}

	return regex_replace(brief, std::regex("\\|"), "&#124;");
}

std::string GollumGenerator::doc_from_mintdoc(const Dictionary *dictionary, std::stringstream &stream,
											  const Definition *context) const {

	std::string token;
	bool finished = false;
	bool new_line = true;
	bool suspect_tag = false;
	auto block_start = std::string::npos;
	Dictionary::TagType tag_type = Dictionary::NO_TAG;

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
				case Dictionary::NO_TAG:
					if (Module *module = dictionary->find_definition_module(symbol)) {
						token.replace(block_start, std::string::npos,
									  external_link(symbol, module->name, module->links.at(symbol)));
					}
					else {
						token.replace(block_start, std::string::npos, external_link(symbol));
					}
					break;

				case Dictionary::SEE_TAG:
					if (Module *module = dictionary->find_definition_module(target_symbol)) {
						token.replace(block_start, std::string::npos,
									  internal_link(symbol, module->links.at(target_symbol)));
					}
					else {
						token.replace(block_start, std::string::npos, external_link(symbol));
					}
					break;

				case Dictionary::MODULE_TAG:
					token.replace(block_start, std::string::npos, external_link(symbol));
					break;
				}
				tag_type = Dictionary::NO_TAG;
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
					if (tag_type == Dictionary::NO_TAG) {
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
}

std::string GollumGenerator::doc_from_mintdoc(const Dictionary *dictionary, const std::string &doc,
											  const Definition *context) const {
	std::stringstream stream(doc);
	return doc_from_mintdoc(dictionary, stream, context);
}

std::string GollumGenerator::definition_brief(const Dictionary *dictionary, const Definition *definition) const {

	switch (definition->type) {
	case Definition::PACKAGE_DEFINITION:
		if (const auto *instance = static_cast<const Package *>(definition)) {
			return brief(doc_from_mintdoc(dictionary, instance->doc, instance));
		}
		break;

	case Definition::ENUM_DEFINITION:
		if (const auto *instance = static_cast<const Enum *>(definition)) {
			return brief(doc_from_mintdoc(dictionary, instance->doc, instance));
		}
		break;

	case Definition::CLASS_DEFINITION:
		if (const auto *instance = static_cast<const Class *>(definition)) {
			return brief(doc_from_mintdoc(dictionary, instance->doc, instance));
		}
		break;

	case Definition::CONSTANT_DEFINITION:
		if (const auto *instance = static_cast<const Constant *>(definition)) {
			return brief(doc_from_mintdoc(dictionary, instance->doc, instance));
		}
		break;

	case Definition::FUNCTION_DEFINITION:
		if (const auto *instance = static_cast<const Function *>(definition)) {
			if (!instance->signatures.empty()) {
				return brief(doc_from_mintdoc(dictionary, instance->signatures.front()->doc, instance));
			}
		}
		break;
	}

	return {};
}

void GollumGenerator::generate_module(const Dictionary *dictionary, FILE *file, const Module *module) {

	trace("module", module->name, brief(module->doc));

	{
		std::string doc_str = doc_from_mintdoc(dictionary, module->doc);
		fprintf(file,
				"# Module\n\n"
				"`load %s`\n\n"
				"%s\n\n",
				module->name.c_str(), doc_str.c_str());
	}

	for (const auto &type : module->elements) {

		switch (type.first) {
		case Definition::PACKAGE_DEFINITION:
			fprintf(file, "# Packages\n\n");
			break;
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "# Constants\n\n");
			break;
		case Definition::CLASS_DEFINITION:
			fprintf(file, "# Classes\n\n");
			break;
		case Definition::ENUM_DEFINITION:
			fprintf(file, "# Enums\n\n");
			break;
		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "# Functions\n\n");
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
				fprintf(file, "## %s\n\n", def.first.c_str());
				if (const auto *instance = static_cast<Enum *>(def.second)) {

					trace("enum", def.first, brief(instance->doc));

					std::string doc_str = doc_from_mintdoc(dictionary, instance->doc, instance);
					fprintf(file, "%s\n\n", doc_str.c_str());
					fprintf(file, "| Constant | Value | Description |\n"
								  "|----------|-------|-------------|\n");

					for (Definition *definition : dictionary->enum_definitions(instance)) {
						if (definition->type == Definition::CONSTANT_DEFINITION) {
							if (auto *value = static_cast<Constant *>(definition)) {
								std::string link_str = internal_link(definition->symbol(),
																	 module->links.at(definition->name));
								std::string brief_str = definition_brief(dictionary, definition);
								fprintf(file, "| %s | `%s` | %s |\n", link_str.c_str(), value->value.c_str(),
										brief_str.c_str());
							}
						}
					}
				}

				fprintf(file, "\n");
				break;

			case Definition::CLASS_DEFINITION:
				fprintf(file, "## %s\n\n", def.first.c_str());
				if (auto *instance = static_cast<Class *>(def.second)) {

					trace("class", def.first, brief(instance->doc));

					std::string doc_str = doc_from_mintdoc(dictionary, instance->doc, instance);
					fprintf(file, "%s\n\n", doc_str.c_str());

					if (!instance->bases.empty()) {
						fprintf(file, "### Inherits\n\n");
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

					fprintf(file, "### Members\n\n");
					fprintf(file, "| Modifiers | Member | Description |\n"
								  "|-----------|--------|-------------|\n");

					for (Definition *definition : dictionary->class_definitions(instance)) {
						if (instance->name == definition->context()) {
							std::string modifiers_str = definition_modifiers(definition);
							std::string link_str = internal_link(definition->symbol(),
																 module->links.at(definition->name));
							std::string brief_str = definition_brief(dictionary, definition);
							fprintf(file, "| %s | %s | %s |\n", modifiers_str.c_str(), link_str.c_str(),
									brief_str.c_str());
						}
					}
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

	fprintf(file, "# Descriptions\n\n");

	for (const auto &def : module->definitions) {
		switch (def.second->type) {
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "## %s\n\n", def.first.c_str());
			if (const Constant *instance = static_cast<Constant *>(def.second)) {
				trace("constant", def.first, brief(instance->doc));
				fprintf(file, "`%s`\n\n", instance->value.empty() ? "none" : instance->value.c_str());
				std::string doc_str = doc_from_mintdoc(dictionary, instance->doc, instance);
				fprintf(file, "%s\n\n", doc_str.c_str());
			}
			break;

		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "## %s\n\n", def.first.c_str());
			if (const Function *instance = static_cast<Function *>(def.second)) {
				trace("function", def.first);
				for (auto *signature : instance->signatures) {
					infos(signature->format, brief(signature->doc));
					fprintf(file, "`%s`\n\n", signature->format.c_str());
					std::string doc_str = doc_from_mintdoc(dictionary, signature->doc, instance);
					fprintf(file, "%s\n\n", doc_str.c_str());
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

	std::string doc_str = doc_from_mintdoc(dictionary, module->doc);
	fprintf(file, "# Description\n\n%s\n\n", doc_str.c_str());

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
			fprintf(file, "# Packages\n\n");
			break;
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "# Constants\n\n");
			break;
		case Definition::CLASS_DEFINITION:
			fprintf(file, "# Classes\n\n");
			break;
		case Definition::ENUM_DEFINITION:
			fprintf(file, "# Enums\n\n");
			break;
		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "# Functions\n\n");
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

	trace("package", package->name, brief(package->doc));

	std::string doc_str = doc_from_mintdoc(dictionary, package->doc, package);
	fprintf(file, "# Description\n\n%s\n\n", doc_str.c_str());

	std::map<Definition::Type, std::map<std::string, Definition *>> elements;

	for (Definition *definition : dictionary->package_definitions(package)) {
		elements[definition->type].emplace(definition->name, definition);
	}

	for (const auto &type : elements) {

		switch (type.first) {
		case Definition::PACKAGE_DEFINITION:
			fprintf(file, "# Packages\n\n");
			break;
		case Definition::CONSTANT_DEFINITION:
			fprintf(file, "# Constants\n\n");
			break;
		case Definition::CLASS_DEFINITION:
			fprintf(file, "# Classes\n\n");
			break;
		case Definition::ENUM_DEFINITION:
			fprintf(file, "# Enums\n\n");
			break;
		case Definition::FUNCTION_DEFINITION:
			fprintf(file, "# Functions\n\n");
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
