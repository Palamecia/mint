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

#include "mint/scheduler/output.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/reference.h"
#include "mint/memory/casttool.h"
#include "mint/memory/class.h"
#include "mint/system/string.h"
#include "mint/system/terminal.h"
#include "mint/system/plugin.h"

using namespace mint;

static std::string reference_value(const Reference &reference) {
	switch (reference.data()->format) {
	case Data::FMT_NONE:
		return MINT_TERM_DARK "none" MINT_TERM_RESET;
	case Data::FMT_NULL:
		return MINT_TERM_DARK "null" MINT_TERM_RESET;
	case Data::FMT_PACKAGE:
		return MINT_TERM_FG_MAGENTA "package:" MINT_TERM_RESET " " + reference.data<Package>()->data->full_name()
			   + MINT_TERM_RESET;
	case Data::FMT_FUNCTION:
		return MINT_TERM_FG_MAGENTA "function:" MINT_TERM_RESET " "
			   + mint::join(reference.data<Function>()->mapping, ", ",
							[ast = AbstractSyntaxTree::instance()](auto it) {
								Module *module = ast->get_module(it->second.handle->module);
								DebugInfo *infos = ast->get_debug_info(it->second.handle->module);
								return std::to_string(it->first) + "@" + ast->get_module_name(module) + "(line "
									   + std::to_string(infos->line_number(it->second.handle->offset)) + ")";
							})
			   + MINT_TERM_RESET;
	case Data::FMT_OBJECT:
		switch (reference.data<Object>()->metadata->metatype()) {
		case Class::OBJECT:
			if (mint::is_class(reference.data<Object>())) {
				return MINT_TERM_FG_MAGENTA "class:" MINT_TERM_RESET " "
					   + reference.data<Object>()->metadata->full_name() + MINT_TERM_RESET;
			}
			return MINT_TERM_FG_MAGENTA "object:" MINT_TERM_RESET " " + reference.data<Object>()->metadata->full_name()
				   + " \033[2m(" + to_string(reference.data()) + ")" MINT_TERM_RESET;
		case Class::STRING:
			return MINT_TERM_FG_GREEN "'" + to_string(reference) + "'" MINT_TERM_RESET;
		case Class::REGEX:
			return MINT_TERM_FG_RED + to_string(reference) + MINT_TERM_RESET;
		case Class::ARRAY:
			return "[ "
				   + mint::join(reference.data<Array>()->values, ", ",
								[](auto it) {
									return reference_value(*it);
								})
				   + " ]";
		case Class::HASH:
			return "{ "
				   + mint::join(reference.data<Hash>()->values, ", ",
								[](auto it) {
									return reference_value(it->first) + ": " + reference_value(it->second);
								})
				   + " }";
		case Class::ITERATOR:
			if (std::optional<WeakReference> &&item = iterator_get(reference.data<Iterator>())) {
				return MINT_TERM_FG_MAGENTA "iterator:" MINT_TERM_RESET " " + reference_value(item.value())
					   + MINT_TERM_RESET;
			}
			return MINT_TERM_FG_MAGENTA "iterator:" MINT_TERM_FG_YELLOW " empty" MINT_TERM_RESET;
		case Class::LIBRARY:
			return MINT_TERM_FG_MAGENTA "library:" MINT_TERM_RESET " " + reference.data<Library>()->plugin->get_path()
				   + MINT_TERM_RESET;
		case Class::LIBOBJECT:
			return MINT_TERM_FG_MAGENTA "libobject:" MINT_TERM_RESET " " + to_string(reference.data())
				   + MINT_TERM_RESET;
		}
		break;
	default:
		return MINT_TERM_FG_YELLOW + to_string(reference) + MINT_TERM_RESET;
	}
	return {};
}

Output::Output() {}

Output::~Output() {
	Terminal::print(stdout, "\n");
}

Output &Output::instance() {

	static Output g_instance;

	return g_instance;
}

void Output::print(Reference &reference) {
	Terminal::printf(stdout, "%s\n", reference_value(reference).c_str());
}

bool Output::global() const {
	return true;
}
