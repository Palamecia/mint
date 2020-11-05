#include "generators/gollumgenerator.h"

#include <system/filesystem.h>
#include <memory/reference.h>
#include <algorithm>
#include <sstream>
#include <regex>
#include <set>

using namespace std;
using namespace mint;

static string indent(size_t count) {
	string str;
	for (size_t i = 0; i < count; ++i) {
		str += "   ";
	}
	return str;
}

static string flags_to_modifiers(uint64_t flags) {

	string modifiers;

	if (flags & Reference::private_visibility) {
		modifiers += "`-` ";
	}
	else if (flags & Reference::protected_visibility) {
		modifiers += "`#` ";
	}
	else if (flags & Reference::package_visibility) {
		modifiers += "`~` ";
	}
	else {
		modifiers += "`+` ";
	}

	if (flags & Reference::global) {
		modifiers += "`@` ";
	}

	if ((flags & Reference::const_value) && (flags & Reference::const_address)) {
		modifiers += "`const` ";
	}
	else {
		if (flags & Reference::const_value) {
			modifiers += "`%` ";
		}
		if (flags & Reference::const_address) {
			modifiers += "`$` ";
		}
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

void GollumGenerator::setupLinks(Dictionnary *dictionnary, Module *module) {

	set<string> links;

	for (auto def : module->definitions) {

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

void GollumGenerator::generateModuleList(Dictionnary *dictionnary, const string &path, const vector<Module *> modules) {

	if (FILE *file = fopen((path + FileSystem::separator + "Modules.md").c_str(), "w")) {

		for (Module *module : modules) {
			auto level = count(module->name.begin(), module->name.end(), '.');
			string base_name = level ? module->name.substr(module->name.rfind('.') + 1) : module->name;
			fprintf(file, "%s* [[%s|%s]]\n", indent(level).c_str(), base_name.c_str(), module->name.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generateModule(Dictionnary *dictionnary, const string &path, Module *module) {

	string module_path = path + FileSystem::separator + module->name + ".md";

	if (FILE *file = open_file(module_path.c_str(), "w")) {

		switch (module->type) {
		case Module::script:
			generateModule(dictionnary, file, module);
			break;

		case Module::group:
			generateModuleGroup(dictionnary, file, module);
			break;
		}

		fclose(file);
	}
}

void GollumGenerator::generatePackageList(Dictionnary *dictionnary, const string &path, const vector<Package *> packages) {

	if (FILE *file = fopen((path + FileSystem::separator + "Packages.md").c_str(), "w")) {

		for (Package *package : packages) {
			auto level = count(package->name.begin(), package->name.end(), '.');
			string base_name = level ? package->name.substr(package->name.rfind('.') + 1) : package->name;
			fprintf(file, "%s* [[%s|Package %s]]\n", indent(level).c_str(), base_name.c_str(), package->name.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generatePackage(Dictionnary *dictionnary, const string &path, Package *package) {

	string package_path = path + FileSystem::separator + "Package " + package->name + ".md";

	if (FILE *file = open_file(package_path.c_str(), "w")) {
		generatePackage(dictionnary, file, package);
		fclose(file);
	}
}

void GollumGenerator::generatePageList(Dictionnary *dictionnary, const string &path, const vector<Page *> pages) {

	if (FILE *file = fopen((path + FileSystem::separator + "Pages.md").c_str(), "w")) {

		for (Page *page : pages) {
			fprintf(file, "* [[%s]]\n", page->name.c_str());
		}

		fclose(file);
	}
}

void GollumGenerator::generatePage(Dictionnary *dictionnary, const string &path, Page *page) {

	string package_path = path + FileSystem::separator + page->name + ".md";

	if (FILE *file = open_file(package_path.c_str(), "w")) {
		fprintf(file, "%s", docFromMintdoc(dictionnary, page->doc).c_str());
		fclose(file);
	}
}

string GollumGenerator::docFromMintdoc(Dictionnary *dictionnary, stringstream &stream, Definition* context) const {

	string token;
	bool finished = false;
	bool new_line = true;
	bool suspect_tag = false;
	auto block_start = string::npos;
	Dictionnary::TagType tag_type = Dictionnary::no_tag;

	string documentation;

	while (!finished && !stream.eof()) {
		switch (int c = stream.get()) {
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
						target_symbol = context->name.substr(0, context->name.rfind('.')) + "." + symbol;
						break;
					}
				}

				switch (tag_type) {
				case Dictionnary::no_tag:
					if (Module *module = dictionnary->findDefinitionModule(symbol)) {
						token.replace(block_start, string::npos, "[[" + symbol + "|" + module->name + "#" + module->links.at(symbol) + "]]");
					}
					else {
						token.replace(block_start, string::npos, "[[" + symbol + "]]");
					}
					break;

				case Dictionnary::see_tag:
					if (Module *module = dictionnary->findDefinitionModule(target_symbol)) {
						token.replace(block_start, string::npos, "[" + symbol + "](#" + module->links.at(target_symbol) + ")");
					}
					else {
						token.replace(block_start, string::npos, "[[" + symbol + "]]");
					}
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
				documentation += ' ';
			}
			documentation += token;
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
			if (!new_line && !token.empty()) {
				documentation += ' ';
			}
			documentation += token + "\n";
			new_line = true;
			token.clear();
			break;

		default:
			if (isspace(c)) {
				if (suspect_tag) {
					if (block_start != string::npos) {
						tag_type = dictionnary->getTagType(token.substr(block_start + 1));
						token.erase(block_start + 1);
					}
					else {
						tag_type = dictionnary->getTagType(token);
						token.clear();
					}
				}
				else {
					if (!new_line && !token.empty()) {
						documentation += ' ';
					}
					documentation += token;
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

string GollumGenerator::docFromMintdoc(Dictionnary *dictionnary, const string &doc, Definition *context) const {
	stringstream stream(doc);
	return docFromMintdoc(dictionnary, stream, context);
}

string GollumGenerator::definitionBrief(Dictionnary *dictionnary, Definition *definition) const {

	string brief;

	switch (definition->type) {
	case Definition::package_definition:
		if (Package *instance = static_cast<Package *>(definition)) {
			brief = docFromMintdoc(dictionnary, instance->doc, instance);
		}
		break;

	case Definition::enum_definition:
		if (Enum *instance = static_cast<Enum *>(definition)) {
			brief = docFromMintdoc(dictionnary, instance->doc, instance);
		}
		break;

	case Definition::class_definition:
		if (Class *instance = static_cast<Class *>(definition)) {
			brief = docFromMintdoc(dictionnary, instance->doc, instance);
		}
		break;

	case Definition::constant_definition:
		if (Constant *instance = static_cast<Constant *>(definition)) {
			brief = docFromMintdoc(dictionnary, instance->doc, instance);
		}
		break;

	case Definition::function_definition:
		if (Function *instance = static_cast<Function *>(definition)) {
			if (!instance->signatures.empty()) {
				brief = docFromMintdoc(dictionnary, instance->signatures.front()->doc, instance);
			}
		}
		break;

	}

	brief = regex_replace(brief, regex("\\n+"), " ");
	brief = regex_replace(brief, regex("^[\\s]+"), "");
	brief = regex_replace(brief, regex("\\[\\[(.*)\\|.*\\]\\]"), "$1");
	brief = regex_replace(brief, regex("\\[(.*)\\]\\(.*\\)"), "$1");

	if (brief.size() > 80) {
		brief = brief.substr(0, 77) + "...";
	}

	return brief;
}

void GollumGenerator::generateModule(Dictionnary *dictionnary, FILE *file, Module *module) {

	printf("module %s :\n{{{\n%s\n}}}\n\n\n", module->name.c_str(), module->doc.c_str());

	fprintf(file, "# Module\n\n"
				  "`load %s`\n\n"
				  "%s\n\n",
			module->name.c_str(),
			docFromMintdoc(dictionnary, module->doc).c_str());

	for (auto type : module->elements) {

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

		for (auto def : type.second) {
			switch (def.second->type) {
			case Definition::package_definition:
				fprintf(file, "* [[%s|Package %s]]\n", def.first.c_str(), def.first.c_str());
				break;

			case Definition::enum_definition:
				fprintf(file, "## %s\n\n", def.first.c_str());
				if (Enum* instance = static_cast<Enum *>(def.second)) {

					printf(" >> enum %s :\n{{{\n%s\n}}}\n\n\n", def.first.c_str(), instance->doc.c_str());
					fprintf(file, "%s\n\n", docFromMintdoc(dictionnary, instance->doc, instance).c_str());
					fprintf(file, "| Constant | Value | Description |\n"
								  "|----------|-------|-------------|\n");

					auto start = module->definitions.find(def.first);
					for (auto i = ++start;  i != module->definitions.end() && i->first.find(def.first + ".") == 0; ++i) {
						if (i->second->type == Definition::constant_definition) {
							if (Constant* value = static_cast<Constant *>(i->second)) {
								string name = i->first.substr(i->first.rfind('.') + 1);
								fprintf(file, "| [%s](#%s) | `%s` | %s |\n",
										name.c_str(),
										module->links.at(i->first).c_str(),
										value->value.c_str(),
										docFromMintdoc(dictionnary, value->doc, i->second).c_str());
							}
						}
					}
				}

				fprintf(file, "\n");
				break;

			case Definition::class_definition:
				fprintf(file, "## %s\n\n", def.first.c_str());
				if (Class* instance = static_cast<Class *>(def.second)) {

					printf(" >> class %s :\n{{{\n%s\n}}}\n\n\n", def.first.c_str(), instance->doc.c_str());
					fprintf(file, "%s\n\n", docFromMintdoc(dictionnary, instance->doc, instance).c_str());

					if (!instance->bases.empty()) {
						fprintf(file, "### Inherits\n\n");
						string context = instance->name.substr(0, instance->name.rfind('.'));
						for (const string &base : instance->bases) {
							if (Module *script = dictionnary->findDefinitionModule(base)) {
								fprintf(file, "* [[%s|%s#%s]]\n",
										base.c_str(),
										script->name.c_str(),
										script->links.at(base).c_str());
							}
							else if (Module *script = dictionnary->findDefinitionModule(context + "." + base)) {
								fprintf(file, "* [[%s|%s#%s]]\n",
										(context + "." + base).c_str(),
										script->name.c_str(),
										script->links.at(context + "." + base).c_str());
							}
							else {
								fprintf(file, "* [[%s]]\n", base.c_str());
							}
						}
						fprintf(file, "\n");
					}

					fprintf(file, "### Members\n\n");
					fprintf(file, "| Modifiers | Member | Description |\n"
								  "|-----------|--------|-------------|\n");

					auto start = module->definitions.find(def.first);
					for (auto i = ++start;  i != module->definitions.end() && i->first.find(def.first + ".") == 0; ++i) {
						auto pos = i->first.rfind('.');
						if (instance->name == i->first.substr(0, pos)) {
							fprintf(file, "| %s | [%s](#%s) | %s |\n",
									flags_to_modifiers(i->second->flags).c_str(),
									i->first.substr(pos + 1).c_str(),
									module->links.at(i->first).c_str(),
									definitionBrief(dictionnary, i->second).c_str());
						}
					}
				}

				fprintf(file, "\n");
				break;

			default:
				fprintf(file, "* [%s](#%s)\n", def.first.c_str(), module->links.at(def.first).c_str());
				break;
			}
		}

		fprintf(file, "\n");
	}

	fprintf(file, "# Descriptions\n\n");

	for (auto def : module->definitions) {
		switch (def.second->type) {
		case Definition::constant_definition:
			fprintf(file, "## %s\n\n", def.first.c_str());
			if (Constant* instance = static_cast<Constant *>(def.second)) {
				printf(" >> constant %s :\n{{{\n%s\n}}}\n\n\n", def.first.c_str(), instance->doc.c_str());
				fprintf(file, "`%s`\n\n", instance->value.empty() ? "none" : instance->value.c_str());
				fprintf(file, "%s\n\n", docFromMintdoc(dictionnary, instance->doc, instance).c_str());
			}
			break;

		case Definition::function_definition:
			fprintf(file, "## %s\n\n", def.first.c_str());
			if (Function* instance = static_cast<Function *>(def.second)) {
				printf(" >> function %s :\n", def.first.c_str());
				for (auto signature : instance->signatures) {
					printf("         %s :\n{{{\n%s\n}}}\n\n\n", signature->format.c_str(), signature->doc.c_str());
					fprintf(file, "`%s`\n\n", signature->format.c_str());
					fprintf(file, "%s\n\n", docFromMintdoc(dictionnary, signature->doc, instance).c_str());
				}
			}
			break;

		default:
			break;
		}
	}
}

void GollumGenerator::generateModuleGroup(Dictionnary *dictionnary, FILE *file, Module *module) {

	printf("module group %s :\n{{{\n%s\n}}}\n\n\n", module->name.c_str(), module->doc.c_str());

	fprintf(file, "# Description\n\n%s\n\n",
			docFromMintdoc(dictionnary, module->doc).c_str());

	for (Module *script : dictionnary->childModules(module)) {
		for (auto type : script->elements) {
			for (auto def : type.second) {
				module->elements[type.first].insert(def);
			}
		}
	}

	for (auto type : module->elements) {

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

		for (auto def : type.second) {
			switch (type.first) {
			case Definition::package_definition:
				fprintf(file, "* [[%s|Package %s]]\n", def.first.c_str(), def.first.c_str());
				break;

			default:
				if (Module* script = dictionnary->findDefinitionModule(def.first)) {
					fprintf(file, "* [[%s|%s#%s]]\n", def.first.c_str(), script->name.c_str(), script->links.at(def.first).c_str());
				}
				else {
					fprintf(file, "* [[%s]]\n", def.first.c_str());
				}
				break;
			}
		}

		fprintf(file, "\n");
	}
}

void GollumGenerator::generatePackage(Dictionnary *dictionnary, FILE *file, Package *package) {

	printf(" >> package %s :\n{{{\n%s\n}}}\n\n\n", package->name.c_str(), package->doc.c_str());

	fprintf(file, "# Description\n\n%s\n\n",
			docFromMintdoc(dictionnary, package->doc, package).c_str());

	map<Definition::Type, map<string, Definition *>> elements;

	for (Definition *definition : dictionnary->packageDefinitions(package)) {
		elements[definition->type].emplace(definition->name, definition);
	}

	for (auto type : elements) {

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

		for (auto def : type.second) {
			switch (type.first) {
			case Definition::package_definition:
				fprintf(file, "* [[%s|Package %s]]\n", def.first.c_str(), def.first.c_str());
				break;

			default:
				if (Module* script = dictionnary->findDefinitionModule(def.first)) {
					fprintf(file, "* [[%s|%s#%s]]\n", def.first.c_str(), script->name.c_str(), script->links.at(def.first).c_str());
				}
				else {
					fprintf(file, "* [[%s]]\n", def.first.c_str());
				}
				break;
			}
		}

		fprintf(file, "\n");
	}
}
