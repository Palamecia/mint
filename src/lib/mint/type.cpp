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
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(to_number(cursor, value)));
}

MINT_FUNCTION(mint_type_to_boolean, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_boolean(to_boolean(cursor, value)));
}

MINT_FUNCTION(mint_type_to_string, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_string(to_string(value)));
}

MINT_FUNCTION(mint_type_to_regex, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	SharedReference result = SharedReference::strong<Regex>();
	result->data<Regex>()->initializer = "/" + to_string(value) + "/";
	result->data<Regex>()->expr = to_regex(value);
	result->data<Regex>()->construct();
	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_type_to_array, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_array(to_array(value)));
}

MINT_FUNCTION(mint_type_to_hash, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_hash(to_hash(cursor, value)));
}

MINT_FUNCTION(mint_type_get_member_owner, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference member_name = move(helper.popParameter());
	SharedReference type = move(helper.popParameter());

	if (type->data()->format == Data::fmt_object && type->data<Object>()->metadata->metatype() == Class::object) {

		auto i = type->data<Object>()->metadata->members().find(Symbol(to_string(member_name)));

		if (i != type->data<Object>()->metadata->members().end()) {
			helper.returnValue(SharedReference::strong(Reference::alloc<Object>(i->second->owner)));
		}
	}
}

MINT_FUNCTION(mint_type_set_member_owner, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference owner = move(helper.popParameter());
	SharedReference member_name = move(helper.popParameter());
	SharedReference type = move(helper.popParameter());

	bool success = false;

	if (owner->data()->format == Data::fmt_object && owner->data<Object>()->metadata->metatype() == Class::object) {

		Class *metadata = owner->data<Object>()->metadata;

		if (type->data()->format == Data::fmt_object && type->data<Object>()->metadata->metatype() == Class::object) {

			auto i = type->data<Object>()->metadata->members().find(Symbol(to_string(member_name)));

			if (i != type->data<Object>()->metadata->members().end()) {
				i->second->owner = metadata;
				success = true;
			}
		}
	}

	helper.returnValue(create_boolean(success));
}

MINT_FUNCTION(mint_type_is_copyable, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference type = move(helper.popParameter());

	switch (type->data()->format) {
	case Data::fmt_object:
		helper.returnValue(create_boolean(type->data<Object>()->metadata->isCopyable()));
		break;

	default:
		helper.returnValue(create_boolean(true));
		break;
	}
}

MINT_FUNCTION(mint_type_disable_copy, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference type = move(helper.popParameter());

	switch (type->data()->format) {
	case Data::fmt_object:
		type->data<Object>()->metadata->disableCopy();
		helper.returnValue(create_boolean(true));
		break;

	default:
		helper.returnValue(create_boolean(false));
		break;
	}
}

MINT_FUNCTION(mint_type_deep_copy, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	SharedReference deep_copy = SharedReference::strong(Reference::standard);
	deep_copy->clone(*value);
	helper.returnValue(move(deep_copy));
}

MINT_FUNCTION(mint_type_is_class, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = move(helper.popParameter());

	if (object->data()->format == Data::fmt_object) {
		helper.returnValue(create_boolean(mint::is_class(object->data<Object>())));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}

MINT_FUNCTION(mint_type_is_object, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference object = move(helper.popParameter());

	if (object->data()->format == Data::fmt_object) {
		helper.returnValue(create_boolean(mint::is_object(object->data<Object>())));
	}
	else {
		helper.returnValue(create_boolean(true));
	}
}

MINT_FUNCTION(mint_type_super, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference type = move(helper.popParameter());
	SharedReference result = create_array();

	if (type->data()->format == Data::fmt_object) {
		for (Class *base : type->data<Object>()->metadata->bases()) {
			array_append(result->data<Array>(), SharedReference::strong(Reference::alloc<Object>(base)));
		}
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_type_is_base_of, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference type = move(helper.popParameter());
	SharedReference base = move(helper.popParameter());

	if (base->data()->format == Data::fmt_object && type->data()->format == Data::fmt_object) {
		helper.returnValue(create_boolean(base->data<Object>()->metadata->isBaseOf(type->data<Object>()->metadata)));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
}
