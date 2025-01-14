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

#ifndef GOLLUMGENERATOR_H
#define GOLLUMGENERATOR_H

#include "abstractgenerator.h"

class GollumGenerator : public AbstractGenerator {
public:
	GollumGenerator();

	void setup_links(const Dictionary *dictionary, Module *module) override;

	void generate_module_list(const Dictionary *dictionary, const std::string &path,
							  const std::vector<Module *> &modules) override;
	void generate_module(const Dictionary *dictionary, const std::string &path, Module *module) override;

	void generate_package_list(const Dictionary *dictionary, const std::string &path,
							   const std::vector<Package *> &packages) override;
	void generate_package(const Dictionary *dictionary, const std::string &path, Package *package) override;

	void generate_page_list(const Dictionary *dictionary, const std::string &path,
							const std::vector<Page *> &pages) override;
	void generate_page(const Dictionary *dictionary, const std::string &path, Page *page) override;

private:
	static std::string external_link(const std::string &label, const std::string &target, const std::string &section);
	static std::string external_link(const std::string &label, const std::string &target);
	static std::string external_link(const std::string &target);
	static std::string internal_link(const std::string &label, const std::string &section);
	static std::string brief(const std::string &documentation);

	std::string doc_from_mintdoc(const Dictionary *dictionary, std::stringstream &stream,
								 const Definition *context = nullptr) const;
	std::string doc_from_mintdoc(const Dictionary *dictionary, const std::string &doc,
								 const Definition *context = nullptr) const;
	std::string definition_brief(const Dictionary *dictionary, const Definition *definition) const;

	void generate_module(const Dictionary *dictionary, FILE *file, const Module *module);
	void generate_module_group(const Dictionary *dictionary, FILE *file, Module *module);
	void generate_package(const Dictionary *dictionary, FILE *file, const Package *package);
};

#endif // GOLLUMGENERATOR_H
