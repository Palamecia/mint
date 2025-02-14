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

#ifndef MINTDOC_GENERATORS_GOLLUMGENERATOR_H
#define MINTDOC_GENERATORS_GOLLUMGENERATOR_H

#include "abstractgenerator.h"
#include "docnode.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

class GollumGenerator : public AbstractGenerator {
public:
	GollumGenerator() = default;

	void setup_links(const Dictionary *dictionary, Module *module) override;

	void generate_module_list(const Dictionary *dictionary, const std::filesystem::path &path,
							  const std::vector<Module *> &modules) override;
	void generate_module(const Dictionary *dictionary, const std::filesystem::path &path, Module *module) override;

	void generate_package_list(const Dictionary *dictionary, const std::filesystem::path &path,
							   const std::vector<Package *> &packages) override;
	void generate_package(const Dictionary *dictionary, const std::filesystem::path &path, Package *package) override;

	void generate_page_list(const Dictionary *dictionary, const std::filesystem::path &path,
							const std::vector<Page *> &pages) override;
	void generate_page(const Dictionary *dictionary, const std::filesystem::path &path, Page *page) override;

private:
	static std::string external_link(const std::string &label, const std::string &target, const std::string &section);
	static std::string external_link(const std::string &label, const std::string &target);
	static std::string external_link(const std::string &target);
	static std::string internal_link(const std::string &label, const std::string &section);
	static std::string brief(const Dictionary *dictionary, const std::unique_ptr<DocNode> &node,
							 const Definition *context = nullptr, size_t max_length = 80);
	static std::string doc_from_mintdoc(const Dictionary *dictionary, const std::unique_ptr<DocNode> &node,
										const Definition *context = nullptr);
	static std::string definition_brief(const Dictionary *dictionary, const Definition *definition);

	enum FormatOption : std::uint8_t {
		NO_OPTIONS = 0x00,
		WITHOUT_LINEBREAK = 0x01,
		WITHOUT_LINKS = 0x02,
		WITHOUT_UNFENCED_CODE = 0x04
	};

	using FormatOptions = std::underlying_type_t<FormatOption>;

	static bool mintdoc_to_string(const Dictionary *dictionary, const Definition *context,
								  const std::unique_ptr<DocNode> &node, const std::string &prefix,
								  std::string &documentation, std::size_t &max_length,
								  FormatOptions options = NO_OPTIONS);

	void generate_module(const Dictionary *dictionary, FILE *file, const Module *module);
	void generate_module_group(const Dictionary *dictionary, FILE *file, Module *module);
	void generate_package(const Dictionary *dictionary, FILE *file, const Package *package);
};

#endif // MINTDOC_GENERATORS_GOLLUMGENERATOR_H
