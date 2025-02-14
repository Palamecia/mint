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

#include "docnode.h"
#include "definition.h"
#include "docparser.h"

#include <sstream>
#include <string>

std::unique_ptr<DocNode> parse_doc(std::stringstream &stream) {
	DocParser parser;
	return parser.parse(stream);
}

std::unique_ptr<DocNode> parse_doc(const std::string &doc) {
	std::stringstream stream(doc);
	return parse_doc(stream);
}

std::string symbol_link_target(const DocNodeSymbolLink *node, const Definition *context) {

	if (!context) {
		return node->symbol;
	}

	switch (context->type) {
	case Definition::PACKAGE_DEFINITION:
	case Definition::ENUM_DEFINITION:
	case Definition::CLASS_DEFINITION:
		return context->name + "." + node->symbol;
	case Definition::CONSTANT_DEFINITION:
	case Definition::FUNCTION_DEFINITION:
		return context->context() + "." + node->symbol;
	}

	return node->symbol;
}
