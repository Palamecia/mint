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

#ifndef ABSTRACTGENERATOR_H
#define ABSTRACTGENERATOR_H

#include "dictionary.h"

class AbstractGenerator {
public:
	AbstractGenerator();
	virtual ~AbstractGenerator();

	virtual void setup_links(const Dictionary *dictionary, Module *module) = 0;

	virtual void generate_module_list(const Dictionary *dictionary, const std::string &path,
									  const std::vector<Module *> &modules) = 0;
	virtual void generate_module(const Dictionary *dictionary, const std::string &path, Module *module) = 0;

	virtual void generate_package_list(const Dictionary *dictionary, const std::string &path,
									   const std::vector<Package *> &packages) = 0;
	virtual void generate_package(const Dictionary *dictionary, const std::string &path, Package *package) = 0;

	virtual void generate_page_list(const Dictionary *dictionary, const std::string &path,
									const std::vector<Page *> &pages) = 0;
	virtual void generate_page(const Dictionary *dictionary, const std::string &path, Page *page) = 0;
};

#endif // ABSTRACTGENERATOR_H
