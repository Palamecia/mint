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

#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/class.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/reference.h"
#include "mint/system/plugin.h"
#include "mint/system/error.h"
#include "mint/scheduler/scheduler.h"
#include <cstddef>
#include <exception>
#include <string>
#include <utility>

using namespace mint;

LibraryClass& LibraryClass::instance(AbstractSyntaxTree& ast) {
	return ast.global_data().builtin<LibraryClass>(Class::Metatype::library);
}

Library::Library(AbstractSyntaxTree& ast) :
    Object(LibraryClass::instance(ast)) {}

Library::Library(Library&& other) noexcept :
    Object(other.metadata),
    plugin(std::move(other.plugin)) {}

Library::Library(const Library& other) :
    Object(other.metadata),
    plugin(other.plugin ? new Plugin(other.plugin->get_path()) : nullptr) {}

Library& Library::operator=(Library&& other) noexcept {
	std::swap(plugin, other.plugin);
	return *this;
}

Library& Library::operator=(const Library& other) {
	if (this == &other) [[unlikely]] {
		return *this;
	}
	plugin.reset(other.plugin ? new Plugin(other.plugin->get_path()) : nullptr);
	return *this;
}

Library::~Library() {}

LibraryClass::LibraryClass(AbstractSyntaxTree& ast) :
    Class(ast.global_data(), "lib", Class::Metatype::library) {

	create_builtin_member(new_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& name = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		try {
			self.data<Library>().plugin = Plugin::load(to_string(name));
			cursor.stack().pop_back();
		}
		catch (const std::exception&) {
			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().emplace_back(create_none());
		}
	}));

	create_builtin_member("call", ast.create_builtin_method(*this, variadic(2), [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto va_args = move_from_stack(cursor, base);
		const auto function = move_from_stack(cursor, base - 1);
		const auto self = move_from_stack(cursor, base - 2);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().pop_back();

		std::string func_name = to_string(function);
		auto* plugin = self.data<Library>().plugin.get();

		const auto signature = static_cast<int>(va_args.data<Iterator>().ctx.size());
		for (Iterator::Context::value_type& arg : va_args.data<Iterator>().ctx) {
			cursor.stack().emplace_back(std::forward<Reference>(arg));
		}

		if (!plugin->call(func_name, signature, cursor)) [[unlikely]] {
			error("no function '{}' taking {} arguments found in plugin '{}'", func_name, signature,
			    plugin->get_path().generic_string());
		}
	}));
}
