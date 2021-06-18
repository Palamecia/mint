#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <cinttypes>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_int8_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	int8_t *data = new int8_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_int8_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<int8_t>>()->impl;
}

MINT_FUNCTION(mint_int8_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<int8_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_int8_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<int8_t>>()->impl));
}

MINT_FUNCTION(mint_int16_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	int16_t *data = new int16_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_int16_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<int16_t>>()->impl;
}

MINT_FUNCTION(mint_int16_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<int16_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_int16_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<int16_t>>()->impl));
}

MINT_FUNCTION(mint_int32_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	int32_t *data = new int32_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_int32_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<int32_t>>()->impl;
}

MINT_FUNCTION(mint_int32_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<int32_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_int32_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<int32_t>>()->impl));
}

MINT_FUNCTION(mint_int64_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	int64_t *data = new int64_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_int64_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<int64_t>>()->impl;
}

MINT_FUNCTION(mint_int64_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<int64_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_int64_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<int64_t>>()->impl));
}

MINT_FUNCTION(mint_uint8_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	uint8_t *data = new uint8_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_uint8_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<uint8_t>>()->impl;
}

MINT_FUNCTION(mint_uint8_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<uint8_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_uint8_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<uint8_t>>()->impl));
}

MINT_FUNCTION(mint_uint16_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	uint16_t *data = new uint16_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_uint16_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<uint16_t>>()->impl;
}

MINT_FUNCTION(mint_uint16_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<uint16_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_uint16_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<uint16_t>>()->impl));
}

MINT_FUNCTION(mint_uint32_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	uint32_t *data = new uint32_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_uint32_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<uint32_t>>()->impl;
}

MINT_FUNCTION(mint_uint32_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<uint32_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_uint32_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<uint32_t>>()->impl));
}

MINT_FUNCTION(mint_uint64_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	uint64_t *data = new uint64_t;
	*data = to_integer(cursor, value);
	helper.returnValue(create_object(data));
}

MINT_FUNCTION(mint_uint64_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	delete value->data<LibObject<uint64_t>>()->impl;
}

MINT_FUNCTION(mint_uint64_copy, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference source = move(helper.popParameter());
	SharedReference target = move(helper.popParameter());
	*target->data<LibObject<uint64_t>>()->impl = to_integer(cursor, source);
}

MINT_FUNCTION(mint_uint64_to_number, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference value = move(helper.popParameter());
	helper.returnValue(create_number(*value->data<LibObject<uint64_t>>()->impl));
}
