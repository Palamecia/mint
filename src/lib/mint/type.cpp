#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "memory/builtin/regex.h"

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
	Reference *result = StrongReference::create<Regex>();
	result->data<Regex>()->initializer = "/" + to_string(value) + "/";
	result->data<Regex>()->expr = to_regex(value);
	result->data<Regex>()->construct();
	helper.returnValue(result);
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

