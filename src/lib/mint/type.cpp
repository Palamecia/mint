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

#include <mint/memory/functiontool.h>
#include <mint/memory/builtin/library.h>
#include <mint/memory/builtin/regex.h>
#include <mint/memory/builtin/string.h>
#include <mint/memory/globaldata.h>
#include <mint/memory/casttool.h>
#include <mint/ast/classregister.h>
#include <mint/system/plugin.h>

using namespace mint;

namespace symbols {

static const Symbol name("name");
static const Symbol flags("flags");

static const std::string MemberInfo("MemberInfo");

}

MINT_FUNCTION(mint_type_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.pop_parameter();
	helper.return_value(create_number(to_number(cursor, value)));
}

MINT_FUNCTION(mint_type_to_boolean, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.pop_parameter();
	helper.return_value(create_boolean(to_boolean(value)));
}

MINT_FUNCTION(mint_type_to_string, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &value = helper.pop_parameter();
	helper.return_value(create_string(to_string(value)));
}

MINT_FUNCTION(mint_type_to_regex, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.pop_parameter();
	WeakReference result = WeakReference::create<Regex>();
	result.data<Regex>()->initializer = "/" + to_string(value) + "/";
	result.data<Regex>()->expr = to_regex(value);
	result.data<Regex>()->construct();
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_type_to_array, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.pop_parameter();
	helper.return_value(create_array(to_array(value)));
}

MINT_FUNCTION(mint_type_to_hash, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.pop_parameter();
	helper.return_value(create_hash(to_hash(value)));
}

MINT_FUNCTION(mint_lang_get_type, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &object = helper.pop_parameter();

	if (is_instance_of(object, Class::OBJECT)) {
		helper.return_value(WeakReference::create<Object>(object.data<Object>()->metadata));
	}
}

MINT_FUNCTION(mint_lang_create_type, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &members = helper.pop_parameter();
	Reference &bases = helper.pop_parameter();
	Reference &type = helper.pop_parameter();

	auto *description = new ClassDescription(GlobalData::instance(), Reference::DEFAULT, to_string(type));

	for (Reference &base : to_array(bases)) {
		switch (base.data()->format) {
		case Data::FMT_OBJECT:
			description->add_base(base.data<Object>()->metadata->get_description()->get_path());
			break;

		default:
			description->add_base(Symbol(to_string(base)));
			break;
		}
	}

	for (auto &member : to_hash(members)) {
		if (is_instance_of(member.first, symbols::MemberInfo)) {
			Symbol symbol(to_string(get_member_ignore_visibility(member.first.data<Object>(), symbols::name)));
			Reference::Flags flags = static_cast<Reference::Flags>(
				to_integer(cursor, get_member_ignore_visibility(member.first.data<Object>(), symbols::flags)));
			if (std::optional<Class::Operator> op = get_symbol_operator(symbol)) {
				description->create_member(*op, WeakReference(flags, member.second.data()));
			}
			else {
				description->create_member(symbol, WeakReference(flags, member.second.data()));
			}
		}
		else {
			Symbol symbol(to_string(member.first));
			if (std::optional<Class::Operator> op = get_symbol_operator(symbol)) {
				description->create_member(*op, std::move(member.second));
			}
			else {
				description->create_member(symbol, std::move(member.second));
			}
		}
	}

	if (Class *prototype = description->generate()) {
		helper.return_value(WeakReference::create<Object>(prototype));
	}

	GlobalData::instance()->create_class(description);
}

MINT_FUNCTION(mint_type_get_member_info, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &member_name = helper.pop_parameter();
	Reference &type = helper.pop_parameter();

	if (is_instance_of(type, Class::OBJECT)) {
		auto i = type.data<Object>()->metadata->members().find(Symbol(to_string(member_name)));
		if (i != type.data<Object>()->metadata->members().end()) {
			helper.return_value(create_iterator(WeakReference::share(member_name),
												create_number(i->second->value.flags() & ~Reference::TEMPORARY),
												WeakReference::create<Object>(i->second->owner)));
		}
	}
}

MINT_FUNCTION(mint_type_is_member_private, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &member_name = helper.pop_parameter();
	const Reference &type = helper.pop_parameter();

	if (is_instance_of(type, Class::OBJECT)) {
		auto i = type.data<Object>()->metadata->members().find(Symbol(to_string(member_name)));
		if (i != type.data<Object>()->metadata->members().end()) {
			helper.return_value(WeakReference::create<Boolean>((i->second->value.flags() & Reference::VISIBILITY_MASK)
															   == Reference::PRIVATE_VISIBILITY));
		}
	}
}

MINT_FUNCTION(mint_type_is_member_protected, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &member_name = helper.pop_parameter();
	const Reference &type = helper.pop_parameter();

	if (is_instance_of(type, Class::OBJECT)) {
		auto i = type.data<Object>()->metadata->members().find(Symbol(to_string(member_name)));
		if (i != type.data<Object>()->metadata->members().end()) {
			helper.return_value(WeakReference::create<Boolean>((i->second->value.flags() & Reference::VISIBILITY_MASK)
															   == Reference::PROTECTED_VISIBILITY));
		}
	}
}

MINT_FUNCTION(mint_type_get_member_owner, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &member_name = helper.pop_parameter();
	const Reference &type = helper.pop_parameter();

	if (is_instance_of(type, Class::OBJECT)) {
		auto i = type.data<Object>()->metadata->members().find(Symbol(to_string(member_name)));
		if (i != type.data<Object>()->metadata->members().end()) {
			helper.return_value(WeakReference::create<Object>(i->second->owner));
		}
	}
}

MINT_FUNCTION(mint_type_is_copyable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.pop_parameter();

	switch (type.data()->format) {
	case Data::FMT_OBJECT:
		helper.return_value(create_boolean(type.data<Object>()->metadata->is_copyable()));
		break;

	default:
		helper.return_value(create_boolean(true));
		break;
	}
}

MINT_FUNCTION(mint_type_deep_copy, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &value = helper.pop_parameter();
	helper.return_value(WeakReference::clone(value));
}

MINT_FUNCTION(mint_type_is_class, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &object = helper.pop_parameter();
	helper.return_value(create_boolean(mint::is_class(object)));
}

MINT_FUNCTION(mint_type_is_object, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &object = helper.pop_parameter();

	if (object.data()->format == Data::FMT_OBJECT) {
		helper.return_value(create_boolean(mint::is_object(object.data<Object>())));
	}
	else {
		helper.return_value(create_boolean(true));
	}
}

MINT_FUNCTION(mint_type_super, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.pop_parameter();
	WeakReference result = create_array();

	if (type.data()->format == Data::FMT_OBJECT) {
		for (Class *base : type.data<Object>()->metadata->bases()) {
			array_append(result.data<Array>(), WeakReference::create<Object>(base));
		}
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_type_is_base_of, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &type = helper.pop_parameter();
	Reference &base = helper.pop_parameter();

	if (base.data()->format == Data::FMT_OBJECT && type.data()->format == Data::FMT_OBJECT) {
		helper.return_value(create_boolean(base.data<Object>()->metadata->is_base_of(type.data<Object>()->metadata)));
	}
	else {
		helper.return_value(create_boolean(false));
	}
}

MINT_FUNCTION(mint_type_is_base_or_same, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &type = helper.pop_parameter();
	Reference &base = helper.pop_parameter();

	if (base.data()->format == Data::FMT_OBJECT && type.data()->format == Data::FMT_OBJECT) {
		helper.return_value(
			create_boolean(base.data<Object>()->metadata->is_base_or_same(type.data<Object>()->metadata)));
	}
	else {
		helper.return_value(create_boolean(false));
	}
}

MINT_FUNCTION(mint_type_is_instance_of, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &type = helper.pop_parameter();
	Reference &object = helper.pop_parameter();

	if (object.data()->format == Data::FMT_OBJECT && type.data()->format == Data::FMT_OBJECT) {
		helper.return_value(create_boolean(object.data<Object>()->metadata == type.data<Object>()->metadata));
	}
	else {
		helper.return_value(create_boolean(false));
	}
}
