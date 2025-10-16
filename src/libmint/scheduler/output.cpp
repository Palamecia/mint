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

#include "mint/scheduler/output.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/module.h"
#include "mint/debug/debuginfo.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/casttool.h"
#include "mint/memory/class.h"
#include "mint/system/string.h"
#include "mint/system/terminal.h"
#include "mint/system/plugin.h"
#include <cstdio>
#include <format>
#include <ranges>
#include <string>

using namespace mint;

namespace {

std::string reference_value(const AbstractSyntaxTree& ast, const Reference& reference) {
	switch (reference.data().format()) {
	case Data::none_format:
		return MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_DARK) "none");
	case Data::null_format:
		return MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_DARK) "null");
	case Data::package_format:
		return std::format(MINT_TERM_STR(
		                       MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "package:" MINT_TERM_OPT(MINT_TERM_RESET) " {}"),
		    reference.data<Package>().data.full_name());
	case Data::function_format:
		return std::format(MINT_TERM_STR(
		                       MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "function:" MINT_TERM_OPT(MINT_TERM_RESET) " {}"),
		    std::views::transform(reference.data<Function>().mapping,
		        [&ast](const auto& item) {
			        const auto& module = item.second.handle().module;
			        DebugInfo* infos = ast.find_debug_info(module);
			        return std::format("{}@{}(line {})", std::to_string(item.first), ast.get_module_name(module),
			            std::to_string(infos->line_number(item.second.handle().offset)));
		        })
		        | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
	case Data::object_format:
		switch (reference.data<Object>().metadata.metatype()) {
		case Class::object:
			if (mint::is_class(reference.data<Object>())) {
				return std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "class:" MINT_TERM_OPT(
				                       MINT_TERM_RESET) " {}"),
				    reference.data<Object>().metadata.full_name());
			}
			return std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "object:" MINT_TERM_OPT(
			                       MINT_TERM_RESET) " {} " MINT_TERM_OPT(MINT_TERM_DARK) "({})"),
			    reference.data<Object>().metadata.full_name(), to_string(&reference.data()));
		case Class::string:
			return MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_FG_YELLOW) "'" + to_string(reference) + "'");
		case Class::regex:
			return MINT_TERM_STDSTR(MINT_TERM_OPT(MINT_TERM_FG_RED) + to_string(reference));
		case Class::array:
			return std::format("[ {} ]", std::views::transform(reference.data<Array>().values,
			                                 [&ast](const auto& item) {
				                                 return reference_value(ast, item);
			                                 })
			                                 | std::views::join_with(std::string(", "))
			                                 | std::ranges::to<std::string>());
		case Class::hash:
			return std::format("{{ {} }}",
			    std::views::transform(reference.data<Hash>().values,
			        [&ast](const auto& item) {
				        return reference_value(ast, item.first) + ": " + reference_value(ast, item.second);
			        })
			        | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
		case Class::iterator:
			if (auto item = iterator_get(reference.data<Iterator>())) {
				return std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "iterator:" MINT_TERM_OPT(
				                       MINT_TERM_RESET) " {}"),
				    reference_value(ast, *item));
			}
			return MINT_TERM_STR(
			    MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "iterator:" MINT_TERM_OPT(MINT_TERM_FG_YELLOW) " empty");
		case Class::library:
			return std::format(MINT_TERM_STR(
			                       MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "library:" MINT_TERM_OPT(MINT_TERM_RESET) " {}"),
			    reference.data<Library>().plugin->get_path().generic_string());
		case Class::libobject:
			return std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_FG_MAGENTA) "libobject:" MINT_TERM_OPT(
			                       MINT_TERM_RESET) " {}"),
			    to_string(&reference.data()));
		}
		break;
	default:
		return std::format(MINT_TERM_STR(MINT_TERM_OPT(MINT_TERM_FG_GREEN) "{}"), to_string(reference));
	}
	return {};
}

}

mint::Output::Output(AbstractSyntaxTree& ast) :
    _ast(ast) {}

Output::~Output() {
	Terminal::print(stdout, "\n");
}

void Output::print(const Reference& reference) {
	Terminal::println(stdout, reference_value(_ast, reference));
}
