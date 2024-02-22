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

#include <mint/system/terminal.h>
#include <mint/system/filesystem.h>
#include <mint/memory/reference.h>
#include <algorithm>
#include <sstream>
#include <regex>
#include <set>

using namespace std;
using namespace mint;

static void trace(const string &type, const string &name) {
	mint::printf(stdout, "\033[1;34m >> \033[3;31m%s \033[0m%s\n", type.c_str(), name.c_str());
}

static void infos(const string &info) {
	mint::printf(stdout, "\033[1;30m    %s\033[0m\n", info.c_str());
}

static string indent(size_t count) {
	string str;
	for (size_t i = 0; i < count; ++i) {
		str += "   ";
	}
	return str;
}

static string definition_modifiers(Definition *definition) {

	string modifiers;

	if (definition->flags & Reference::private_visibility) {
		modifiers += "`-` ";
	}
	else if (definition->flags & Reference::protected_visibility) {
		modifiers += "`#` ";
	}
	else if (definition->flags & Reference::package_visibility) {
		modifiers += "`~` ";
	}
	else {
		modifiers += "`+` ";
	}

	if (definition->flags & Reference::final_member) {
		modifiers += "`final` ";
	}
	else if (definition->flags & Reference::override_member) {
		modifiers += "`override` ";
	}

	if (definition->flags & Reference::global) {
		modifiers += "`@` ";
	}

	if ((definition->flags & Reference::const_value) && (definition->flags & Reference::const_address)) {
		modifiers += "`const` ";
	}
	else {
		if (definition->flags & Reference::const_value) {
			modifiers += "`%` ";
		}
		if (definition->flags & Reference::const_address) {
			modifiers += "`$` ";
		}
	}

	switch (definition->type) {
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

static void process_script(stringstream &stream, string &token) {

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

GollumGenerator::GollumGenerator() {

}

void GollumGenerator::setup_links(Dictionnary *dictionnary, Module *module) {

	set<string> links;

	for (const auto &def : module->definitions) {

		string link;

		for (char c : def.first) {
			if (isspace(c)) {
				link += '-';
			}
			else if (isalnum(c)) {
				link += c;
			}
			else if (c == '-' || c == '_') {
				link += c;
			}
		}

		int count = 1;
		string suffix;

		while (links.find(link + suffix) != links.end()) {
			stringstream stream;
			stream << '-' << count++;
			suffix = stream.str();
		}

		module->links.emplace(def.first, link + suffix);
		links.emplace(link + suffix);
	}
}

void GollumGenerator::generate_module_list(Dictionnary *dictionnary, const string &path, const vector<Module *> &modules) {

	string file_path = path + FileSystem::separator + "Modules.md";

	if (FILE *file = open_file(file_path.c_str(), "w")) {

		for (Module *module : modules) {
			size_t level = static_cast<size_t>(count(module->name.begin(), module->name.end(), '.'));
			string indent_str = indent(level);
			string base_name = level ? module->name.substr(module->name.rfind('.') + 1) : module->name;
			string brief_str = brief(doc_from_mintdoc(dictionnary, module->doc));
			fprintf(file, "%s* [[%s|%s]] %s\n", indent_str.c_str(), base_name.c_str(), module->name.c_str(), brief_str.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generate_module(Dictionnary *dictionnary, const string &path, Module *module) {

	string module_path = path + FileSystem::separator + module->name + ".md";

	if (FILE *file = open_file(module_path.c_str(), "w")) {

		switch (module->type) {
		case Module::script:
			generate_module(dictionnary, file, module);
			break;

		case Module::group:
			generate_module_group(dictionnary, file, module);
			break;
		}

		fclose(file);
	}
}

void GollumGenerator::generate_package_list(Dictionnary *dictionnary, const string &path, const vector<Package *> &packages) {

	string file_path = path + FileSystem::separator + "Packages.md";

	if (FILE *file = open_file(file_path.c_str(), "w")) {

		for (Package *package : packages) {
			size_t level = static_cast<size_t>(count(package->name.begin(), package->name.end(), '.'));
			string base_name = level ? package->symbol() : package->name;
			string indent_str = indent(level);
			string link_str = external_link(base_name, "Package " + package->name);
			string brief_str = brief(doc_from_mintdoc(dictionnary, package->doc, package));
			fprintf(file, "%s* %s %s\n", indent_str.c_str(), link_str.c_str(), brief_str.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generate_package(Dictionnary *dictionnary, const string &path, Package *package) {

	string package_path = path + FileSystem::separator + "Package " + package->name + ".md";

	if (FILE *file = open_file(package_path.c_str(), "w")) {
		generate_package(dictionnary, file, package);
		fclose(file);
	}
}

void GollumGenerator::generate_page_list(Dictionnary *dictionnary, const string &path, const vector<Page *> &pages) {

	string file_path = path + FileSystem::separator + "Pages.md";

	if (FILE *file = open_file(file_path.c_str(), "w")) {

		for (Page *page : pages) {
			string link_str = external_link(page->name);
			string brief_str = brief(doc_from_mintdoc(dictionnary, page->doc));
			fprintf(file, "* %s %s\n", link_str.c_str(), brief_str.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generate_page(Dictionnary *dictionnary, const string &path, Page *page) {

	string package_path = path + FileSystem::separator + page->name + ".md";

	if (FILE *file = open_file(package_path.c_str(), "w")) {
		string doc_str = doc_from_mintdoc(dictionnary, page->doc);
		fprintf(file, "%s", doc_str.c_str());
		fclose(file);
	}
}

string GollumGenerator::external_link(const string &label, const string &target, const string &section) {
	return "[" + regex_replace(label, regex("\\|"), "&#124;") + "](" + target + "#" + section + ")";
}

string GollumGenerator::external_link(const string &label, const string &target) {
	return "[[" + regex_replace(label, regex("\\|"), "&#124;") + "|" + target + "]]";
}

string GollumGenerator::external_link(const string &target) {
	return "[" + regex_replace(target, regex("\\|"), "&#124;") + "](" + target + ")";
}

string GollumGenerator::internal_link(const string &label, const string &section) {
	return "[" + regex_replace(label, regex("\\|"), "&#124;") + "](#" + section + ")";
}

string GollumGenerator::brief(const string &documentation) {

	string brief = documentation;

	brief = regex_replace(brief, regex("\\n+"), " ");
	brief = regex_replace(brief, regex("^[\\s]+"), "");
	brief = regex_replace(brief, regex("\\[\\[(.+?)\\|.+?\\]\\]"), "$1");
	brief = regex_replace(brief, regex("\\[(.+?)\\]\\(.+?\\)"), "$1");

	if (brief.size() > 80) {
		brief = brief.substr(0, 77);
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

	return regex_replace(brief, regex("\\|"), "&#124;");
}

static bool must_join(char c) {
	return c == '!' || c == ',' || c == '.' || c == ':' || c == ';' || c == '?' || c == ')' || c == ']' || c == '}';
}

string GollumGenerator::doc_from_mintdoc(Dictionnary *dictionnary, stringstream &stream, Definition* context) const {

	string token;
	bool finished = false;
	bool new_line = true;
	bool suspect_tag = false;
	auto block_start = string::npos;
	Dictionnary::TagType tag_type = Dictionnary::no_tag;

	string documentation;

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
			if (block_start != string::npos) {

				string symbol = token.substr(block_start + 1);
				string target_symbol = symbol;

				if (context) {
					switch (context->type) {
					case Definition::package_definition:
					case Definition::enum_definition:
					case Definition::class_definition:
						target_symbol = context->name + "." + symbol;
						break;
					case Definition::constant_definition:
					case Definition::function_definition:
						target_symbol = context->context() + "." + symbol;
						break;
					}
				}

				switch (tag_type) {
				case Dictionnary::no_tag:
					if (Module *module = dictionnary->find_definition_module(symbol)) {
						token.replace(block_start, string::npos, external_link(symbol, module->name, module->links.at(symbol)));
					}
					else {
						token.replace(block_start, string::npos, external_link(symbol));
					}
					break;

				case Dictionnary::see_tag:
					if (Module *module = dictionnary->find_definition_module(target_symbol)) {
						token.replace(block_start, string::npos, internal_link(symbol, module->links.at(target_symbol)));
					}
					else {
						token.replace(block_start, string::npos, external_link(symbol));
					}
					break;

				case Dictionnary::module_tag:
					token.replace(block_start, string::npos, external_link(symbol));
					break;
				}
				tag_type = Dictionnary::no_tag;
				block_start = string::npos;
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
			if (block_start != string::npos) {
				block_start = string::npos;
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
			if (block_start != string::npos) {
				block_start = string::npos;
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
					if (block_start != string::npos) {
						tag_type = dictionnary->get_tag_type(token.substr(block_start + 1));
						token.erase(block_start + 1);
					}
					else {
						tag_type = dictionnary->get_tag_type(token);
						token.clear();
					}
				}
				else if (new_line) {
					token += static_cast<char>(c);
				}
				else {
					if (!new_line && !token.empty() && !must_join(token.front())) {
						documentation += ' ' + token;
					}
					else {
						documentation += token;
					}
					if (!token.empty()) {
						new_line = false;
						token.clear();
					}
					if (tag_type == Dictionnary::no_tag) {
						block_start = string::npos;
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

string GollumGenerator::doc_from_mintdoc(Dictionnary *dictionnary, const string &doc, Definition *context) const {
	stringstream stream(doc);
	return doc_from_mintdoc(dictionnary, stream, context);
}

string GollumGenerator::definition_brief(Dictionnary *dictionnary, Definition *definition) const {

	switch (definition->type) {
	case Definition::package_definition:
		if (Package *instance = static_cast<Package *>(definition)) {
			return brief(doc_from_mintdoc(dictionnary, instance->doc, instance));
		}
		break;

	case Definition::enum_definition:
		if (Enum *instance = static_cast<Enum *>(definition)) {
			return brief(doc_from_mintdoc(dictionnary, instance->doc, instance));
		}
		break;

	case Definition::class_definition:
		if (Class *instance = static_cast<Class *>(definition)) {
			return brief(doc_from_mintdoc(dictionnary, instance->doc, instance));
		}
		break;

	case Definition::constant_definition:
		if (Constant *instance = static_cast<Constant *>(definition)) {
			return brief(doc_from_mintdoc(dictionnary, instance->doc, instance));
		}
		break;

	case Definition::function_definition:
		if (Function *instance = static_cast<Function *>(definition)) {
			if (!instance->signatures.empty()) {
				return brief(doc_from_mintdoc(dictionnary, instance->signatures.front()->doc, instance));
			}
		}
		break;

	}

	return {};
}

void GollumGenerator::generate_module(Dictionnary *dictionnary, FILE *file, Module *module) {

	trace("module", module->name);

	string doc_str = doc_from_mintdoc(dictionnary, module->doc);
	fprintf(file, "# Module\n\n"
				  "`load %s`\n\n"
				  "%s\n\n", module->name.c_str(), doc_str.c_str());

	for (const auto &type : module->elements) {

		switch (type.first) {
		case Definition::package_definition:
			fprintf(file, "# Packages\n\n");
			break;
		case Definition::constant_definition:
			fprintf(file, "# Constants\n\n");
			break;
		case Definition::class_definition:
			fprintf(file, "# Classes\n\n");
			break;
		case Definition::enum_definition:
			fprintf(file, "# Enums\n\n");
			break;
		case Definition::function_definition:
			fprintf(file, "# Functions\n\n");
			break;
		}

		for (const auto &def : type.second) {
			switch (def.second->type) {
			case Definition::package_definition:
				{
					string link_str = external_link(def.first, "Package " + def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;

			case Definition::enum_definition:
				fprintf(file, "## %s\n\n", def.first.c_str());
				if (Enum* instance = static_cast<Enum *>(def.second)) {

					trace("enum", def.first);

					string doc_str = doc_from_mintdoc(dictionnary, instance->doc, instance);
					fprintf(file, "%s\n\n", doc_str.c_str());
					fprintf(file, "| Constant | Value | Description |\n"
								  "|----------|-------|-------------|\n");

					for (Definition *definition : dictionnary->enum_definitions(instance)) {
						if (definition->type == Definition::constant_definition) {
							if (Constant* value = static_cast<Constant *>(definition)) {
								string link_str = internal_link(definition->symbol(), module->links.at(definition->name));
								string brief_str = definition_brief(dictionnary, definition);
								fprintf(file, "| %s | `%s` | %s |\n", link_str.c_str(), value->value.c_str(), brief_str.c_str());
							}
						}
					}
				}

				fprintf(file, "\n");
				break;

			case Definition::class_definition:
				fprintf(file, "## %s\n\n", def.first.c_str());
				if (Class* instance = static_cast<Class *>(def.second)) {

					trace("class", def.first);

					string doc_str = doc_from_mintdoc(dictionnary, instance->doc, instance);
					fprintf(file, "%s\n\n", doc_str.c_str());

					if (!instance->bases.empty()) {
						fprintf(file, "### Inherits\n\n");
						string context = instance->context();
						for (const string &base : instance->bases) {
							if (Module *script = dictionnary->find_definition_module(base)) {
								string link_str = external_link(base, script->name, script->links.at(base));
								fprintf(file, "* %s\n", link_str.c_str());
							}
							else if (Module *script = dictionnary->find_definition_module(context + "." + base)) {
								string link_str = external_link(context + "." + base, script->name, script->links.at(context + "." + base));
								fprintf(file, "* %s\n", link_str.c_str());
							}
							else {
								string link_str = external_link(base);
								fprintf(file, "* %s\n", link_str.c_str());
							}
						}
						fprintf(file, "\n");
					}

					fprintf(file, "### Members\n\n");
					fprintf(file, "| Modifiers | Member | Description |\n"
								  "|-----------|--------|-------------|\n");
					
					for (Definition *definition : dictionnary->class_definitions(instance)) {
						if (instance->name == definition->context()) {
							string modifiers_str = definition_modifiers(definition);
							string link_str = internal_link(definition->symbol(), module->links.at(definition->name));
							string brief_str = definition_brief(dictionnary, definition);
							fprintf(file, "| %s | %s | %s |\n", modifiers_str.c_str(), link_str.c_str(), brief_str.c_str());
						}
					}
				}

				fprintf(file, "\n");
				break;

			default:
				string link_str = internal_link(def.first, module->links.at(def.first));
				fprintf(file, "* %s\n", link_str.c_str());
				break;
			}
		}

		fprintf(file, "\n");
	}

	fprintf(file, "# Descriptions\n\n");

	for (const auto &def : module->definitions) {
		switch (def.second->type) {
		case Definition::constant_definition:
			fprintf(file, "## %s\n\n", def.first.c_str());
			if (Constant* instance = static_cast<Constant *>(def.second)) {
				trace("constant", def.first);
				fprintf(file, "`%s`\n\n", instance->value.empty() ? "none" : instance->value.c_str());
				string doc_str = doc_from_mintdoc(dictionnary, instance->doc, instance);
				fprintf(file, "%s\n\n", doc_str.c_str());
			}
			break;

		case Definition::function_definition:
			fprintf(file, "## %s\n\n", def.first.c_str());
			if (Function* instance = static_cast<Function *>(def.second)) {
				trace("function", def.first);
				for (auto signature : instance->signatures) {
					infos(signature->format);
					fprintf(file, "`%s`\n\n", signature->format.c_str());
					string doc_str = doc_from_mintdoc(dictionnary, signature->doc, instance);
					fprintf(file, "%s\n\n", doc_str.c_str());
				}
			}
			break;

		default:
			break;
		}
	}
}

void GollumGenerator::generate_module_group(Dictionnary *dictionnary, FILE *file, Module *module) {

	trace("module group", module->name);

	string doc_str = doc_from_mintdoc(dictionnary, module->doc);
	fprintf(file, "# Description\n\n%s\n\n", doc_str.c_str());
	
	for (Module *script : dictionnary->child_modules(module)) {
		for (const auto &type : script->elements) {
			for (auto def : type.second) {
				module->elements[type.first].insert(def);
			}
		}
	}

	for (const auto &type : module->elements) {

		switch (type.first) {
		case Definition::package_definition:
			fprintf(file, "# Packages\n\n");
			break;
		case Definition::constant_definition:
			fprintf(file, "# Constants\n\n");
			break;
		case Definition::class_definition:
			fprintf(file, "# Classes\n\n");
			break;
		case Definition::enum_definition:
			fprintf(file, "# Enums\n\n");
			break;
		case Definition::function_definition:
			fprintf(file, "# Functions\n\n");
			break;
		}

		for (const auto &def : type.second) {
			switch (type.first) {
			case Definition::package_definition:
				{
					string link_str = external_link(def.first, "Package " + def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;

			default:
					if (Module* script = dictionnary->find_definition_module(def.first)) {
					string link_str = external_link(def.first, script->name, script->links.at(def.first));
					fprintf(file, "* %s\n", link_str.c_str());
				}
				else {
					string link_str = external_link(def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;
			}
		}

		fprintf(file, "\n");
	}
}

void GollumGenerator::generate_package(Dictionnary *dictionnary, FILE *file, Package *package) {

	trace("package", package->name);

	string doc_str = doc_from_mintdoc(dictionnary, package->doc, package);
	fprintf(file, "# Description\n\n%s\n\n", doc_str.c_str());

	map<Definition::Type, map<string, Definition *>> elements;
	
	for (Definition *definition : dictionnary->package_definitions(package)) {
		elements[definition->type].emplace(definition->name, definition);
	}

	for (const auto &type : elements) {

		switch (type.first) {
		case Definition::package_definition:
			fprintf(file, "# Packages\n\n");
			break;
		case Definition::constant_definition:
			fprintf(file, "# Constants\n\n");
			break;
		case Definition::class_definition:
			fprintf(file, "# Classes\n\n");
			break;
		case Definition::enum_definition:
			fprintf(file, "# Enums\n\n");
			break;
		case Definition::function_definition:
			fprintf(file, "# Functions\n\n");
			break;
		}

		for (const auto &def : type.second) {
			switch (type.first) {
			case Definition::package_definition:
				{
					string link_str = external_link(def.first, "Package " + def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;

			default:
					if (Module* script = dictionnary->find_definition_module(def.first)) {
					string link_str = external_link(def.first, script->name, script->links.at(def.first));
					fprintf(file, "* %s\n", link_str.c_str());
				}
				else {
					string link_str = external_link(def.first);
					fprintf(file, "* %s\n", link_str.c_str());
				}
				break;
			}
		}

		fprintf(file, "\n");
	}
}
