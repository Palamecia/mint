/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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
#include "mint/compiler/type_annotation.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/compiler/descriptions.h"
#include "mint/memory/class.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/program/function_literal.h"
#include "mint/program/module.h"
#include "mint/program/program.h"
#include "mint/program/cursor.h"
#include "mint/memory/reference.h"
#include "mint/system/plugin.h"
#include "mint/system/error.h"
#include "mint/scheduler/scheduler.h"
#include <exception>
#include <string>
#include <utility>

using namespace mint;

namespace {

ClassDescription make_description() {
	auto description = ClassDescription(Module::invalid_id, "lib");
	description
	    .create_method(get_operator_symbol(Class::new_operator),
	        make_union(BuiltinTypeKind::library, PrimitiveTypeKind::none))
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "path"})
	    .set_required_parameter_count(2);
	description.create_method("call", {})
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	description.create_method("callAsNumber", PrimitiveTypeKind::number)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	description.create_method("callAsBoolean", PrimitiveTypeKind::boolean)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	description.create_method("callAsString", BuiltinTypeKind::string)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	description.create_method("callAsRegex", BuiltinTypeKind::regex)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	description.create_method("callAsArray", BuiltinTypeKind::array)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	description.create_method("callAsHash", BuiltinTypeKind::hash)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::library})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	return description;
}

}

ClassDescription& mint::LibraryClass::description() {
	static auto g_description = make_description();
	return g_description;
}

LibraryClass& LibraryClass::instance(Program& program) {
	return program.global_data().builtin<LibraryClass>(Class::Metatype::library);
}

Library::Library(Program& program) :
    Object(LibraryClass::instance(program)) {}

Library::Library(Program& program, const std::string& plugin) :
    Object(LibraryClass::instance(program)),
    plugin(Plugin::load(plugin)) {}

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

LibraryClass::LibraryClass(Program& program) :
    Class(program.global_data(), description(), Class::Metatype::library) {

	create_builtin_member(new_operator, program.create_builtin_method(*this, 2, [](Cursor& cursor) {
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

	auto call_handle = program.create_builtin_method(*this, variadic(2), [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto va_args = move_from_stack(cursor, base);
		const auto function = move_from_stack(cursor, base - 1);
		const auto self = move_from_stack(cursor, base - 2);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().pop_back();

		const auto func_name = to_string(function);
		auto* plugin = self.data<Library>().plugin.get();

		const auto signature = static_cast<int>(va_args.data<Iterator>().ctx.size());
		for (Iterator::Context::value_type& arg : va_args.data<Iterator>().ctx) {
			cursor.stack().emplace_back(std::forward<Reference>(arg));
		}

		if (!plugin->call(func_name, signature, cursor)) [[unlikely]] {
			error("no function '{}' taking {} arguments found in plugin '{}'", func_name, signature,
			    plugin->get_path().generic_string());
		}
	});

	create_builtin_member("call", call_handle);
	create_builtin_member("callAsNumber", call_handle);
	create_builtin_member("callAsBoolean", call_handle);
	create_builtin_member("callAsString", call_handle);
	create_builtin_member("callAsRegex", call_handle);
	create_builtin_member("callAsArray", call_handle);
	create_builtin_member("callAsHash", call_handle);
}
