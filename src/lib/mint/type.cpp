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

#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/classtool.h"
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/ast/classregister.h"
#include "mint/system/plugin.h"
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace symbols {

static const mint::Symbol name("name");
static const mint::Symbol flags("flags");

static const std::string member_info("MemberInfo");

}

namespace {

mint::Reference mint_type_to_number(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_number(to_number(cursor, value));
}

mint::Reference mint_type_to_boolean(mint::Cursor& /*cursor*/, const mint::Reference& value) {
	return mint::create_boolean(to_boolean(value));
}

mint::Reference mint_type_to_string(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_string(cursor.ast(), to_string(value));
}

mint::Reference mint_type_to_regex(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_regex(cursor.ast(), mint::to_string(value), mint::to_regex(value));
}

mint::Reference mint_type_to_array(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_array(cursor.ast(), mint::to_array(value));
}

mint::Reference mint_type_to_hash(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_hash(cursor.ast(), mint::to_hash(value));
}

mint::Reference mint_lang_get_type(mint::Cursor& /*cursor*/, const mint::Reference& object) {
	if (is_instance_of(object, mint::Class::Metatype::object)) {
		return mint::create_object(object.data<mint::Object>().metadata);
	}
	return {};
}

mint::Reference mint_lang_create_type(mint::Cursor& cursor, const mint::Reference& type, const mint::Reference& bases,
    const mint::Reference& members) {

	auto base_list = std::vector<mint::ClassRegister::Path>();

	for (const mint::Reference& base : mint::to_array(bases)) {
		switch (base.data().format()) {
		case mint::Data::Format::object:
			base_list.emplace_back(base.data<mint::Object>().metadata.get_description().get_path());
			break;
		default:
			base_list.emplace_back(to_string(base));
			break;
		}
	}

	auto member_list = std::vector<std::pair<mint::Symbol, mint::Reference>>();

	for (auto& member : to_hash(members)) {
		if (is_instance_of(member.first, symbols::member_info)) {
			const auto symbol = mint::Symbol(
			    to_string(get_member_ignore_visibility(member.first.data<mint::Object>(), symbols::name)));
			const auto flags = to_integer<mint::Reference::Flags>(cursor,
			    get_member_ignore_visibility(member.first.data<mint::Object>(), symbols::flags));
			member_list.emplace_back(symbol, mint::Reference(flags, member.second.data()));
		}
		else {
			member_list.emplace_back(mint::Symbol(to_string(member.first)), std::move(member.second));
		}
	}

	return mint::create_alias(mint::create_class(cursor.ast(), to_string(type), base_list, member_list));
}

mint::Reference mint_type_get_member_info(mint::Cursor& cursor, const mint::Reference& type,
    mint::Reference& member_name) {
	if (is_instance_of(type, mint::Class::Metatype::object)) {
		if (auto* member = type.data<mint::Object>().metadata.find_member(mint::Symbol(to_string(member_name)))) {
			return create_iterator_from(cursor, mint::Reference(member_name),
			    mint::create_number(member->value.flags() & ~mint::Reference::temporary),
			    mint::create_alias(member->owner));
		}
	}
	return {};
}

mint::Reference mint_type_is_member_private(mint::Cursor& /*cursor*/, const mint::Reference& type,
    const mint::Reference& member_name) {
	if (is_instance_of(type, mint::Class::Metatype::object)) {
		if (auto* member = type.data<mint::Object>().metadata.find_member(mint::Symbol(to_string(member_name)))) {
			return mint::create_boolean(
			    (member->value.flags() & mint::Reference::visibility_mask) == mint::Reference::private_visibility);
		}
	}
	return {};
}

mint::Reference mint_type_is_member_protected(mint::Cursor& /*cursor*/, const mint::Reference& type,
    const mint::Reference& member_name) {
	if (is_instance_of(type, mint::Class::Metatype::object)) {
		if (auto* member = type.data<mint::Object>().metadata.find_member(mint::Symbol(to_string(member_name)))) {
			return mint::create_boolean(
			    (member->value.flags() & mint::Reference::visibility_mask) == mint::Reference::protected_visibility);
		}
	}
	return {};
}

mint::Reference mint_type_get_member_owner(mint::Cursor& /*cursor*/, const mint::Reference& type,
    const mint::Reference& member_name) {
	if (is_instance_of(type, mint::Class::Metatype::object)) {
		if (auto* member = type.data<mint::Object>().metadata.find_member(mint::Symbol(to_string(member_name)))) {
			return mint::create_alias(member->owner);
		}
	}
	return {};
}

mint::Reference mint_type_is_trivially_copyable(mint::Cursor& /*cursor*/, const mint::Reference& type) {
	if (!mint::is_instance_of(type, mint::Data::Format::object)) {
		return mint::create_boolean(true);
	}
	return mint::create_boolean(type.data<mint::Object>().metadata.is_trivially_copyable());
}

mint::Reference mint_type_deep_copy(mint::Cursor& /*cursor*/, const mint::Reference& value) {
	return {mint::copy_from, value};
}

mint::Reference mint_type_is_class(mint::Cursor& /*cursor*/, const mint::Reference& object) {
	return mint::create_boolean(mint::is_class(object));
}

mint::Reference mint_type_is_object(mint::Cursor& /*cursor*/, const mint::Reference& object) {
	if (mint::is_instance_of(object, mint::Data::Format::object)) {
		return mint::create_boolean(mint::is_object(object.data<mint::Object>()));
	}
	return mint::create_boolean(true);
}

mint::Reference mint_type_super(mint::Cursor& cursor, const mint::Reference& type) {
	if (type.data().format() == mint::Data::Format::object) {
		return mint::create_array(cursor.ast(),
		    {std::from_range, std::views::transform(type.data<mint::Object>().metadata.bases(), [](mint::Class& base) {
			     return mint::create_alias(base);
		     })});
	}
	return mint::create_array(cursor.ast());
}

mint::Reference mint_type_is_base_of(mint::Cursor& /*cursor*/, const mint::Reference& base,
    const mint::Reference& type) {
	if (base.data().format() == mint::Data::Format::object && type.data().format() == mint::Data::Format::object) {
		return mint::create_boolean(base.data<mint::Object>().metadata.is_base_of(type.data<mint::Object>().metadata));
	}
	return mint::create_boolean(false);
}

mint::Reference mint_type_is_base_or_same(mint::Cursor& /*cursor*/, const mint::Reference& base,
    const mint::Reference& type) {
	if (base.data().format() == mint::Data::Format::object && type.data().format() == mint::Data::Format::object) {
		return mint::create_boolean(
		    base.data<mint::Object>().metadata.is_base_or_same(type.data<mint::Object>().metadata));
	}
	return mint::create_boolean(false);
}

mint::Reference mint_type_is_instance_of(mint::Cursor& /*cursor*/, const mint::Reference& object,
    const mint::Reference& type) {
	if (object.data().format() == mint::Data::Format::object && type.data().format() == mint::Data::Format::object) {
		return mint::create_boolean(object.data<mint::Object>().metadata.is_same(type.data<mint::Object>().metadata));
	}
	return mint::create_boolean(false);
}

}

MINT_EXPORT_FUNCTION(mint_type_to_number, 1);
MINT_EXPORT_FUNCTION(mint_type_to_boolean, 1);
MINT_EXPORT_FUNCTION(mint_type_to_string, 1);
MINT_EXPORT_FUNCTION(mint_type_to_regex, 1);
MINT_EXPORT_FUNCTION(mint_type_to_array, 1);
MINT_EXPORT_FUNCTION(mint_type_to_hash, 1);
MINT_EXPORT_FUNCTION(mint_lang_get_type, 1);
MINT_EXPORT_FUNCTION(mint_lang_create_type, 3);
MINT_EXPORT_FUNCTION(mint_type_get_member_info, 2);
MINT_EXPORT_FUNCTION(mint_type_is_member_private, 2);
MINT_EXPORT_FUNCTION(mint_type_is_member_protected, 2);
MINT_EXPORT_FUNCTION(mint_type_get_member_owner, 2);
MINT_EXPORT_FUNCTION(mint_type_is_trivially_copyable, 1);
MINT_EXPORT_FUNCTION(mint_type_deep_copy, 1);
MINT_EXPORT_FUNCTION(mint_type_is_class, 1);
MINT_EXPORT_FUNCTION(mint_type_is_object, 1);
MINT_EXPORT_FUNCTION(mint_type_super, 1);
MINT_EXPORT_FUNCTION(mint_type_is_base_of, 2);
MINT_EXPORT_FUNCTION(mint_type_is_base_or_same, 2);
MINT_EXPORT_FUNCTION(mint_type_is_instance_of, 2);
