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

#ifndef MINTDOC_GENERATORS_GOLLUM_GENERATOR_H
#define MINTDOC_GENERATORS_GOLLUM_GENERATOR_H

#include "abstract_generator.h"
#include "definition.h"
#include "doc_node.h"
#include "module.h"
#include "page.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

class GollumGenerator : public AbstractGenerator {
public:
	static constexpr std::size_t default_brief_length = 80;

	GollumGenerator() = default;

	void setup_links(const Dictionary& dictionary, Module& module) override;

	void generate_module_list(const Dictionary& dictionary, const std::filesystem::path& path,
	    const std::vector<std::reference_wrapper<const Module>>& modules) override;
	void generate_module(const Dictionary& dictionary, const std::filesystem::path& path, const Module& module) override;

	void generate_package_list(const Dictionary& dictionary, const std::filesystem::path& path,
	    const std::vector<std::reference_wrapper<const Package>>& packages) override;
	void generate_package(const Dictionary& dictionary, const std::filesystem::path& path,
	    const Package& package) override;

	void generate_page_list(const Dictionary& dictionary, const std::filesystem::path& path,
	    const std::vector<std::reference_wrapper<const Page>>& pages) override;
	void generate_page(const Dictionary& dictionary, const std::filesystem::path& path, const Page& page) override;

private:
	static std::string external_link(const std::string& label, const std::string& target, const std::string& section);
	static std::string external_link(const std::string& label, const std::string& target);
	static std::string external_link(const std::string& target);
	static std::string internal_link(const std::string& label, const std::string& section);
	static std::string brief(const Dictionary& dictionary, const DocNode& node, const Definition* context = nullptr,
	    std::size_t max_length = default_brief_length);
	static std::string doc_from_mintdoc(const Dictionary& dictionary, const DocNode& node,
	    const Definition* context = nullptr);
	static std::string definition_brief(const Dictionary& dictionary, const Definition& definition);

	using FormatOptions = std::uint8_t;
	static constexpr FormatOptions no_options = 0x00;
	static constexpr FormatOptions without_linebreak = 0x01;
	static constexpr FormatOptions without_links = 0x02;
	static constexpr FormatOptions without_unfenced_code = 0x04;

	static bool mintdoc_to_string(const Dictionary& dictionary, const Definition* context, const DocNode& node,
	    const std::string& prefix, std::string& documentation, std::size_t& max_length,
	    FormatOptions options = no_options);

	static void generate_script_module(FILE* file, const Dictionary& dictionary, const Module& module);
	static void generate_group_module(FILE* file, const Dictionary& dictionary, const Module& module);
	static void generate_package(FILE* file, const Dictionary& dictionary, const Package& package);
};

#endif // MINTDOC_GENERATORS_GOLLUM_GENERATOR_H
