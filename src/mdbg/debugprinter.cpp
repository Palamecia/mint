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

#include "debugprinter.h"

#include "mint/ast/module.h"
#include "mint/debug/debuginfo.h"
#include "mint/memory/algorithm.hpp"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/system/assert.h"
#include "mint/system/string.h"
#include "mint/system/terminal.h"
#include "mint/system/plugin.h"
#include "mint/scheduler/scheduler.h"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <format>
#include <ranges>
#include <string>

DebugPrinter::~DebugPrinter() {}

void DebugPrinter::print(const mint::Reference& reference) {
	mint::visit<void>( //
	    mint::Overloaded {
	        [](mint::None&) {
		        mint::Terminal::println(stdout, "none");
	        },
	        [](mint::Null&) {
		        mint::Terminal::println(stdout, "null");
	        },
	        [](mint::Number& number) {
		        mint::Terminal::println(stdout, mint::to_string(number.value));
	        },
	        [](mint::Boolean& boolean) {
		        mint::Terminal::println(stdout, boolean.value ? "true" : "false");
	        },
	        [](mint::Object& object) {
		        const auto type = object.metadata.full_name();
		        mint::Terminal::println(stdout, std::format("({}) {{", type));
		        if (mint::is_object(object)) {
			        for (auto member : object.metadata.members()) {
				        mint::Terminal::println(stdout,
				            std::format("\t{} : ({}) {}", member.first.str(), type_name(member.second.get().value),
				                reference_value(mint::Class::MemberInfo::get(member.second, object))));
			        }
		        }
		        else {
			        for (auto member : object.metadata.members()) {
				        mint::Terminal::println(stdout,
				            std::format("\t{} : ({}) {}", member.first.str(), type_name(member.second.get().value),
				                reference_value(member.second.get().value)));
			        }
		        }
		        mint::Terminal::println(stdout, "}");
	        },
	        [](mint::String& string) {
		        mint::Terminal::println(stdout, std::format("\"{}\"", string.str));
	        },
	        [](mint::Regex& regex) {
		        mint::Terminal::println(stdout, regex.initializer);
	        },
	        [](mint::Array& array) {
		        mint::Terminal::println(stdout, array_value(array));
	        },
	        [](mint::Hash& hash) {
		        mint::Terminal::println(stdout, hash_value(hash));
	        },
	        [](mint::Iterator& iterator) {
		        mint::Terminal::println(stdout, iterator_value(iterator));
	        },
	        [](mint::Library& library) {
		        mint::Terminal::println(stdout, library_value(library));
	        },
	        [](mint::Data& libobject) {
		        mint::Terminal::println(stdout, object_value(libobject));
	        },
	        [](mint::Package& package) {
		        mint::Terminal::println(stdout, std::format("package: {}", package.data.full_name()));
	        },
	        [](mint::Function& function) {
		        mint::Terminal::println(stdout, function_value(function));
	        },
	    },
	    reference);
}

std::string reference_value(const mint::Reference& reference) {
	return mint::visit<std::string>( //
	    mint::Overloaded {
	        [](mint::None&) -> std::string {
		        return "none";
	        },
	        [](mint::Null&) -> std::string {
		        return "null";
	        },
	        [](mint::Number& number) -> std::string {
		        return mint::to_string(number.value);
	        },
	        [](mint::Boolean& boolean) -> std::string {
		        return boolean.value ? "true" : "false";
	        },
	        [](mint::String& string) -> std::string {
		        return std::format("\"{}\"", string.str);
	        },

	        [](mint::Regex& regex) -> std::string {
		        return regex.initializer;
	        },

	        [](mint::Array& array) -> std::string {
		        return array_value(array);
	        },

	        [](mint::Hash& hash) -> std::string {
		        return hash_value(hash);
	        },

	        [](mint::Iterator& iterator) -> std::string {
		        return iterator_value(iterator);
	        },

	        [](mint::Library& library) -> std::string {
		        return library_value(library);
	        },

	        [](mint::Object& object) -> std::string {
		        return object_value(object);
	        },
	        [](mint::Data& object) -> std::string {
		        if (mint::is_instance_of(mint::WeakReference(mint::Reference::default_flags, object),
		                mint::Class::libobject)) {
			        return object_value(object);
		        }
		        return "unknown";
	        },
	        [](mint::Package& package) -> std::string {
		        return package.data.full_name();
	        },
	        [](mint::Function& function) -> std::string {
		        return function_value(function);
	        },
	    },
	    reference);
}

std::string iterator_value(mint::Iterator& iterator) {
	return std::format("({})", std::views::transform(iterator.ctx,
	                               [](auto& item) {
		                               return reference_value(item);
	                               })
	                               | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
}

std::string array_value(mint::Array& array) {
	return std::format("[{}]", std::views::transform(array.values,
	                               [](auto& item) {
		                               return reference_value(mint::array_get_item(item));
	                               })
	                               | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
}

std::string hash_value(mint::Hash& hash) {
	return std::format("{{{}}}", std::views::transform(hash.values,
	                                 [](auto& item) {
		                                 return reference_value(mint::hash_get_key(item)) + " : "
		                                        + reference_value(mint::hash_get_value(item));
	                                 })
	                                 | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
}

std::string library_value(mint::Library& library) {
	return library.plugin->get_path().generic_string();
}

std::string object_value(mint::Data& object) {
	return std::format("0x{:X}", std::bit_cast<std::uintptr_t>(&object));
}

std::string function_value(mint::Function& function) {
	auto* scheduler = mint::Scheduler::instance();
	assert_x(scheduler, __func__, "execution should be done using a scheduler");
	return std::format("function: {}", std::views::transform(function.mapping,
	                                       [&ast = scheduler->ast()](auto& item) {
		                                       mint::Module& module = item.second.handle().module;
		                                       mint::DebugInfo* infos = ast.find_debug_info(module);
		                                       return std::format("{}@{}(line {})", std::to_string(item.first),
		                                           ast.get_module_name(module),
		                                           infos->line_number(item.second.handle().offset));
	                                       })
	                                       | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
}
