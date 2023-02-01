#include <memory/functiontool.h>
#include <memory/builtin/library.h>
#include <memory/builtin/regex.h>
#include <memory/builtin/string.h>
#include <memory/casttool.h>
#include <system/plugin.h>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_type_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_number(to_number(cursor, value)));
}

MINT_FUNCTION(mint_type_to_boolean, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_boolean(to_boolean(cursor, value)));
}

MINT_FUNCTION(mint_type_to_string, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_string(to_string(value)));
}

MINT_FUNCTION(mint_type_to_regex, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	WeakReference result = WeakReference::create<Regex>();
	result.data<Regex>()->initializer = "/" + to_string(value) + "/";
	result.data<Regex>()->expr = to_regex(value);
	result.data<Regex>()->construct();
	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_type_to_array, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_array(to_array(value)));
}

MINT_FUNCTION(mint_type_to_hash, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(create_hash(to_hash(cursor, value)));
}

MINT_FUNCTION(mint_type_get_member_owner, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &member_name = helper.popParameter();
	Reference &type = helper.popParameter();

	if (type.data()->format == Data::fmt_object && type.data<Object>()->metadata->metatype() == Class::object) {

		auto i = type.data<Object>()->metadata->members().find(Symbol(to_string(member_name)));

		if (i != type.data<Object>()->metadata->members().end()) {
			helper.returnValue(WeakReference::create<Object>(i->second->owner));
		}
	}
}

MINT_FUNCTION(mint_type_set_member_owner, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &owner = helper.popParameter();
	Reference &member_name = helper.popParameter();
	Reference &type = helper.popParameter();

	bool success = false;

	if (owner.data()->format == Data::fmt_object && owner.data<Object>()->metadata->metatype() == Class::object) {

		Class *metadata = owner.data<Object>()->metadata;

		if (type.data()->format == Data::fmt_object && type.data<Object>()->metadata->metatype() == Class::object) {

			auto i = type.data<Object>()->metadata->members().find(Symbol(to_string(member_name)));

			if (i != type.data<Object>()->metadata->members().end()) {
				i->second->owner = metadata;
				success = true;
			}
		}
	}

	helper.returnValue(create_boolean(success));
}

MINT_FUNCTION(mint_type_is_copyable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.popParameter();

	switch (type.data()->format) {
	case Data::fmt_object:
		helper.returnValue(create_boolean(type.data<Object>()->metadata->isCopyable()));
		break;

	default:
		helper.returnValue(create_boolean(true));
		break;
	}
}

MINT_FUNCTION(mint_type_disable_copy, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.popParameter();

	switch (type.data()->format) {
	case Data::fmt_object:
		type.data<Object>()->metadata->disableCopy();
		helper.returnValue(create_boolean(true));
		break;

	default:
		helper.returnValue(create_boolean(false));
		break;
	}
}

MINT_FUNCTION(mint_type_deep_copy, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &value = helper.popParameter();
	helper.returnValue(WeakReference::clone(value));
}

MINT_FUNCTION(mint_type_is_class, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &object = helper.popParameter();
	helper.returnValue(create_boolean(mint::is_class(object)));
}

MINT_FUNCTION(mint_type_is_object, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &object = helper.popParameter();

	if (object.data()->format == Data::fmt_object) {
		helper.returnValue(create_boolean(mint::is_object(object.data<Object>())));
	}
	else {
		helper.returnValue(create_boolean(true));
	}
}

MINT_FUNCTION(mint_type_super, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.popParameter();
	WeakReference result = create_array();

	if (type.data()->format == Data::fmt_object) {
		for (Class *base : type.data<Object>()->metadata->bases()) {
			array_append(result.data<Array>(), WeakReference::create<Object>(base));
		}
	}

	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_type_is_base_of, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &type = helper.popParameter();
	Reference &base = helper.popParameter();

	if (base.data()->format == Data::fmt_object && type.data()->format == Data::fmt_object) {
		helper.returnValue(create_boolean(base.data<Object>()->metadata->isBaseOf(type.data<Object>()->metadata)));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}
